# include "gastro.h"
# define dcos(a) (cos((double)((a)*(RAD_DEG))))
# define dsin(a) (sin((double)((a)*(RAD_DEG))))

/* by necesity hard wired */
# define D_NSTARS 1000
# define BYTES_STAR 66
# define BLOCK 1000
# include <sys/time.h>
# include <time.h>

void sort_stars_mag (SStars *stars, int N) {

# define SWAPFUNC(A,B){ SStars tmp; tmp = stars[A]; stars[A] = stars[B]; stars[B] = tmp; }
# define COMPARE(A,B)(stars[A].mag < stars[B].mag)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

SStars *gstars (char *file, int *NSTARS, Coords *coords, int *NX, int *NY, double *dNdM) {

  char line[64], side[64];
  Header header, theader;
  FILE *f;
  int j, Ninstar, nstar, rnumber, N, Nstars, nbytes;
  SStars *stars;
  char *buffer;
  double X, Y, T1, T2, T3, type, csign;
  double PD, PR, DE, RE;
  double ra, dec, dmag;

  /* read in image header, open image data region */
  if (!gfits_read_header (file, &header)) {
    fprintf (stderr, "ERROR: can't find image file %s (1)\n", file);
    exit(0);
  }
  /* get complete info from header */
  gfits_scan (&header, "NAXIS1", "%d", 1, NX);
  gfits_scan (&header, "NAXIS2", "%d", 1, NY);

  /* attempt to get detailed astrometric information from header */
  if (!strcasecmp (ROUGH_ASTROMETRY, "header")) {
    if (!HEADER[0]) {
      GetCoords (coords, &header);
    } else {
      gfits_read_header (HEADER, &theader);
      GetCoords (coords, &theader);
    }
    if (!strcmp (coords[0].ctype, "NONE") || (coords[0].cdelt1 == 0) ||  (coords[0].cdelt2 == 0)) {
      fprintf (stderr, "header coordinates incomplete, trying for rough coordinates\n");
      strcpy (ROUGH_ASTROMETRY, "config");
    } else {
      if (FLIPX) {
	coords[0].pc1_1 *= -1;
	coords[0].crpix1 = *NX - coords[0].crpix1;
      }
      if (FLIPY) {
	coords[0].pc2_2 *= -1;
	coords[0].crpix2 = *NY - coords[0].crpix2;
      }
      ASEC_PIX = fabs (coords[0].cdelt1 * 3600.0);
      csign = coords[0].cdelt1 / fabs (coords[0].cdelt1);
      CCD_PC1_1 = coords[0].pc1_1 * csign;
      CCD_PC2_1 = coords[0].pc2_1 * csign;
      csign = coords[0].cdelt2 / fabs (coords[0].cdelt2);
      CCD_PC1_2 = coords[0].pc1_2 * csign;
      CCD_PC2_2 = coords[0].pc2_2 * csign;
      if (!strcmp (&coords[0].ctype[4], "-PLY")) {
	strcpy (coords[0].ctype, "DEC--TAN");
      }
    }
  }
  
  /* get just RA & DEC from header, other terms from config file */
  if (!strcasecmp (ROUGH_ASTROMETRY, "config")) {
    /* default values for coords */
    InitCoords (coords, "DEC--TAN");
    coords[0].pc1_1 = CCD_PC1_1; coords[0].pc1_2 = CCD_PC1_2;
    coords[0].pc2_1 = CCD_PC2_1; coords[0].pc2_2 = CCD_PC2_2;
    coords[0].cdelt1 = coords[0].cdelt2 = ASEC_PIX / 3600.0;
    coords[0].crpix1 = 0.5*(*NX);
    coords[0].crpix2 = 0.5*(*NY);

    /* get RA & DEC from header, unless FORCE is ste */
    if (!FORCE) {
      /* RA (in hours, not degrees) */
      if (!gfits_scan (&header, "RA", "%s", 1, line)) {
	fprintf (stderr, "ERROR: no astrometry in header\n");
	exit (1);
      }
      ohana_dms_to_ddd (&coords[0].crval1, line);
      coords[0].crval1 = coords[0].crval1 * 15.0;

      /* DEC */
      if (!gfits_scan (&header, "DEC", "%s", 1, line)) {
	fprintf (stderr, "ERROR: no astrometry in header\n");
	exit (1);
      } 
      ohana_dms_to_ddd (&coords[0].crval2, line);
    }
  }
  if (VERBOSE) fprintf (stderr, "coordinates from header: %9.4f %9.4f\n", coords[0].crval1, coords[0].crval2);

  /* use RA & DEC from command line arguments */
  if (FORCE) {
    coords[0].crval1 = F_RA;
    coords[0].crval2 = F_DEC;
    if (VERBOSE) fprintf (stderr, " forcing coordinates to: %9.4f %9.4f\n", coords[0].crval1, coords[0].crval2);
  }    

  /* the following two sections are LONEOS derived and may not be needed elsewhere */
  if (LONEOS_COORDS) {
    gfits_scan (&header, "COMMENT", "%s", 1, line);
    sscanf (line, "%*s%d%s", &rnumber, side);
    if (get_region_coords (&ra, &dec, rnumber, side)) {
      if (fabs(ra - coords[0].crval1) > 0.1) {
	fprintf (stderr, "large offset from claimed position, using region coords %f %f -> %f %f (%d %s)\n", 
		 coords[0].crval1, coords[0].crval2, ra, dec, rnumber, side);
	coords[0].crval1 = ra;
	coords[0].crval2 = dec;
      }
    }
  }

  /* at this point, we need to correct the crval1, crval2, and ROT_ZERO values
     based on the pole axis angle and the ra, dec offsets */
  if (POLAR_ALIGNMENT) {
    X = coords[0].crval1;
    Y = coords[0].crval2;
    PD = POLE_DEC;  PR = POLE_RA;
    DE = DEC_OFFSET; RE = RA_OFFSET;
    
    T1 = dcos(Y-DE) * dcos(X-RE) * dsin(PD) + dsin(Y-DE) * dcos(PD);
    T2 = dcos(Y-DE) * dsin(X-RE);
    T3 = dsin(Y-DE) * dsin(PD) - dcos(Y-DE) * dcos(X-RE) * dcos(PD);
    
    coords[0].crval1 = (DEG_RAD * atan2 (T2, T1)) + PR;
    coords[0].crval2 = (DEG_RAD * asin (T3));
    coords[0].crval1 = ohana_normalize_angle (coords[0].crval1);
    
    if (VERBOSE) fprintf (stderr, "  after polar alignment: %9.4f %9.4f\n", coords[0].crval1, coords[0].crval2);
  }

  Nstars = 0;
  gfits_scan (&header, "NSTARS", "%d", 1, &Nstars);
  if (Nstars == -1) {
    fprintf (stderr, "ERROR: failed to find NSTARS\n");
    exit (0);
  }
  ALLOCATE (stars, SStars, Nstars);

  /* re-open file for stars */
  f = fopen (file, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't find image file %s (2)\n", file);
    exit(0);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  N = nstar = 0;
  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR));
  
  while ((nbytes = fread (buffer, 1, (BLOCK*BYTES_STAR), f)) != 0) {
    Ninstar = nbytes / BYTES_STAR;
    for (j = 0; j < Ninstar; j++, nstar++) {
      dparse (&stars[N].X,   1, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].Y,   2, &buffer[j*BYTES_STAR]);
      dparse (&stars[N].mag, 3, &buffer[j*BYTES_STAR]);
      dparse (&dmag, 4, &buffer[j*BYTES_STAR]);
      dparse (&type,         5, &buffer[j*BYTES_STAR]);
      if ((type == 4) || (type == 6) || (type == 5) || (type == 9)) continue;
      if (dmag > 100) continue;
      N++;
    }
  }
  free (header.buffer);
  free (buffer);
  fclose (f);
 
  if (nstar != Nstars) {
    fprintf (stderr, "WARNING: only read %d of %d stars\n", nstar, Nstars);
  }
  if (N < 5) { 
    fprintf (stderr, "ERROR: too few stars for reliable solution, only %d\n",
	     N);
    exit (0);
  }

  sort_stars_mag (stars, N);  /* sorting by magnitude */
  Nstars = N;

  if (VERBOSE) fprintf (stderr, "\nread %d stars from data file", Nstars);
  if (*NSTARS < Nstars) {
    REALLOCATE (stars, SStars, *NSTARS);
    if (VERBOSE) fprintf (stderr, ", using %d\n", *NSTARS);
  } else {
    *NSTARS = Nstars;
    if (VERBOSE) fprintf (stderr, "\n");
  }

  *dNdM = *NSTARS / (stars[(*NSTARS-1)].mag - stars[0].mag) ;
  if (VERBOSE) fprintf (stderr, "brightest star in datafile: %f mag\n", stars[0].mag);
  
  return (stars);
}

# if (0)

  /***  this is tailored for LONEOS ***/
  Nccd = -1;
  gfits_scan (&header, "NCCD", "%d", 1, &Nccd);
  if (Nccd == -1) {  /* no ccd info in header, not loneos */
    coords[0].crpix1 = 0.5*(*NX);
    coords[0].crpix2 = 0.5*(*NY);
  }
  if (Nccd == 0) {  /* chip 0 (a) *** might be wrong *** */
    coords[0].crpix1 = 0;
    coords[0].crpix2 = 0.5*(*NY);
  }
  if (Nccd == 1) {  /* chip 1 (b) */
    coords[0].crpix1 = (*NX);
    coords[0].crpix2 = 0.5*(*NY);
  }

# endif
