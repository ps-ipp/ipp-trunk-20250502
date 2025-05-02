# include <dvo.h>

/* lock the image table */
int dvo_image_lock (FITS_DB *db, char *filename, double timeout, int lockstate) {

  int READWRITE;

  // default access control options:
  READWRITE = TRUE;

  // in read-only mode, do not backup or require write access
  if (lockstate == LCK_SOFT) {
    READWRITE = FALSE;
  }

  // do not perform a backup here
  if (!check_file_access (filename, FALSE, READWRITE, TRUE)) return (FALSE);

  db[0].lockstate = lockstate;
  db[0].timeout   = timeout;

  if (!gfits_db_lock (db, filename)) {
    fprintf (stderr, "can't lock image catalog\n");
    return (FALSE);
  }
  return (TRUE);
}

int dvo_image_unlock (FITS_DB *db) {

  mode_t mode;

  gfits_db_close (db);

  /* force permissions to 666 */
  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  chmod (db[0].filename, mode);
  return (TRUE);
}

/* determine db mode (RAW/MEF) and read the complete table : raw FITS data is freed by
 * FtableToImage, leaving only the Image structure data */
int dvo_image_load (FITS_DB *db, int VERBOSE, int FORCE_READ) {

  int Naxis, Nread, status;
  char buffer[241];

  /* database name must be set first */
  if (db == NULL) return (FALSE);
  if (db[0].f == NULL) return (FALSE);

  /* test for NAXIS == 2 in line 3 */
  bzero (buffer, 241);
  fseeko (db[0].f, 0, SEEK_SET);
  Nread = fread (buffer, 1, 240, db[0].f);
  if (Nread != 240) {
    if (VERBOSE) fprintf (stderr, "can't determine image db mode\n");
    return (FALSE);
  }
  Nread = sscanf (&buffer[160], "NAXIS   = %d", &Naxis);
  if (Nread != 1) {
    if (VERBOSE) fprintf (stderr, "can't determine image db mode\n");
    return (FALSE);
  }
  fseeko (db[0].f, 0, SEEK_SET);

  /* default values */
  db[0].mode = DVO_MODE_MEF;
  db[0].format = DVO_FORMAT_UNDEF;
  if (Naxis == 2) db[0].mode = DVO_MODE_RAW; /* image table can only be RAW or MEF */

  switch (db[0].mode) {
    case DVO_MODE_MEF:
      if (VERBOSE) fprintf (stderr, "reading images (mode DVO_MODE_MEF)\n");
      status = gfits_db_load (db);
      break;
    case DVO_MODE_RAW:
      if (VERBOSE) fprintf (stderr, "reading images (mode DVO_MODE_RAW)\n");
      status = dvo_image_load_raw (db, VERBOSE, FORCE_READ);
      break;
    default:
      fprintf (stderr, "error getting image mode\n");
      exit (2);
  }
  if (!status) return (FALSE);

  // XXX this is the problem... the conversion (external to internal) is 
  // only allocated Nimage * sizeof(image) not padded to FITS blocks,
  // so when we later do a gfits_copy_ftable, we stomp on bad memory
  FtableToImage (&db[0].ftable, &db[0].theader, &db[0].format);
  db[0].nativeOrder = TRUE;  /* table has internal byte-order */
  db[0].scaledValue = TRUE;  /* table has internal byte-order */
  return (TRUE);
}

/* write the complete image table to disk in the requested mode */
int dvo_image_save (FITS_DB *db, int VERBOSE) {

  int status;

  /* convert from internal to requested external format */
  ImageToFtable (&db[0].ftable, &db[0].theader, db[0].format);
  db[0].nativeOrder = FALSE;
  db[0].scaledValue = FALSE;

  /* write data in appropriate mode */
  switch (db[0].mode) {
    case DVO_MODE_MEF:
    case DVO_MODE_SPLIT:
      status = gfits_db_save (db);
      break;
    case DVO_MODE_RAW:
      status = dvo_image_save_raw (db, VERBOSE);
      break;
    default:
      fprintf (stderr, "error writing image mode\n");
      exit (2);
  }
  return (status);
}

int dvo_image_addrows (FITS_DB *db, Image *new, off_t Nnew) {

  off_t Nimages;

  /* adjust header */
  Nimages = 0;
  if (!gfits_scan (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  &Nimages)) return (FALSE);
  Nimages += Nnew;

  if (!gfits_modify (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  Nimages)) return (FALSE);

  if (!gfits_table_to_vtable (&db[0].ftable, &db[0].vtable, 0, 0)) return (FALSE);

  if (!gfits_vadd_rows (&db[0].vtable, (char *) new, Nnew, sizeof(Image))) return (FALSE);

  /* check that primary header and table header agree */
  if (Nimages != db[0].theader.Naxis[1]) {
    fprintf (stderr, "header / table length mismatch!\n");
    return (FALSE);
  }
  return (TRUE);
}  

/* write the vtable data to disk in the requested mode */
int dvo_image_update (FITS_DB *db, int VERBOSE) {

  int status;

  /* convert from internal to requested external format */
  /* theader is modified to match output format */
  ImageToVtable (&db[0].vtable, &db[0].theader, db[0].format);
  db[0].scaledValue = FALSE;
  db[0].nativeOrder = FALSE;

  /* write data in appropriate mode */
  switch (db[0].mode) {
    case DVO_MODE_MEF:
    case DVO_MODE_SPLIT:
      status = gfits_db_update (db);
      break;
    case DVO_MODE_RAW:
      status = dvo_image_update_raw (db, VERBOSE);
      break;
    default:
      fprintf (stderr, "error writing image mode\n");
      exit (2);
  }
  return (status);
}

void dvo_image_create (FITS_DB *db, double ZeroPoint) {

  /* create new header */
  gfits_init_header (&db[0].header);

  /* check the mode */
  switch (db[0].mode) {
    case DVO_MODE_RAW:
      /* make header a fake image */
      db[0].header.bitpix   = 16;
      db[0].header.Naxes    = 2;
      db[0].header.Naxis[0] = 1;
      db[0].header.Naxis[1] = 1;
      gfits_create_header (&db[0].header);
      break;
    case DVO_MODE_MEF:
    case DVO_MODE_SPLIT:
      db[0].header.extend   = TRUE;
      gfits_create_header (&db[0].header);
      db[0].mode = DVO_MODE_MEF;
      break;
    default:
      fprintf (stderr, "invalid output catalog mode\n");
      exit (1);
  }

  /* check the format */
  if (db[0].format == DVO_FORMAT_UNDEF) {
    fprintf (stderr, "invalid output catalog format\n");
    exit (1);
  }

  gfits_create_matrix (&db[0].header, &db[0].matrix);
  gfits_table_set_Image (&db[0].ftable);

  gfits_modify (&db[0].header, "NIMAGES", "%d", 1, 0);
  gfits_modify (&db[0].header, "ZERO_PT", "%lf", 1, ZeroPoint);
  
  dvo_image_createID (&db[0].header);

  // if (db[0].format == DVO_FORMAT_INTERNAL)  	  gfits_modify (&db[0].header, "FORMAT", "%s", 1, "INTERNAL");
  if (db[0].format == DVO_FORMAT_LONEOS)    	  gfits_modify (&db[0].header, "FORMAT", "%s", 1, "LONEOS");
  if (db[0].format == DVO_FORMAT_ELIXIR)    	  gfits_modify (&db[0].header, "FORMAT", "%s", 1, "ELIXIR");
  if (db[0].format == DVO_FORMAT_PANSTARRS_DEV_0) gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PANSTARRS_DEV_0");
  if (db[0].format == DVO_FORMAT_PANSTARRS_DEV_1) gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PANSTARRS_DEV_1");
  if (db[0].format == DVO_FORMAT_PS1_DEV_1)       gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_DEV_1");
  if (db[0].format == DVO_FORMAT_PS1_DEV_2)       gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_DEV_2");
  if (db[0].format == DVO_FORMAT_PS1_DEV_3)       gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_DEV_3");
  if (db[0].format == DVO_FORMAT_PS1_V1)          gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V1");
  if (db[0].format == DVO_FORMAT_PS1_V2)          gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V2");
  if (db[0].format == DVO_FORMAT_PS1_V3)          gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V3");
  if (db[0].format == DVO_FORMAT_PS1_V4)          gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V4");
  if (db[0].format == DVO_FORMAT_PS1_V5)          gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V5");
  if (db[0].format == DVO_FORMAT_PS1_V6)          gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V6");
  if (db[0].format == DVO_FORMAT_PS1_V5_LOAD)     gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_V5_LOAD");
  if (db[0].format == DVO_FORMAT_PS1_REF)         gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_REF");
  if (db[0].format == DVO_FORMAT_PS1_REF_V2)      gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_REF_V2");
  if (db[0].format == DVO_FORMAT_PS1_REF_V3)      gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_REF_V3");
  if (db[0].format == DVO_FORMAT_PS1_SIM)         gfits_modify (&db[0].header, "FORMAT", "%s", 1, "PS1_SIM");
  
  return;
}

// given a dvo image db, add an ID to the header
int dvo_image_createID (Header *header) {
  
  char dbID[64];

  if (!header->buffer) return FALSE;

  int status = gfits_scan (header, "DVO_DBID", "%s", 1, dbID);      
  if (status) {
    // do not add a DVO_DBID to a table which already has one
    return TRUE;
  }

  // I have a header, I want to add ONE line.  double check there is enough space
  char *p = gfits_header_field (header, "END", 1);
  if (p == NULL) {
    fprintf (stderr, "header is missing END\n");
    return FALSE;
  }
  if (p - header->buffer + 80 == header->datasize) {
    fprintf (stderr, "no free space in block, can't insert keyword\n");
    return FALSE;
  }

  int start_size = header->datasize;

  int PID = getpid();
  long A = PID + time(NULL);
  srand48(A);
  
  int i;
  for (i = 0; i < 32; i++) {
    myAssert (snprintf (&dbID[i], 2, "%1x", (int)(16.0*drand48())) < 2, "overflow");
  }

  gfits_modify (header, "DVO_DBID", "%s", 1, dbID);      
  if (start_size != header->datasize) {
    fprintf (stderr, "error: header buffer grew? (%d to %d bytes)\n", (int) start_size, (int) header->datasize);
    return FALSE;
  }
  return TRUE;
}

int gfits_table_set_Image (FTable *ftable) {

  Header *header;

  header = ftable[0].header;

  gfits_table_mkheader_Image (header);
  
  /* create table */
  if (!gfits_create_table (header, ftable)) return (FALSE);

  return (TRUE);
}

int gfits_table_mkheader_Image (Header *header) {

  /* create table header */
  if (!gfits_create_table_header (header, "BINTABLE", "DVO_IMAGE")) return (FALSE);

  /* define table layout */
  /** TABLE DEFINITION **/
  gfits_define_bintable_column (header, "D",    "CRVAL1",           "coordinate at reference pixel",   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "D",    "CRVAL2",           "coordinate at reference pixel",   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CRPIX1",           "coordinate of reference pixel",   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CRPIX2",           "coordinate of reference pixel",   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CDELT1",           "degrees per pixel",               "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CDELT2",           "degrees per pixel",               "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PC1_1",            "rotation matrix",                 "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PC1_2",            "rotation matrix",                 "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PC2_1",            "rotation matrix",                 "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PC2_2",            "rotation matrix",                 "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "14E",  "POLYTERMS",        "higher order warping terms",      "",                  1.0, 0.0);
  if (sizeof(void *) == 4) {
    gfits_define_bintable_column (header, "J",    "POINTER1",       "",  "",                  1.0, 0.0);
    gfits_define_bintable_column (header, "J",    "POINTER2",       "",  "",                  1.0, 0.0);
  }
  if (sizeof(void *) == 8) {
    gfits_define_bintable_column (header, "K",    "POINTER1",       "",  "",                  1.0, 0.0);
    gfits_define_bintable_column (header, "K",    "POINTER2",       "",  "",                  1.0, 0.0);
  }
  gfits_define_bintable_column (header, "15A",  "CTYPE",            "coordinate type",                 "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "A",    "NPOLYTERMS",       "order of polynomial",             "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "TZERO",            "readout time (row 0)",            "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "NSTAR",            "number of stars on image",        "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "SECZ",             "airmass",                         "mag",               1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "NX",               "image width",                     "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "NY",               "image height",                    "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "APMIFIT",          "aperture correction",             "mag",               1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DAPMIFIT",         "apmifit error",                   "mag",               1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MCAL_PSF",         "calibration mag for psfs",        "mag",               1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MCAL_APER",        "calibration mag for aper",        "mag",               1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DMCAL",            "error on Mcal",                   "mag",               1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "XM",               "image chisq",                     "10*log(value)",     1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "PADDING",          "filler for 8-byte boundaries,",   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "PHOTCODE",         "identifier for CCD,",             "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "EXPTIME",          "exposure time",                   "seconds",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "ST",               "sidereal time of exposure",       "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "LAT",              "observatory latitude",            "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "RA_CENTER",        "image center",                    "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DEC_CENTER",       "image center",                    "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "RADIUS",           "image radius",                    "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "REF_COLOR_BLUE",   "median astrometry ref color",     "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "REF_COLOR_RED",    "median astrometry ref color",     "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "117A", "NAME",             "name of original image ",         "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "DETECTION_LIMIT",  "detection limit",                 "10*mag",            1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "SATURATION_LIMIT", "saturation limit",                "10*mag",            1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "CERROR",           "astrometric error",               "50*arcsec",         1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "FWHM_X",           "PSF x width",                     "25*arcsec",         1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "FWHM_Y",           "PSF y width",                     "25*arcsec",         1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "TRATE",            "scan rate",                       "100 usec/pixel",    1.0, 0.0);
  gfits_define_bintable_column (header, "B",    "CCDNUM",           "CCD ID number",                   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "FLAGS",            "image quality flags",             "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "IMAGE_ID",         "internal image ID",               "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "PARENT_ID",        "associated ref image",            "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "EXTERN_ID",        "external image ID",               "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "SOURCE_ID",        "analysis source ID",              "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "NLINK_ASTROM",     "mean number of matched measurements for astrometry", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "NLINK_PHOTOM",     "mean number of matched measurements for astrometry", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "UBERCAL_DIST",     "distance to nearest ubercal image", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "XPIX_SYS_ERR",     "systematic astrometry error in X", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "YPIX_SYS_ERR",     "systematic astrometry error in Y", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MAG_SYS_ERR",      "systematic photometry error",     "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "N_FIT_ASTROM",     "number of stars used for astrometry cal", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "N_FIT_PHOTOM",     "number of stars used for photometry cal", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "PHOTOM_MAP_ID",    "reference to 2D zero point map",  "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "ASTROM_MAP_ID",    "reference to 2D astrometry map",  "",                  1.0, 0.0);
  if (sizeof(void *) == 4) {
    gfits_define_bintable_column (header, "J",    "POINTER3",       "",  "",                  1.0, 0.0);
  }
  if (sizeof(void *) == 8) {
    gfits_define_bintable_column (header, "K",    "POINTER3",       "",  "",                  1.0, 0.0);
  }

  return (TRUE);
}

/* return internal structure representation */
Image *gfits_table_get_Image (FTable *ftable, off_t *Ndata, char *scaledValue, char *nativeOrder) {

  int Ncols;
  Image *data;

  Ncols = ftable[0].header[0].Naxis[0];
  if (Ncols != sizeof(Image)) {
    fprintf (stderr, "ERROR: mis-match in table size: width is %d but should be %d bytes\n", Ncols, (int) sizeof(Image));
    return NULL;
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  data = (Image *) ftable[0].buffer;

  if (!scaledValue) {
    myAbort ("invalid to call this without suppying 'scaledValue'");
  }
  if (*scaledValue == FALSE) {
    myAbort ("invalid for table NOT to be scaledValue");
  }
  if (!nativeOrder) {
    myAbort ("invalid to call this without suppying 'nativeOrder'");
  }
  if (*nativeOrder == FALSE) {
    myAbort ("invalid for table NOT to be nativeOrder");
  }
    
  return (data);
}

