# include "addstar.h"

int SetSDSSFlags (Measure *measure, unsigned int flags1, unsigned int flags2);

# define NFILTER 5

# define GET_COLUMN_5(NAME,TYPE) \
  TYPE *NAME; \
  NAME = (TYPE *) gfits_get_bintable_column_data (table.header, &table, #NAME, type, &Nrow, &Ncol); \
  assert (NAME); assert (!strcmp (type, #TYPE)); assert (Nrow == Nstars); assert (Ncol == NFILTER);

# define GET_COLUMN_5_NOASSERT(NAME,TYPE) \
  TYPE *NAME; \
  NAME = (TYPE *) gfits_get_bintable_column_data (table.header, &table, #NAME, type, &Nrow, &Ncol);

# define GET_COLUMN_1(NAME,TYPE) \
  TYPE *NAME; \
  NAME = (TYPE *) gfits_get_bintable_column_data (table.header, &table, #NAME, type, &Nrow, &Ncol); \
  assert (NAME); assert (!strcmp (type, #TYPE)); assert (Nrow == Nstars); assert (Ncol == 1);

/* grab named photcode */
# define NAMED_PHOTCODE_AND_ZP(CODE,ZP,NAME) { \
  PhotCode *code; \
  code = GetPhotcodebyName (NAME); \
  if (code == NULL) { \
    fprintf (stderr, "ERROR:  photcode %s not found in photcode table\n", NAME); \
    exit (0); } \
  CODE = code[0].code; \
  ZP = 0.001*code[0].C; }

// XXX NOTE : as of 2008.02.27, the zero point is still carried internally in millimags

// given a file with the pointer at the start of the table block and the 
// corresponding image header, load the stars from the table
Catalog *ReadStarsSDSS (FILE *f, char *name, Header *header, Header *in_theader, Image *images, off_t *nimages) {

  off_t Nskip, Nrow;
  int i, j, N, Nstars, camcol;
  char type[80];
  Header theader;
  FTable table;
  double clockRate, mjd[5], jd, sidtime, alt, az;
  float seeing[5], photErr[5], zeropt[5], ZeroPt;
  time_t tzero[5];
  char filtname[16][5];
  int photcode[5];
  int Ncol; // used in the GET_COLUMN_1,5 macros above
  
  myAbort("need to save the RA,DEC range in a returned SkyRegion (see LoadData)");

  if (in_theader == NULL) {
    table.header = &theader;
    if (!gfits_fread_header (f, table.header)) Shutdown ("ERROR: can't read table header");
  } else {
    table.header = in_theader;
    Nskip = in_theader[0].datasize;
    fseeko (f, Nskip, SEEK_CUR); 
  }

  /* load the table data */
  if (!gfits_fread_ftable_data (f, &table, FALSE)) {
    fprintf (stderr, "ERROR: can't read table header\n");
    exit (1);
  }

  strcpy (filtname[0], "u");
  strcpy (filtname[1], "g");
  strcpy (filtname[2], "r");
  strcpy (filtname[3], "i");
  strcpy (filtname[4], "z");

  NAMED_PHOTCODE_AND_ZP (photcode[0], zeropt[0], "U_SDSS");
  NAMED_PHOTCODE_AND_ZP (photcode[1], zeropt[1], "G_SDSS");
  NAMED_PHOTCODE_AND_ZP (photcode[2], zeropt[2], "R_SDSS");
  NAMED_PHOTCODE_AND_ZP (photcode[3], zeropt[3], "I_SDSS");
  NAMED_PHOTCODE_AND_ZP (photcode[4], zeropt[4], "Z_SDSS");

  // XXXYYYZZZ SDSS tables have special flags for undefined/unmeasured values and errors:
  // -9999 flags unmeasured values, and the corresponding error may or may not be meaningful
  // -1000 flags errors that are not determined, even though the corresponding quantity is.
  // These special values need to be trapped here to avoid averaging meaningful numbers with the flag values.

  // various header values needed to calculate per-star data below
  gfits_scan (header, "C_OBS", "%lf", 1, &clockRate); // value in header is usec / unbinned row
  clockRate *= 1e-6; // convert to seconds / unbinned row

  gfits_scan (table.header, "MJD_U", "%lf", 1, &mjd[0]);
  gfits_scan (table.header, "MJD_G", "%lf", 1, &mjd[1]);
  gfits_scan (table.header, "MJD_R", "%lf", 1, &mjd[2]);
  gfits_scan (table.header, "MJD_I", "%lf", 1, &mjd[3]);
  gfits_scan (table.header, "MJD_Z", "%lf", 1, &mjd[4]);
  tzero[0] = ohana_mjd_to_sec (mjd[0]);
  tzero[1] = ohana_mjd_to_sec (mjd[1]);
  tzero[2] = ohana_mjd_to_sec (mjd[2]);
  tzero[3] = ohana_mjd_to_sec (mjd[3]);
  tzero[4] = ohana_mjd_to_sec (mjd[4]);

  gfits_scan (table.header, "SEEING_U", "%f", 1, &seeing[0]);
  gfits_scan (table.header, "SEEING_G", "%f", 1, &seeing[1]);
  gfits_scan (table.header, "SEEING_R", "%f", 1, &seeing[2]);
  gfits_scan (table.header, "SEEING_I", "%f", 1, &seeing[3]);
  gfits_scan (table.header, "SEEING_Z", "%f", 1, &seeing[4]);

  gfits_scan (table.header, "PSFERR_U", "%f", 1, &photErr[0]);
  gfits_scan (table.header, "PSFERR_G", "%f", 1, &photErr[1]);
  gfits_scan (table.header, "PSFERR_R", "%f", 1, &photErr[2]);
  gfits_scan (table.header, "PSFERR_I", "%f", 1, &photErr[3]);
  gfits_scan (table.header, "PSFERR_Z", "%f", 1, &photErr[4]);

  gfits_scan (header, "CAMCOL", "%d", 1, &camcol); // value in header is usec / unbinned row

  ZeroPt = GetZeroPoint();

  // create a Star entry for each filter and detection
  Nstars = table.header[0].Naxis[1];

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);

  ALLOCATE (catalog->average, Average, Nstars);
  ALLOCATE (catalog->measure, Measure, NFILTER*Nstars);

  for (i = 0; i < Nstars; i++) {
    for (j = 0; j < NFILTER; j++) {
      dvo_measure_init (&catalog->measure[i*NFILTER + j]);
    }
    dvo_average_init (&catalog->average[i]);
  }

  GET_COLUMN_5 (rowc, float);
  GET_COLUMN_5 (colc, float);
  GET_COLUMN_5 (sky, float);
  GET_COLUMN_5 (psfCounts, float);
  GET_COLUMN_5 (fiberCounts, float);
  GET_COLUMN_5 (offsetRa, float);
  GET_COLUMN_5 (offsetDec, float);
  GET_COLUMN_5 (flags, int);
  GET_COLUMN_5 (flags2, int);

#ifdef notyet
  GET_COLUMN_5 (prob_psf, float);
#else
  GET_COLUMN_5_NOASSERT (prob_psf, float);
#endif

  GET_COLUMN_1 (ra, double);
  GET_COLUMN_1 (dec, double);

  GET_COLUMN_5 (rowcErr, float);
  GET_COLUMN_5 (colcErr, float);
  GET_COLUMN_5 (skyErr, float);
  GET_COLUMN_5 (psfCountsErr, float);

  // the value of stars[].M is supposed to be the instrumental magnitude offset by the
  // default zero point 25.0 (-2.5*log_10(counts/sec) + ZeroPt).  The magnitude reported
  // by SDSS is the calibrated mag: -2.5*log_10(counts/sec) + C_0.  Adjust magnitudes to
  // compensate for the difference.

  for (i = 0; i < Nstars; i++) {
    // any values not explicitly set are left at 0.0
    catalog->average[i].R             = ra[i] + dCOS(dec[i]) * offsetRa[i*NFILTER] / 3600.0;
    catalog->average[i].D             = dec[i] + offsetDec[i*NFILTER] / 3600.0;
    catalog->average[i].dR            = NAN;
    catalog->average[i].dD            = NAN;
    catalog->average[i].Nmeasure      = NFILTER;
    catalog->average[i].measureOffset = i*NFILTER;

    for (j = 0; j < NFILTER; j++) {
      N = NFILTER*i + j;
      
      catalog->measure[N].R         = catalog->average[i].R;
      catalog->measure[N].D         = catalog->average[i].D;
      catalog->measure[N].Xccd      = colc[N];
      catalog->measure[N].Yccd      = rowc[N];
      catalog->measure[N].dXccd     = ToShortPixels(colcErr[N]);
      catalog->measure[N].dYccd     = ToShortPixels(rowcErr[N]);
      catalog->measure[N].M         = psfCounts[N] + ZeroPt - zeropt[j];
      catalog->measure[N].dM        = psfCountsErr[N];
      catalog->measure[N].Map       = fiberCounts[N] + ZeroPt - zeropt[j];
      catalog->measure[N].Sky       = sky[N]; // adjust this to counts?
      catalog->measure[N].dSky      = skyErr[N];
      catalog->measure[N].FWx       = ToShortPixels(seeing[j]); // reported in arcsec?
      catalog->measure[N].FWy       = ToShortPixels(seeing[j]);
      if (prob_psf) {
          catalog->measure[N].psfChisq  = prob_psf[N]; // XXX not really the correct value...
      } else {
          catalog->measure[N].psfChisq  = NAN;
      }
      catalog->measure[N].detID     = N;
      catalog->measure[N].t         = tzero[j] + clockRate*rowc[N]; // time since row 0
      catalog->measure[N].dt        = 4.32912209; // 2.5 * log(53.907456) the sdss exposure time // old comment is 53907456 is this 2048*clockRate ?

      SetSDSSFlags (&catalog->measure[N], flags[N], flags2[N]);

      // longitude and latitude for SDSS:
      // Latitude 32° 46' 49.30" N, Longitude 105° 49' 13.50" W
      // longitude = 105.820419312 deg = 7.05469417
      // latitude = 32.7803611755 deg

      double Longitude = 7.05469417;   // hours (+ = W)
      double Latitude = 32.7803611755; // degrees

      jd = ohana_sec_to_jd (catalog->measure[N].t);
      sidtime  = 15.0*ohana_lst (jd, Longitude); // sidtime in degrees
      altaz (&alt, &az, sidtime - catalog->average[i].R, catalog->average[i].D, Latitude);

      catalog->measure[N].airmass   = 1.0 / dCOS(90.0 - alt);
      catalog->measure[N].az        = az;
      catalog->measure[N].photcode  = photcode[j];
      catalog->measure[N].imageID   = j + *nimages; // set imageID to entry for this filter
    }
  }    

  for (i = 0; i < NFILTER; i++) {

    N = i + *nimages;
    
    // XXX for now, we define a totally fake coordinate system centered on the first listed star
    InitCoords (&images[N].coords, "DEC--TAN");
    images[N].coords.crval1 = catalog->average[0].R;
    images[N].coords.crval2 = catalog->average[0].D;
    images[N].coords.crpix1 = catalog->measure[0].Xccd;
    images[N].coords.crpix2 = catalog->measure[0].Yccd;
    images[N].coords.cdelt1 = images[N].coords.cdelt2 = 0.4 / 3600.0;

    images[N].NX = 2048;
    images[N].NY = 1490;

    images[N].tzero = tzero[i];
    images[N].cerror = 0.0;
 
    // set photcodes for the 5 images (SDSS_U,G,R,I,Z)
    images[N].photcode = photcode[i];

    // calculate this from : C_OBS, TRACKING, and NY
    images[N].exptime = 2048*clockRate;
  
    images[N].apmifit = 0.0;
    images[N].dapmifit = 0.0;
    images[N].detection_limit = 0.0; 
    images[N].saturation_limit = 0.0;
    images[N].fwhm_x = seeing[i];
    images[N].fwhm_y = seeing[i];

    // XXX longitude and latitude are known for SDSS
    // SDSS is at : Latitude 32° 46' 49.30" N, Longitude 105° 49' 13.50" W
    // longitude = 105.820419312 deg = 7.05469417
    // latitude = 32.7803611755 deg

    double Longitude = 7.05469417;   // hours (+ = W)
    double Latitude = 32.7803611755; // degrees

    jd = ohana_sec_to_jd (images[N].tzero);
    images[N].sidtime  = ohana_lst (jd, Longitude);
    images[N].latitude = Latitude;

    altaz (&alt, &az, 15.0*images[N].sidtime - images[N].coords.crval1, images[N].coords.crval2, Latitude);

    // secz is in units of airmass
    images[N].trate = clockRate * 1e-4;
    images[N].secz = catalog->measure[0].airmass;
    images[N].ccdnum = camcol;

    images[N].McalPSF   = 0.0;
    images[N].McalAPER  = 0.0;
    images[N].McalChiSq = NAN;
    images[N].dMcal     = NAN;
    images[N].flags     = 0;

    images[N].nstar = Nstars;
  
    images[N].imageID = N;
    images[N].externID = 0;
    images[N].sourceID = 0;
    images[N].parentID = UINT32_MAX; // UpdateImageIDs sets parentID = 0

    // save the filename
    snprintf (images[N].name, DVO_IMAGE_NAME_LEN, "%s[%s]", name, filtname[i]);
  }

  *nimages += NFILTER;
  return (catalog);
}

int SetSDSSFlags (Measure *measure, unsigned int flags1, unsigned int flags2) {

  // XXX this is wrong, need to roll left to set the correct bit 
  if (flags1 & 0x00000002) measure->photFlags |= 0x0001; // BRIGHT            - 1  1
  if (flags1 & 0x00000004) measure->photFlags |= 0x0002; // EDGE              - 1  2
  if (flags1 & 0x00000008) measure->photFlags |= 0x0004; // BLENDED           - 1  3
  if (flags1 & 0x00000010) measure->photFlags |= 0x0008; // CHILD             - 1  4
  if (flags1 & 0x00000020) measure->photFlags |= 0x0010; // PEAKCENTER        - 1  5
  if (flags1 & 0x00000040) measure->photFlags |= 0x0020; // NODEBLEND         - 1  6
  if (flags1 & 0x00040000) measure->photFlags |= 0x0040; // SATUR             - 1 18
  if (flags1 & 0x00080000) measure->photFlags |= 0x0080; // NOTCHECKED        - 1 19
  if (flags1 & 0x10000000) measure->photFlags |= 0x0100; // BINNED1           - 1 28
  if (flags1 & 0x20000000) measure->photFlags |= 0x0200; // BINNED2           - 1 29
  if (flags1 & 0x40000000) measure->photFlags |= 0x0400; // BINNED4           - 1 30
  if (flags2 & 0x00000040) measure->photFlags |= 0x0800; // LOCAL_EDGE        - 2  7
  if (flags2 & 0x00000800) measure->photFlags |= 0x1000; // INTERP_CENTER     - 2 12
  if (flags2 & 0x00002000) measure->photFlags |= 0x2000; // DEBLEND_NOPEAK    - 2 14
  if (flags2 & 0x02000000) measure->photFlags |= 0x4000; // NOTCHECKED_CENTER - 2 26
  return (TRUE);

}
