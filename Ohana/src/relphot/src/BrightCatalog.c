# include "relphot.h"

// relphot_client reads from the local catalogs and generates a set of bright, subset catalogs
// we are going to save these as a single big FITS file, with only 3 extensions:

// measure, average, secfilt.  these only need to contain the fields relevant to the
// MeasureTiny, AverageTiny, and Secfilt tables, but with cat_id appended to each row so we can
// reassign correctly on read

# define GET_COLUMN(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

BrightCatalog *BrightCatalogLoad(char *filename) {

  int i, Ncol;
  off_t Nrow;
  char type[16];
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  header.buffer = NULL;
  matrix.buffer = NULL;
  ftable.buffer = NULL;
  theader.buffer = NULL;
  BrightCatalog *catalog = NULL;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    goto escape;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    goto escape;
  }

  ALLOCATE (catalog, BrightCatalog, 1);
  catalog->Naverage = 0;
  catalog->Nmeasure = 0;

  ftable.header = &theader;

  // *** Measure Data *** 
  { 
    // load data for this header 
    if (!gfits_load_header (f, &theader)) goto escape;

    // read the fits table bytes
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
    // need to create and assign to flat-field correction
    GET_COLUMN(R,         "RA",   	double);
    GET_COLUMN(D,         "DEC",  	double);
    GET_COLUMN(M,         "MAG_SYS",  	float);
    GET_COLUMN(Mkron,     "MAG_KRON",  	float);
    GET_COLUMN(McalPSF,   "MCAL_PSF",  	float);
    GET_COLUMN(McalAPER,  "MCAL_APER", 	float);
    GET_COLUMN(Mflat,     "MAG_FLAT",  	float);
    GET_COLUMN(dM,        "MAG_ERR",  	float);
    GET_COLUMN(airmass,   "AIRMASS",  	float);
    GET_COLUMN(Xccd,      "X_CCD",    	float);
    GET_COLUMN(Yccd,      "Y_CCD",    	float);
    GET_COLUMN(dt,        "EXPTIME",  	float);
    GET_COLUMN(t,         "TIME",     	int);
    GET_COLUMN(averef,    "AVE_REF",  	int); // XXX signed vs unsigned?
    GET_COLUMN(imageID,   "IMAGE_ID", 	int);
    GET_COLUMN(dbFlags,   "DB_FLAGS", 	int);
    GET_COLUMN(photFlags, "PHOT_FLAGS", int);
    GET_COLUMN(catID,     "CAT_ID",     int);
    GET_COLUMN(photcode,  "PHOTCODE",   short);
    gfits_free_header (&theader);
    gfits_free_table  (&ftable);

    MeasureTiny *measure = NULL;
    ALLOCATE (measure, MeasureTiny, Nrow);
    for (i = 0; i < Nrow; i++) {
      measure[i].R         = R[i];
      measure[i].D         = D[i];
      measure[i].M         = M[i];
      measure[i].Mkron     = Mkron[i];
      measure[i].McalPSF   = McalPSF[i];
      measure[i].McalAPER  = McalAPER[i];
      measure[i].Mflat     = Mflat[i];
      measure[i].dM        = dM[i];
      measure[i].airmass   = airmass[i];
      measure[i].Xccd      = Xccd[i];
      measure[i].Yccd      = Yccd[i];
      measure[i].dt        = dt[i];
      measure[i].t         = t[i];
      measure[i].averef    = averef[i];
      measure[i].imageID   = imageID[i];
      measure[i].dbFlags   = dbFlags[i];
      measure[i].photFlags = photFlags[i];
      measure[i].catID     = catID[i];
      measure[i].photcode  = photcode[i];
      measure[i].myDet     = FALSE;
    }
    fprintf (stderr, "loaded data for %lld measure\n", (long long) Nrow);

    free (R       );
    free (D       );
    free (M       );
    free (Mkron   );
    free (McalPSF );
    free (McalAPER);
    free (Mflat   );
    free (dM      );
    free (airmass );
    free (Xccd    );
    free (Yccd    );
    free (dt      );
    free (t       );
    free (averef  );
    free (imageID );
    free (dbFlags );
    free (photFlags );
    free (catID   );
    free (photcode);

    catalog->measure = measure;
    catalog->Nmeasure = Nrow;
  }

  /*** load Average values ***/
  { 

    // load data for this header 
    if (!gfits_load_header (f, &theader)) goto escape;

    // read the fits table bytes
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
    // need to create and assign to flat-field correction
    GET_COLUMN(R,             "RA",   	     double);
    GET_COLUMN(D,             "DEC",  	     double);
    GET_COLUMN(Nmeasure,      "NMEAS",       int);
    GET_COLUMN(measureOffset, "MEASURE_OFF", int);
    GET_COLUMN(flags,         "FLAGS",       int);
    GET_COLUMN(catID,         "CAT_ID",      int);
    GET_COLUMN(objID,         "OBJ_ID",      int);
    gfits_free_header (&theader);
    gfits_free_table  (&ftable);

    AverageTiny *average = NULL;
    ALLOCATE (average, AverageTiny, Nrow);
    for (i = 0; i < Nrow; i++) {
      average[i].R              = R[i];
      average[i].D              = D[i];
      average[i].Nmeasure       = Nmeasure[i];
      average[i].measureOffset  = measureOffset[i];
      average[i].flags          = flags[i];
      average[i].catID          = catID[i];
      average[i].objID          = objID[i];
      average[i].nOwn           = 0;
    }
    fprintf (stderr, "loaded data for %lld average\n", (long long) Nrow);

    free (R             );
    free (D             );
    free (Nmeasure      );
    free (measureOffset );
    free (flags         );
    free (catID         );
    free (objID         );

    catalog->average = average;
    catalog->Naverage = Nrow;
  }

  /*** load Secfilt values ***/
  {

    // load data for this header 
    if (!gfits_load_header (f, &theader)) goto escape;

    // read the fits table bytes
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
    // need to create and assign to flat-field correction
    GET_COLUMN(M,      "MAG",      float);
    GET_COLUMN(Mkron,  "MAG_KRON", float);
    GET_COLUMN(dM,     "MAG_ERR",  float);
    GET_COLUMN(Mchisq, "MAG_CHI",  float);
    GET_COLUMN(flags,  "FLAGS",    int);
    GET_COLUMN(Ncode,  "NCODE",    short);
    GET_COLUMN(Nused,  "NUSED",    short);
    GET_COLUMN(Mmin,   "MAG_MIN",  float);
    GET_COLUMN(Mmax,   "MAG_MAX",  float);
    gfits_free_header (&theader);
    gfits_free_table  (&ftable);

    SecFilt *secfilt = NULL;
    ALLOCATE (secfilt, SecFilt, Nrow);
    for (i = 0; i < Nrow; i++) {
      secfilt[i].MpsfChp  = M[i];         
      secfilt[i].MkronChp = Mkron[i];         
      secfilt[i].dMpsfChp = dM[i];
      secfilt[i].Mchisq   = Mchisq[i];
      secfilt[i].flags    = flags[i];
      secfilt[i].Ncode    = Ncode[i];
      secfilt[i].Nused    = Nused[i];
      secfilt[i].Mmin     = Mmin[i];
      secfilt[i].Mmax     = Mmax[i];
    }
    fprintf (stderr, "loaded data for %lld secfilt\n", (long long) Nrow);

    free (M     );
    free (Mkron );
    free (dM    );
    free (Mchisq);
    free (flags );
    free (Ncode );
    free (Nused );
    free (Mmin  );
    free (Mmax  );
    catalog->secfilt = secfilt;
    // assert Nsecfilt * Naverage = Nrow?
  }

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);

  return catalog;

escape:
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  if (catalog) free (catalog);

  fclose (f);
  return NULL;
}

// we are passed a BrightCatalog structure, write it to a FITS table (3 ext)
int BrightCatalogSave(char *filename, BrightCatalog *catalog) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file for output %s\n", filename);
    return FALSE;
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  /*** MeasureTiny ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "MEASURE_TINY");

    gfits_define_bintable_column (&theader, "D", "RA",       "ra",                         "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC",      "dec",                        "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_SYS",  "magnitude (sys)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_KRON", "magnitude (sys,kron)",        NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MCAL_PSF", "magnitude (cal)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MCAL_APER","magnitude (cal)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_FLAT", "magnitude (flat)",            NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_ERR",  "magnitude (err)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "AIRMASS",  "airmass",                	    NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "X_CCD",    "ccd x coord",            	   "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "Y_CCD",    "ccd y coord",            	   "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "EXPTIME",  "-2.5 * log (exposure time)", "sec",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "TIME",     "time of exp",                "sec",    1.0, FT_BZERO_INT32);
    gfits_define_bintable_column (&theader, "J", "AVE_REF",  "pointer to average table",    NULL,    1.0, FT_BZERO_INT32);
    gfits_define_bintable_column (&theader, "J", "IMAGE_ID", "image",                       NULL,    1.0, FT_BZERO_INT32);
    gfits_define_bintable_column (&theader, "J", "DB_FLAGS", "flags",                       NULL,    1.0, FT_BZERO_INT32);
    gfits_define_bintable_column (&theader, "J", "PHOT_FLAGS", "photflags",                 NULL,    1.0, FT_BZERO_INT32);
    gfits_define_bintable_column (&theader, "J", "CAT_ID",   "catalog",                     NULL,    1.0, FT_BZERO_INT32);
    gfits_define_bintable_column (&theader, "I", "PHOTCODE", "photcode",                    NULL,    1.0, FT_BZERO_INT16);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // create intermediate storage arrays
    double *R         ; ALLOCATE (R        ,  double, catalog->Nmeasure);
    double *D         ; ALLOCATE (D        ,  double, catalog->Nmeasure);
    float  *M         ; ALLOCATE (M        ,  float,  catalog->Nmeasure);
    float  *Mkron     ; ALLOCATE (Mkron    ,  float,  catalog->Nmeasure);
    float  *McalPSF   ; ALLOCATE (McalPSF  ,  float,  catalog->Nmeasure);
    float  *McalAPER  ; ALLOCATE (McalAPER ,  float,  catalog->Nmeasure);
    float  *Mflat     ; ALLOCATE (Mflat    ,  float,  catalog->Nmeasure);
    float  *dM        ; ALLOCATE (dM       ,  float,  catalog->Nmeasure);
    float  *airmass   ; ALLOCATE (airmass  ,  float,  catalog->Nmeasure);
    float  *Xccd      ; ALLOCATE (Xccd     ,  float,  catalog->Nmeasure);
    float  *Yccd      ; ALLOCATE (Yccd     ,  float,  catalog->Nmeasure);
    float  *dt        ; ALLOCATE (dt       ,  float,  catalog->Nmeasure);
    int    *t         ; ALLOCATE (t        ,  int  ,  catalog->Nmeasure);
    int    *averef    ; ALLOCATE (averef   ,  int  ,  catalog->Nmeasure);
    int    *imageID   ; ALLOCATE (imageID  ,  int  ,  catalog->Nmeasure);
    int    *dbFlags   ; ALLOCATE (dbFlags  ,  int  ,  catalog->Nmeasure);
    int    *photFlags ; ALLOCATE (photFlags,  int  ,  catalog->Nmeasure);
    int    *catID     ; ALLOCATE (catID    ,  int  ,  catalog->Nmeasure);
    short  *photcode  ; ALLOCATE (photcode ,  short,  catalog->Nmeasure);

    // assign the storage arrays
    MeasureTiny *measure = catalog->measure;
    for (i = 0; i < catalog->Nmeasure; i++) {
      R[i]        = measure[i].R        ;
      D[i]        = measure[i].D        ;
      M[i]  	  = measure[i].M        ;
      Mkron[i]	  = measure[i].Mkron    ;
      McalPSF[i]  = measure[i].McalPSF  ;
      McalAPER[i] = measure[i].McalAPER ;
      Mflat[i]    = measure[i].Mflat    ;
      dM[i]       = measure[i].dM       ;
      airmass[i]  = measure[i].airmass  ;
      Xccd[i]     = measure[i].Xccd     ;
      Yccd[i]     = measure[i].Yccd     ;
      dt[i]       = measure[i].dt       ;
      t[i] 	  = measure[i].t        ;
      averef[i]   = measure[i].averef   ;
      catID[i]    = measure[i].catID    ;
      imageID[i]  = measure[i].imageID  ;
      dbFlags[i]  = measure[i].dbFlags  ;
      photFlags[i]= measure[i].photFlags;
      photcode[i] = measure[i].photcode ;
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "RA",   	R,         catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "DEC",  	D,         catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MAG_SYS",  	M,         catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MAG_KRON",  	Mkron,     catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MCAL_PSF",  	McalPSF,   catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MCAL_APER",  McalAPER,  catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MAG_FLAT",  	Mflat,     catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MAG_ERR",  	dM,        catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "AIRMASS",  	airmass,   catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "X_CCD",    	Xccd,      catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "Y_CCD",    	Yccd,      catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "EXPTIME",  	dt,        catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "TIME",     	t,         catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "AVE_REF",  	averef,    catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID", 	imageID,   catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "DB_FLAGS", 	dbFlags,   catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "PHOT_FLAGS", photFlags, catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "CAT_ID",     catID,     catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "PHOTCODE",   photcode,  catalog->Nmeasure);

    free (R       );
    free (D       );
    free (M       );
    free (Mkron   );
    free (McalPSF );
    free (McalAPER);
    free (Mflat   );
    free (dM      );
    free (airmass );
    free (Xccd    );
    free (Yccd    );
    free (dt      );
    free (t       );
    free (averef  );
    free (imageID );
    free (dbFlags );
    free (photFlags );
    free (catID   );
    free (photcode);

    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table  (f, &ftable);
    gfits_free_header (&theader);
    gfits_free_table (&ftable);
  }

  /*** AverageTiny ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "AVERAGE_TINY");

    gfits_define_bintable_column (&theader, "D", "RA",   	"ra (J2000)", 	         "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC",  	"dec (J2000)",	         "degree", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "NMEAS",       "number of measures",     NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "MEASURE_OFF", "index to measurements",  NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "FLAGS",       "flags",                  NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "CAT_ID",      "catalog ref",            NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "OBJ_ID",      "object ref",             NULL,    1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // create intermediate storage arrays
    double *R             ; ALLOCATE (R,             double, catalog->Naverage);
    double *D             ; ALLOCATE (D,             double, catalog->Naverage);
    int   *Nmeasure       ; ALLOCATE (Nmeasure,      int,    catalog->Naverage);
    int   *measureOffset  ; ALLOCATE (measureOffset, int,    catalog->Naverage);
    int   *flags          ; ALLOCATE (flags,         int,    catalog->Naverage);
    int   *catID          ; ALLOCATE (catID,         int,    catalog->Naverage);
    int   *objID          ; ALLOCATE (objID,         int,    catalog->Naverage);

    // assign the storage arrays
    AverageTiny *average = catalog->average;
    for (i = 0; i < catalog->Naverage; i++) {
      R[i]           	= average[i].R       ;
      D[i]           	= average[i].D       ;
      Nmeasure[i]    	= average[i].Nmeasure;
      measureOffset[i]  = average[i].measureOffset;
      flags[i]          = average[i].flags;
      catID[i]          = average[i].catID;
      objID[i]          = average[i].objID;
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "RA",          R,             catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "DEC",         D,             catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "NMEAS",       Nmeasure,      catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "MEASURE_OFF", measureOffset, catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "FLAGS",       flags,         catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "CAT_ID",      catID,         catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "OBJ_ID",      objID,         catalog->Naverage);

    free (R             );
    free (D             );
    free (Nmeasure      );
    free (measureOffset );
    free (flags         );
    free (catID         );
    free (objID         );

    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table  (f, &ftable);
    gfits_free_header (&theader);
    gfits_free_table (&ftable);
  }

  /*** SecFilt ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "SECFILT");

    gfits_define_bintable_column (&theader, "E", "MAG",      "",              "mag",   1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_KRON", "",              "mag",   1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_ERR",  "",              "mag",   1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_CHI",  "",              NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "FLAGS",    "",              NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NCODE",    "",              NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NUSED",    "",              NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_MIN",  "min valid mag", "mag",   1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_MAX",  "max valid mag", "mag",   1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // Nsecfilt is number of average filters, Nsec is total number of values
    int Nsecfilt = GetPhotcodeNsecfilt();
    int Nsec = Nsecfilt * catalog->Naverage;

    // create intermediate storage arrays
    float *M        ; ALLOCATE (M      ,  float, Nsec);
    float *Mkron    ; ALLOCATE (Mkron  ,  float, Nsec);
    float *dM       ; ALLOCATE (dM     ,  float, Nsec);
    float *Mchisq   ; ALLOCATE (Mchisq ,  float, Nsec);
    int   *flags    ; ALLOCATE (flags  ,  int,   Nsec);
    short *Ncode    ; ALLOCATE (Ncode  ,  short, Nsec);
    short *Nused    ; ALLOCATE (Nused  ,  short, Nsec);
    float *Mmin     ; ALLOCATE (Mmin   ,  float, Nsec);
    float *Mmax     ; ALLOCATE (Mmax   ,  float, Nsec);

    // assign the storage arrays
    SecFilt *secfilt = catalog->secfilt;
    for (i = 0; i < Nsec; i++) {
      M     [i]       = secfilt[i].MpsfChp ;
      Mkron [i]       = secfilt[i].MkronChp;
      dM    [i]       = secfilt[i].dMpsfChp;
      Mchisq[i]       = secfilt[i].Mchisq  ;
      flags [i]       = secfilt[i].flags   ;
      Ncode [i]       = secfilt[i].Ncode   ;
      Nused [i]       = secfilt[i].Nused   ;
      Mmin  [i]       = secfilt[i].Mmin    ;
      Mmax  [i]       = secfilt[i].Mmax    ;
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "MAG",      M     , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_KRON", Mkron , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_ERR",  dM    , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_CHI",  Mchisq, Nsec);
    gfits_set_bintable_column (&theader, &ftable, "FLAGS",    flags , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "NCODE",    Ncode , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "NUSED",    Nused , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_MIN",  Mmin  , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_MAX",  Mmax  , Nsec);

    free (M      );
    free (Mkron  );
    free (dM     );
    free (Mchisq );
    free (flags  );
    free (Ncode  );
    free (Nused  );
    free (Mmin   );
    free (Mmax   );

    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table  (f, &ftable);
    gfits_free_header (&theader);
    gfits_free_table (&ftable);
  }
  return TRUE;
}

// merge a list of catalogs into a single BrightCatalog array set
BrightCatalog *BrightCatalogMerge (Catalog *catalog, int Ncatalog) {

  off_t i, j, k;

  BrightCatalog *bcatalog = NULL;
  ALLOCATE (bcatalog, BrightCatalog, 1);
  
  int Nmeas = 0;
  int Naves = 0;
  for (i = 0; i < Ncatalog; i++) {
    Nmeas += catalog[i].Nmeasure;
    Naves += catalog[i].Naverage;
  }
    
  // XXX this prevents different catalogs from having different Nsecfilt values
  int Nsecfilt = GetPhotcodeNsecfilt();

  ALLOCATE (bcatalog[0].measure, MeasureTiny, Nmeas);
  ALLOCATE (bcatalog[0].average, AverageTiny, Naves);
  ALLOCATE (bcatalog[0].secfilt, SecFilt, Naves*Nsecfilt);

  int Nm = 0;
  int Na = 0;
  for (i = 0; i < Ncatalog; i++) {
    if (!catalog[i].Naverage) continue;
    for (j = 0; j < catalog[i].Naverage; j++) {
      bcatalog[0].average[Na] = catalog[i].averageT[j];
      for (k = 0; k < Nsecfilt; k++) {
	bcatalog[0].secfilt[Nsecfilt*Na + k] = catalog[i].secfilt[Nsecfilt*j + k];
      }
      Na++;
      assert (Na <= Naves);
    }
    for (j = 0; j < catalog[i].Nmeasure; j++) {
      bcatalog[0].measure[Nm] = catalog[i].measureT[j];
      Nm++;
      assert (Nm <= Nmeas);
    }
  }
  bcatalog->Naverage = Na;
  bcatalog->Nmeasure = Nm;
  return bcatalog;
}

void BrightCatalogFree (BrightCatalog *bcatalog) {
  if (!bcatalog) return;
  FREE (bcatalog[0].average);
  FREE (bcatalog[0].measure);
  FREE (bcatalog[0].secfilt);
  FREE (bcatalog);
  return;
}

// distribute a bright catalog across separate catalogs
CatalogSplitter *BrightCatalogSplitInit (int Nsecfilt) {

  int i;

  CatalogSplitter *catalogs = NULL;

  ALLOCATE (catalogs, CatalogSplitter, 1);

  // as we see new BrightCatalogs, we will update maxID and extend this array
  catalogs->maxID = 0;
  ALLOCATE (catalogs->index, int, catalogs->maxID + 1);
  for (i = 0; i <= catalogs->maxID; i++) catalogs->index[i] = -1;

  catalogs->Nsecfilt = Nsecfilt;

  catalogs->Ncatalog =  0;
  catalogs->NCATALOG = 16;
  catalogs->catalog = NULL;
  ALLOCATE (catalogs->catalog, Catalog, catalogs->NCATALOG);

  ALLOCATE (catalogs->catIDs,   int,   catalogs->NCATALOG);
  ALLOCATE (catalogs->NAVERAGE, off_t, catalogs->NCATALOG);
  ALLOCATE (catalogs->NMEASURE, off_t, catalogs->NCATALOG);

  for (i = 0; i < catalogs->NCATALOG; i++) {
    dvo_catalog_init (&catalogs->catalog[i], TRUE);
    catalogs->catIDs[i] = 0;
    catalogs->NAVERAGE[i] = 256;
    catalogs->NMEASURE[i] = 256;
    catalogs->catalog[i].Naverage = 0;
    catalogs->catalog[i].Nmeasure = 0;
    ALLOCATE (catalogs->catalog[i].averageT, AverageTiny, catalogs->NAVERAGE[i]);
    ALLOCATE (catalogs->catalog[i].measureT, MeasureTiny, catalogs->NMEASURE[i]);
    ALLOCATE (catalogs->catalog[i].secfilt,  SecFilt,     catalogs->NAVERAGE[i]*Nsecfilt);
  }
  return catalogs;
}

// distribute a bright catalog across separate catalogs
int BrightCatalogSplitFree (CatalogSplitter *catalogs) {

  // don't free the catalogs : free elsewhere
  free (catalogs->catIDs);
  free (catalogs->NAVERAGE);
  free (catalogs->NMEASURE);
  free (catalogs->index);
  free (catalogs);
  return TRUE;
}

// distribute a bright catalog across separate catalogs
int BrightCatalogSplit (CatalogSplitter *catalogs, BrightCatalog *bcatalog) {

  int i;

  int D_NCATALOG = 16;

  int Nsecfilt = catalogs->Nsecfilt;

  // find the max value of catID in this BrightCatalog
  int catIDmax = 0;
  for (i = 0; i < bcatalog->Naverage; i++) {
    catIDmax = MAX(catIDmax, bcatalog->average[i].catID);
  }
    
  int maxIDold = catalogs->maxID;
  catalogs->maxID = MAX (maxIDold, catIDmax);
    
  // extend the index array and init
  REALLOCATE (catalogs->index, int, catalogs->maxID + 1);
  for (i = maxIDold + 1; i <= catalogs->maxID; i++) catalogs->index[i] = -1;

  // identify the new catID values
  for (i = 0; i < bcatalog->Naverage; i++) {
    int catID = bcatalog->average[i].catID;
    assert (catID > 0);
    assert (catID < catalogs->maxID + 1);
    int idx = catalogs->index[catID];
    if (idx != -1) continue; // already have seen this one

    // a new catID value
    int Ncat = catalogs->Ncatalog; // the next available slot
    catalogs->catIDs[Ncat] = catID;
    assert (Ncat >= 0);
    assert (Ncat < catalogs->NCATALOG);

    catalogs->catalog[Ncat].Nsecfilt = Nsecfilt;
    catalogs->catalog[Ncat].catID = catID;

    catalogs->index[catID] = Ncat;
    assert (catID > 0);
    assert (catID < catalogs->maxID + 1);

    catalogs->Ncatalog ++;

    if (catalogs->Ncatalog >= catalogs->NCATALOG) {
      catalogs->NCATALOG += D_NCATALOG;

      // fprintf (stderr, "realloc catalogs->catalog: old: %llx  ", (long long) catalogs->catalog);
      REALLOCATE (catalogs->catalog, Catalog, catalogs->NCATALOG);
      // fprintf (stderr, "new: %llx  -  %llx\n", (long long) catalogs->catalog, (long long) (catalogs->catalog + sizeof(Catalog)*catalogs->NCATALOG));

      // fprintf (stderr, "realloc catalogs->NAVERAGE: old: %llx  ", (long long) catalogs->NAVERAGE);
      REALLOCATE (catalogs->NAVERAGE, off_t,  catalogs->NCATALOG);
      // fprintf (stderr, "new: %llx  -  %llx\n", (long long) catalogs->NAVERAGE, (long long) (catalogs->NAVERAGE + sizeof(off_t)*catalogs->NCATALOG));

      // fprintf (stderr, "realloc catalogs->NMEASURE: old: %llx  ", (long long) catalogs->NMEASURE);
      REALLOCATE (catalogs->NMEASURE, off_t,  catalogs->NCATALOG);
      // fprintf (stderr, "new: %llx  -  %llx\n", (long long) catalogs->NMEASURE, (long long) (catalogs->NMEASURE + sizeof(off_t)*catalogs->NCATALOG));

      REALLOCATE (catalogs->catIDs,   int,   catalogs->NCATALOG);

      int j;
      for (j = catalogs->NCATALOG - D_NCATALOG; j < catalogs->NCATALOG; j++) {
	dvo_catalog_init (&catalogs->catalog[j], TRUE);
	catalogs->catIDs[j] = 0;
	catalogs->NAVERAGE[j] = 256;
	catalogs->NMEASURE[j] = 256;
	catalogs->catalog[j].Naverage = 0;
	catalogs->catalog[j].Nmeasure = 0;
	ALLOCATE (catalogs->catalog[j].averageT, AverageTiny, catalogs->NAVERAGE[j]);
	ALLOCATE (catalogs->catalog[j].measureT, MeasureTiny, catalogs->NMEASURE[j]);
	ALLOCATE (catalogs->catalog[j].secfilt,  SecFilt,     catalogs->NAVERAGE[j]*Nsecfilt);
      }
      D_NCATALOG = MAX(2000, 2*D_NCATALOG);
    }
  }

  // each bcatalog (from each host) has a different set of catID ranges
  // they also (probably) have contiguous ranges.  But, it is a bit tricky to be sure I
  // can use that info, so perhaps ignore it

  // assign the averages to the corresponding catalog
  for (i = 0; i < bcatalog->Naverage; i++) {
    int ID = bcatalog->average[i].catID;
    int Nc = catalogs->index[ID];
    assert (Nc > -1);
    assert (Nc < catalogs->NCATALOG);

    int Na = catalogs->catalog[Nc].Naverage;
    catalogs->catalog[Nc].averageT[Na] = bcatalog->average[i];

    // secfilt entries are grouped in blocks for each average
    int k;
    for (k = 0; k < Nsecfilt; k++) {
      catalogs->catalog[Nc].secfilt[Na*Nsecfilt + k] = bcatalog->secfilt[i*Nsecfilt + k];
    }      

    catalogs->catalog[Nc].Naverage ++;
    if (catalogs->catalog[Nc].Naverage >= catalogs->NAVERAGE[Nc]) {
      catalogs->NAVERAGE[Nc] += MAX(catalogs->NAVERAGE[Nc], 1024);
      REALLOCATE (catalogs->catalog[Nc].averageT, AverageTiny, catalogs->NAVERAGE[Nc]);
      REALLOCATE (catalogs->catalog[Nc].secfilt,  SecFilt,     catalogs->NAVERAGE[Nc]*Nsecfilt);
    }	       
  }

  // assign the measures to the corresponding catalog
  for (i = 0; i < bcatalog->Nmeasure; i++) {
    int ID = bcatalog->measure[i].catID;
    int Nc = catalogs->index[ID];
    assert (Nc > -1);
    assert (Nc < catalogs->NCATALOG);
    int Na = catalogs->catalog[Nc].Nmeasure;
    catalogs->catalog[Nc].measureT[Na] = bcatalog->measure[i];
    catalogs->catalog[Nc].Nmeasure ++;
    if (catalogs->catalog[Nc].Nmeasure >= catalogs->NMEASURE[Nc]) {
      catalogs->NMEASURE[Nc] += MAX(catalogs->NMEASURE[Nc], 4096);
      REALLOCATE (catalogs->catalog[Nc].measureT, MeasureTiny, catalogs->NMEASURE[Nc]);
    }	       
  }

  return TRUE;
}
