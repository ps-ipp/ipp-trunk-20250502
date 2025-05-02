# include "gastro2.h"

void gstars (char *filename, CmpCatalog *Target) {

  int Nstars;
  char line[80];
  double det;
  off_t Nskip;
  int NX, NY, FoundAstrom, naxis;
  StarData *stars;
  FILE *f;

  /* open file for stars */
  f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't open file to load stars\n");
    exit (1);
  }
  if (!gfits_fread_header (f, &Target[0].header)) {
    fprintf (stderr, "ERROR: can't read image header\n");
    exit (1);
  }
  /* this line should not be needed */
  fseeko (f, Target[0].header.datasize, SEEK_SET); 

  NX = Target[0].header.Naxis[0];
  NY = Target[0].header.Naxis[1];

  /* default values for coords */
  InitCoords (&Target[0].coords, "DEC--TAN");
  Target[0].coords.pc1_1 = CCD_PC1_1; Target[0].coords.pc1_2 = CCD_PC1_2;
  Target[0].coords.pc2_1 = CCD_PC2_1; Target[0].coords.pc2_2 = CCD_PC2_2;
  Target[0].coords.cdelt1 = Target[0].coords.cdelt2 = ASEC_PIX / 3600.0;
  Target[0].coords.crpix1 = 0.5*NX;
  Target[0].coords.crpix2 = 0.5*NY;
  
  /* attempt to get detailed astrometric information from header */
  FoundAstrom = FALSE;
  if (!strcasecmp (ROUGH_ASTROMETRY, "header")) {
    if (!HEADER[0]) {
      FoundAstrom = GetCoords (&Target[0].coords, &Target[0].header);
    } else {
      Header header;

      if (!gfits_read_header (HEADER, &header)) {
	fprintf (stderr, "ERROR: can't load external header\n");
	exit (1);
      }
      FoundAstrom = GetCoords (&Target[0].coords, &header);
      gfits_free_header (&header);
    }
    if (!FoundAstrom) {
      fprintf (stderr, "header coordinates incomplete, trying for rough coordinates\n");
      strcpy (ROUGH_ASTROMETRY, "config");
    } else {
      /* make optional adustments to header values (standard problems) */
      if (FLIPX) {
	Target[0].coords.pc1_1 *= -1;
	Target[0].coords.crpix1 = NX - Target[0].coords.crpix1;
      }
      if (FLIPY) {
	Target[0].coords.pc2_2 *= -1;
	Target[0].coords.crpix2 = NY - Target[0].coords.crpix2;
      }
    }
  }
  
  /*** abstract RA & DEC keywords, formats */
  /* get just RA & DEC from header, other terms from config file */
  if (!strcasecmp (ROUGH_ASTROMETRY, "config")) {
    /* get RA & DEC from header */
    FoundAstrom = TRUE;
    FoundAstrom &= gfits_scan (&Target[0].header, "RA", "%s", 1, line);
    ohana_dms_to_ddd (&Target[0].coords.crval1, line);
    Target[0].coords.crval1 = Target[0].coords.crval1 * 15.0;
    FoundAstrom &= gfits_scan (&Target[0].header, "DEC", "%s", 1, line);
    ohana_dms_to_ddd (&Target[0].coords.crval2, line);
  }

  /* use RA & DEC from command line arguments */
  if (FORCE) {
    Target[0].coords.crval1 = F_RA;
    Target[0].coords.crval2 = F_DEC;
    if (VERBOSE) fprintf (stderr, " forcing coordinates to: %9.4f %9.4f\n", Target[0].coords.crval1, Target[0].coords.crval2);
    FoundAstrom = TRUE;
  }    

  if (VERBOSE) fprintf (stderr, "using coordinates: %9.4f %9.4f\n", Target[0].coords.crval1, Target[0].coords.crval2);
  if (!FoundAstrom) {
    fprintf (stderr, "ERROR: can't get any valid coordinates, fix config file?\n");
    exit (1);
  }

  /* at this point, we need to correct the crval1, crval2, and ROT_ZERO values
     based on the pole axis angle and the ra, dec offsets */

# define dcos(a) (cos((double)((a)*(RAD_DEG))))
# define dsin(a) (sin((double)((a)*(RAD_DEG))))

  if (POLAR_ALIGNMENT) {

    double X, Y, PD, PR, DE, RE, T1, T2, T3;

    X = Target[0].coords.crval1;
    Y = Target[0].coords.crval2;
    PD = POLE_DEC;   PR = POLE_RA;
    DE = DEC_OFFSET; RE = RA_OFFSET;
    
    T1 = dcos(Y-DE) * dcos(X-RE) * dsin(PD) + dsin(Y-DE) * dcos(PD);
    T2 = dcos(Y-DE) * dsin(X-RE);
    T3 = dsin(Y-DE) * dsin(PD) - dcos(Y-DE) * dcos(X-RE) * dcos(PD);
    
    Target[0].coords.crval1 = (DEG_RAD * atan2 (T2, T1)) + PR;
    Target[0].coords.crval2 = (DEG_RAD * asin (T3));
    Target[0].coords.crval1 = ohana_normalize_angle (Target[0].coords.crval1);
    
    if (VERBOSE) fprintf (stderr, "  after polar alignment: %9.4f %9.4f\n", Target[0].coords.crval1, Target[0].coords.crval2);
  }

  /* get image area in deg^2 */
  det = Target[0].coords.pc1_1 * Target[0].coords.pc2_2 - Target[0].coords.pc1_2 * Target[0].coords.pc2_1;
  Target[0].Area = fabs (NX*NY*Target[0].coords.cdelt1*Target[0].coords.cdelt2*det);

  /* read from FITS table or from text table */
  /* Is NAXIS == 0 a better test?? */
  gfits_scan_alt (&Target[0].header, "NAXIS",  "%d", 1, &naxis);
  if ((naxis == 0) && !TEXTMODE) {
    Nskip = gfits_data_size (&Target[0].header);
    fseeko (f, Nskip, SEEK_CUR); 
    stars = rfits (f, &Nstars);
  } else {
    /* allocate space for stars */
    if (!gfits_scan (&Target[0].header, "NSTARS", "%d", 1, &Nstars)) {
      fprintf (stderr, "ERROR: failed to find NSTARS\n");
      exit (1);
    }
    stars = rtext (f, &Nstars);
  }
  fclose (f);

  stars = remove_clumps (stars, &Nstars, NX, NY);

  sort_stars_mag (stars, Nstars);  /* sorting by magnitude */
  Target[0].stars = stars;
  Target[0].N = Nstars;

  /* limit number of stars */
  if (GASTRO_MAX_NSTARS && (GASTRO_MAX_NSTARS < Target[0].N)) {
    Target[0].N = GASTRO_MAX_NSTARS;
    REALLOCATE (Target[0].stars, StarData, Target[0].N);
  }
  if (VERBOSE) fprintf (stderr, "using %d stars from data file\n", Target[0].N);

  /* calculate luminosity function of stars */
  get_luminosity_func (stars, Target[0].N, &Target[0].lum);

}

/* 

load cmp file FITS header 

extract needed data from header:
- NX, NY?
- coords
   
load stellar photometry

sort stars by mag

filter & limit numbers

find luminosity function slope, area?

*/
