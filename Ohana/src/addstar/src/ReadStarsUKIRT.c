# include "addstar.h"

# define GET_COLUMN(NAME,TYPE) \
  TYPE *NAME; \
  NAME = (TYPE *) gfits_get_bintable_column_data (table.header, &table, #NAME, type, &Nrow, &Ncol); \
  assert (NAME); assert (!strcmp (type, #TYPE)); assert (Nrow == Nstars); assert (Ncol == 1);

// given a file with the pointer at the start of the table block and the 
// corresponding image header, load the stars from the table
Catalog *ReadStarsUKIRT (FILE *f, char *imagename, Header *header, Image *images, off_t *nimages, SkyRegion *region) {

  off_t Nrow;
  int Ncol;
  char type[80];
  FTable table;
  float RMIN, RMAX, DMIN, DMAX;
  
  // myAbort("need to save the RA,DEC range in a returned SkyRegion (see LoadData)");

  RMIN = region->Rmin;
  RMAX = region->Rmax;
  DMIN = region->Dmin;
  DMAX = region->Dmax;

  // the FITS binary table header is the same as the image header
  table.header = header;

  // put the file pointer at the start of the fits table data
  off_t Nskip = header[0].datasize;
  fseeko (f, Nskip, SEEK_CUR); 

  /* load the table data */
  if (!gfits_fread_ftable_data (f, &table, FALSE)) {
    fprintf (stderr, "ERROR: can't read table data\n");
    exit (1);
  }

  Coords coords;
  if (!GetCoords (&coords, header)) {
    fprintf (stderr, "unable to read header WCS info\n");
    exit (3);
  }

  // longitude and latitude for UKIRT:
  // Latitude = 19.822433, Longitude = -155.470289 W (from google maps)
  // longitude = 155.470289 deg = 10.3646859333 h

  double Longitude = 10.3646859333;	// hours (+ = W)
  double Latitude  = 19.822433;	// degrees

  // DETECTID tells us which chip supplied the data
  char detIDstr[80];
  if (!gfits_scan (header, "DETECTID", "%s", 1, detIDstr)) {
    fprintf (stderr, "missing DETECTID\n");
    exit (3);
  }
  
  int detID = 0;
  if (!strcmp (detIDstr, "RSC:H2:60")) { detID = 1; }
  if (!strcmp (detIDstr, "RSC:H2:63")) { detID = 2; }
  if (!strcmp (detIDstr, "RSC:H2:76")) { detID = 3; }
  if (!strcmp (detIDstr, "RSC:H2:41")) { detID = 4; }
  if (!detID) {
    fprintf (stderr, "unknown DETECTID %s\n", detIDstr);
    exit (3);
  }

  char filter[50];
  if (!gfits_scan (header, "FILTER", "%s", 1, filter)) {
    fprintf (stderr, "missing FILTER\n");
    exit (3);
  }

  int knownFilter = FALSE;
  if (!strcmp (filter, "Z")) { knownFilter = TRUE; }
  if (!strcmp (filter, "Y")) { knownFilter = TRUE; }
  if (!strcmp (filter, "J")) { knownFilter = TRUE; }
  if (!strcmp (filter, "H")) { knownFilter = TRUE; }
  if (!strcmp (filter, "K")) { knownFilter = TRUE; }
  if (!knownFilter) {
    fprintf (stderr, "unknown FILTER %s\n", filter);
    exit (3);
  }
  
  char photcodeName[64];
  snprintf (photcodeName, 64, "UKIRT.%s.%02d", filter, detID);
  PhotCode *code = GetPhotcodebyName (photcodeName);
  if (code == NULL) {
    fprintf (stderr, "ERROR:  photcode %s not found in photcode table\n", photcodeName);
    exit (3); 
  }
  int photcode = code[0].code;

  float zeropt;
  if (!gfits_scan (header, "MAGZPT", "%f", 1, &zeropt)) {
    fprintf (stderr, "ERROR:  missing MAGZPT\n");
    exit (3); 
  }
  // Mrel = Minst + K*(airmass - 1.0) + zp_nom - Mcal
  // Mrel = Minst + K*(airmass - 1.0) + zp
  // zp = zp_nom - Mcal
  // Mcal = zp_nom - zp
  float Mcal = code[0].C*0.001 - zeropt;

  float ApCor4, ApCor7;
  if (!gfits_scan (header, "APCOR4", "%f", 1, &ApCor4)) {
    fprintf (stderr, "ERROR:  missing APCOR4\n");
    exit (3); 
  }
  if (!gfits_scan (header, "APCOR7", "%f", 1, &ApCor7)) {
    fprintf (stderr, "ERROR:  missing APCOR7\n");
    exit (3); 
  }

  // ohana_memcheck (TRUE);

  double mjd;
  gfits_scan (header, "MJD-OBS", "%lf", 1, &mjd);
  time_t tzero = ohana_mjd_to_sec (mjd);
  double jd = ohana_sec_to_jd (tzero);

  double sidtime  = ohana_lst (jd, Longitude); // sidtime in hours
  double sidtime_deg = 15.0*sidtime; // sidtime in degrees

  float seeing;
  gfits_scan (header, "SEEING", "%f", 1, &seeing);

  float exptime;
  gfits_scan (header, "EXP_TIME", "%f", 1, &exptime);
  float magtime = 2.5*log10(exptime);

  float ZeroPt = GetZeroPoint();

  // ohana_memcheck (TRUE);

  // create a Star entry for each filter and detection
  int Nstars = table.header[0].Naxis[1];

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);

  ALLOCATE (catalog->average, Average, Nstars);
  ALLOCATE (catalog->measure, Measure, Nstars);

  int i;
  for (i = 0; i < Nstars; i++) {
    dvo_measure_init (&catalog->measure[i]);
    dvo_average_init (&catalog->average[i]);
  }

  GET_COLUMN (X_coordinate, float);
  GET_COLUMN (Y_coordinate, float);

  GET_COLUMN (X_coordinate_err, float);
  GET_COLUMN (Y_coordinate_err, float);

  GET_COLUMN (Aper_flux_4, float); // 1 arcsec aperture
  GET_COLUMN (Aper_flux_4_err, float);

  GET_COLUMN (Aper_flux_7, float); // 4 arcsec aperture
  GET_COLUMN (Aper_flux_7_err, float);

  GET_COLUMN (Kron_flux, float);
  GET_COLUMN (Kron_flux_err, float);

  GET_COLUMN (Sky_level, float);
  GET_COLUMN (Sky_rms, float);

  double R, D, alt, az;

  // ohana_memcheck (TRUE);

  for (i = 0; i < Nstars; i++) {

    // any values not explicitly set are left at 0.0
    XY_to_RD (&R, &D, X_coordinate[i], Y_coordinate[i], &coords);

    catalog->average[i].R             = R;
    catalog->average[i].D             = D;
    catalog->average[i].dR            = NAN;
    catalog->average[i].dD            = NAN;
    catalog->average[i].Nmeasure      = 1;
    catalog->average[i].measureOffset = i;

    // determine the full coverage of this set of measurements
    RMIN = MIN (RMIN, catalog->average[i].R);
    RMAX = MAX (RMAX, catalog->average[i].R);
    DMIN = MIN (DMIN, catalog->average[i].D);
    DMAX = MAX (DMAX, catalog->average[i].D);

    catalog->measure[i].R         = catalog->average[i].R;
    catalog->measure[i].D         = catalog->average[i].D;
    catalog->measure[i].Xccd      = X_coordinate[i];
    catalog->measure[i].Yccd      = Y_coordinate[i];
    catalog->measure[i].dXccd     = ToShortPixels(X_coordinate_err[i]);
    catalog->measure[i].dYccd     = ToShortPixels(Y_coordinate_err[i]);

    catalog->measure[i].M         = -2.5*log10(Aper_flux_4[i]) + ZeroPt + magtime + ApCor4;
    catalog->measure[i].dM        = Aper_flux_4_err[i] / Aper_flux_4[i];
    catalog->measure[i].Map       = -2.5*log10(Aper_flux_7[i]) + ZeroPt + magtime + ApCor7;
    catalog->measure[i].dMap      = Aper_flux_7_err[i] / Aper_flux_7[i];
    catalog->measure[i].Mkron     = -2.5*log10(Kron_flux[i]) + ZeroPt + magtime;
    catalog->measure[i].dMkron    = Kron_flux_err[i] / Kron_flux[i];
    catalog->measure[i].Sky       = Sky_level[i]; // adjust this to counts?
    catalog->measure[i].dSky      = Sky_rms[i];
    catalog->measure[i].FWx       = ToShortPixels(seeing); // reported in arcsec?
    catalog->measure[i].FWy       = ToShortPixels(seeing);

    catalog->measure[i].McalPSF   = Mcal;
    catalog->measure[i].McalAPER  = Mcal;

    catalog->measure[i].detID     = i;
    catalog->measure[i].t         = tzero; // time since row 0
    catalog->measure[i].dt        = magtime; // 2.5 * log(exptime) 

    altaz (&alt, &az, sidtime_deg - catalog->average[i].R, catalog->average[i].D, Latitude);

    catalog->measure[i].airmass   = 1.0 / dCOS(90.0 - alt);
    catalog->measure[i].az        = az;
    catalog->measure[i].photcode  = photcode;
    catalog->measure[i].imageID   = *nimages; // set imageID to entry for this filter
  }
  catalog->Naverage = Nstars;
  catalog->Nmeasure = Nstars;

  if (1) {
    int N = *nimages;
    
    // ohana_memcheck (TRUE);

    images[N].coords = coords;

    int Nx, Ny;
    gfits_scan (header, "NXOUT", "%d", 1, &Nx);
    gfits_scan (header, "NYOUT", "%d", 1, &Ny);

    images[N].NX = Nx;
    images[N].NY = Ny;
  
    images[N].tzero = tzero;
    images[N].cerror = 0.0;
 
    // set photcodes for the 5 images (SDSS_U,G,R,I,Z)
    images[N].photcode = photcode;

    // calculate this from : C_OBS, TRACKING, and NY
    images[N].exptime = exptime;
  
    images[N].apmifit = 0.0;
    images[N].dapmifit = 0.0;
    images[N].detection_limit = 0.0; 
    images[N].saturation_limit = 0.0;
    images[N].fwhm_x = seeing * 25.0 * images[N].coords.cdelt1 * 3600.0;
    images[N].fwhm_y = seeing * 25.0 * images[N].coords.cdelt1 * 3600.0;

    images[N].sidtime  = sidtime;
    images[N].latitude = Latitude;

    XY_to_RD (&R, &D, 0.5*images[N].NX, 0.5*images[N].NY, &coords);
    altaz (&alt, &az, sidtime_deg - R, D, Latitude);

    images[N].trate = 0.0;
    images[N].secz = 1.0 / sin (RAD_DEG * alt);
    images[N].ccdnum = detID;

    // secz is in units milli-airmass
    images[N].McalPSF   = Mcal;
    images[N].McalAPER  = Mcal;
    images[N].McalChiSq = NAN;
    images[N].dMcal     = NAN;
    images[N].flags     = 0;

    images[N].nstar = Nstars;
  
    images[N].imageID = N;
    images[N].externID = 0;
    images[N].sourceID = 0;
    images[N].parentID = UINT32_MAX; // UpdateImageIDs sets parentID = 0

    // save the filename
    snprintf (images[N].name, DVO_IMAGE_NAME_LEN, "%s[%02d]", imagename, detID);
    *nimages = N + 1;
  }
  
  // ohana_memcheck (TRUE);

  if (VERBOSE) fprintf (stderr, "stars cover region %f,%f - %f,%f\n", RMIN, DMIN, RMAX, DMAX);

  // define containing region a bit generously
  region->Rmin = RMIN;
  region->Rmax = RMAX;
  region->Dmin = DMIN;
  region->Dmax = DMAX;

  return (catalog);
}
