# include "relastro.h"

// NOTE: this file is nearly identical to relphot/src/BrightCatalog.c, but here we use Average, not AverageTiny

// relastro_client -load reads from the local catalogs and generates a set of bright, subset catalogs
// we are going to save these as a single big FITS file, with only 3 extensions:

// measure, average, secfilt.  these only need to contain the fields relevant to the
// MeasureTiny, Average, and Secfilt tables, but with cat_id appended to each row so we can
// reassign correctly on read

# define GET_COLUMN(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// XXX double check the header extname values for each table
// XXX make sure we free things as we can
// XXX make sure we close files as we can
// XXX handle free and close on error return as well
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
    goto escape;
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
    GET_COLUMN(McalPSF,   "MCAL_PSF",  	float);
    GET_COLUMN(McalAPER,  "MCAL_APER", 	float);
    GET_COLUMN(dM,        "MAG_ERR",  	float);
    GET_COLUMN(airmass,   "AIRMASS",  	float);
    GET_COLUMN(Xccd,      "X_CCD",    	float);
    GET_COLUMN(Yccd,      "Y_CCD",    	float);
    GET_COLUMN(Xfix,      "X_FIX",    	float);
    GET_COLUMN(Yfix,      "Y_FIX",    	float);
    GET_COLUMN(dt,        "EXPTIME",  	float);
    GET_COLUMN(psfQF,     "PSF_QF",  	float);
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
      memset (&measure[i], 0, sizeof(MeasureTiny));
      measure[i].R         = R[i];
      measure[i].D         = D[i];
      measure[i].M         = M[i];
      measure[i].McalPSF   = McalPSF[i];
      measure[i].McalAPER  = McalAPER[i];
      measure[i].dM        = dM[i];
      measure[i].airmass   = airmass[i];
      measure[i].Xccd      = Xccd[i];
      measure[i].Yccd      = Yccd[i];
      measure[i].Xfix      = Xfix[i];
      measure[i].Yfix      = Yfix[i];
      measure[i].dt        = dt[i];
      measure[i].psfQF     = psfQF[i];
      measure[i].t         = t[i];
      measure[i].averef    = averef[i];
      measure[i].imageID   = imageID[i];
      measure[i].dbFlags   = dbFlags[i];
      measure[i].photFlags = photFlags[i];
      measure[i].catID     = catID[i];
      measure[i].photcode  = photcode[i];
      measure[i].dXccd     = 0.0;
      measure[i].dYccd     = 0.0;
      measure[i].dRsys     = 0.0;
      measure[i].myDet     = 0;
    }
    // fprintf (stderr, "loaded data for %lld measures\n", (long long) Nrow);

    free (R       );
    free (D       );
    free (M       );
    free (McalPSF );
    free (McalAPER);
    free (dM      );
    free (airmass );
    free (Xccd    );
    free (Yccd    );
    free (Xfix    );
    free (Yfix    );
    free (dt      );
    free (psfQF   );
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

    GET_COLUMN(R,              "RA",             double);
    GET_COLUMN(D,              "DEC",            double);
    GET_COLUMN(dR,             "RA_ERR",         float);
    GET_COLUMN(dD,             "DEC_ERR",        float);
    GET_COLUMN(uR,             "U_RA",           float);
    GET_COLUMN(uD,             "U_DEC",          float);
    GET_COLUMN(duR,            "U_RA_ERR",       float);
    GET_COLUMN(duD,            "U_DEC_ERR",      float);
    GET_COLUMN(uRgal,          "U_RA_GAL",       float);
    GET_COLUMN(uDgal,          "U_DEC_GAL",      float);
    GET_COLUMN(P,              "PAR",            float);
    GET_COLUMN(dP,             "PAR_ERR",        float);
    GET_COLUMN(ChiSqAve,       "CHISQ_POS",      float);
    GET_COLUMN(ChiSqPM,        "CHISQ_PM",       float);
    GET_COLUMN(ChiSqPar,       "CHISQ_PAP",      float);
    GET_COLUMN(Tmean,          "MEAN_EPOCH",     int);
    GET_COLUMN(Trange,         "TIME_RANGE",     int);
    GET_COLUMN(stargal,        "STARGAL_SEP",    float);
    GET_COLUMN(Npos,           "NUMBER_POS",     short);
    GET_COLUMN(Nmeasure,       "NMEASURE",       short);
    GET_COLUMN(Nmissing,       "NMISSING",       short);
    GET_COLUMN(Ngalphot,      "NGALPHOT",      short);
    GET_COLUMN(measureOffset,  "OFF_MEASURE",    int);
    GET_COLUMN(missingOffset,  "OFF_MISSING",    int);
    GET_COLUMN(refColorBlue,   "REF_COLOR_BLUE", float);
    GET_COLUMN(refColorRed,    "REF_COLOR_RED",  float);
    GET_COLUMN(flags,          "FLAGS",          int);
    GET_COLUMN(photFlagsUpper, "PHOTFLAGS_U",    int);
    GET_COLUMN(photFlagsLower, "PHOTFLAGS_L",    int);
    GET_COLUMN(objID,          "OBJ_ID",         int);
    GET_COLUMN(catID,          "CAT_ID",         int);
    GET_COLUMN(extID,          "EXT_ID",         int64_t);
    gfits_free_header (&theader);
    gfits_free_table  (&ftable);

    Average *average = NULL;
    ALLOCATE (average, Average, Nrow);
    for (i = 0; i < Nrow; i++) {
      dvo_average_init (&average[i]);
      average[i].R               = R[i]               ;
      average[i].D               = D[i]               ;
      average[i].dR              = dR[i]              ;
      average[i].dD              = dD[i]              ;
      average[i].uR              = uR[i]              ;
      average[i].uD              = uD[i]              ;
      average[i].duR             = duR[i]             ;
      average[i].duD             = duD[i]             ;
      average[i].uRgal           = uRgal[i]           ;
      average[i].uDgal           = uDgal[i]           ;
      average[i].P               = P[i]               ;
      average[i].dP              = dP[i]              ;
      average[i].ChiSqAve        = ChiSqAve[i]        ;
      average[i].ChiSqPM         = ChiSqPM[i]         ;
      average[i].ChiSqPar        = ChiSqPar[i]        ;
      average[i].Tmean           = Tmean[i]           ;
      average[i].Trange          = Trange[i]          ;
      average[i].stargal         = stargal[i]         ;
      average[i].Npos            = Npos[i]            ;
      average[i].Nmeasure        = Nmeasure[i]        ;
      average[i].Nmissing        = Nmissing[i]        ;
      average[i].Ngalphot       = Ngalphot[i]         ;
      average[i].measureOffset   = measureOffset[i]   ; 
      average[i].missingOffset   = missingOffset[i]   ; 
      average[i].refColorBlue    = refColorBlue[i]    ;  
      average[i].refColorRed     = refColorRed[i]     ;  
      average[i].flags           = flags[i]           ;
      average[i].photFlagsUpper  = photFlagsUpper[i]  ;
      average[i].photFlagsLower  = photFlagsLower[i]  ;
      average[i].objID           = objID[i]           ;
      average[i].catID           = catID[i]           ;
      average[i].extID           = extID[i]           ;
    }
    fprintf (stderr, "loaded data for %lld averages\n", (long long) Nrow);

    free (R);
    free (D);
    free (dR);
    free (dD);
    free (uR);
    free (uD);
    free (duR);
    free (duD);
    free (uRgal);
    free (uDgal);
    free (P);
    free (dP);
    free (ChiSqAve);
    free (ChiSqPM);
    free (ChiSqPar);
    free (Tmean);
    free (Trange);
    free (stargal);
    free (Npos);
    free (Nmeasure);
    free (Nmissing);
    free (Ngalphot);
    free (measureOffset);
    free (missingOffset);
    free (refColorBlue);
    free (refColorRed);
    free (flags);
    free (photFlagsUpper);
    free (photFlagsLower);
    free (objID);
    free (catID);
    free (extID);

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
      secfilt[i].dMpsfChp = dM[i];
      secfilt[i].Mchisq   = Mchisq[i];
      secfilt[i].flags    = flags[i];
      secfilt[i].Ncode    = Ncode[i];
      secfilt[i].Nused    = Nused[i];
      secfilt[i].Mmin     = Mmin[i];
      secfilt[i].Mmax     = Mmax[i];
    }
    // fprintf (stderr, "loaded data for %lld secfilt\n", (long long) Nrow);

    free (M    );
    free (dM   );
    free (Mchisq);
    free (flags);
    free (Ncode);
    free (Nused);
    free (Mmin );
    free (Mmax );
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

  if (f) fclose (f);
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
    // ohana_memcheck (1);
    gfits_create_table_header (&theader, "BINTABLE", "MEASURE_TINY");

    // ohana_memcheck (1);

    gfits_define_bintable_column (&theader, "D", "RA",       "ra",                         "degrees", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC",      "dec",                        "degrees", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_SYS",  "magnitude (sys)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MCAL_PSF", "magnitude (cal)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MCAL_APER","magnitude (cal)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_ERR",  "magnitude (err)",             NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "AIRMASS",  "airmass",                	    NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "X_CCD",    "ccd x raw coord",            "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "Y_CCD",    "ccd y raw coord",            "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "X_FIX",    "ccd x fixed coord",          "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "Y_FIX",    "ccd y fiex coord",       	   "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "EXPTIME",  "-2.5 * log (exposure time)", "sec",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "PSF_QF",   "psf quality factor",          NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "TIME",     "time of exp",                "sec",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "AVE_REF",  "pointer to average table",    NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "IMAGE_ID", "image",                       NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "DB_FLAGS", "flags",                       NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "PHOT_FLAGS", "photflags",                 NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "CAT_ID",   "catalog",                     NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "PHOTCODE", "photcode",                    NULL,    1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // create intermediate storage arrays
    double *R         ; ALLOCATE (R        ,  double, catalog->Nmeasure);
    double *D         ; ALLOCATE (D        ,  double, catalog->Nmeasure);
    float  *M         ; ALLOCATE (M        ,  float,  catalog->Nmeasure);
    float  *McalPSF   ; ALLOCATE (McalPSF  ,  float,  catalog->Nmeasure);
    float  *McalAPER  ; ALLOCATE (McalAPER ,  float,  catalog->Nmeasure);
    float  *dM        ; ALLOCATE (dM       ,  float,  catalog->Nmeasure);
    float  *airmass   ; ALLOCATE (airmass  ,  float,  catalog->Nmeasure);
    float  *Xccd      ; ALLOCATE (Xccd     ,  float,  catalog->Nmeasure);
    float  *Yccd      ; ALLOCATE (Yccd     ,  float,  catalog->Nmeasure);
    float  *Xfix      ; ALLOCATE (Xfix     ,  float,  catalog->Nmeasure);
    float  *Yfix      ; ALLOCATE (Yfix     ,  float,  catalog->Nmeasure);
    float  *dt        ; ALLOCATE (dt       ,  float,  catalog->Nmeasure);
    float  *psfQF     ; ALLOCATE (psfQF    ,  float,  catalog->Nmeasure);
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
      McalPSF[i]  = measure[i].McalPSF  ;
      McalAPER[i] = measure[i].McalAPER ;
      dM[i]       = measure[i].dM       ;
      airmass[i]  = measure[i].airmass  ;
      Xccd[i]     = measure[i].Xccd     ;
      Yccd[i]     = measure[i].Yccd     ;
      Xfix[i]     = measure[i].Xfix     ;
      Yfix[i]     = measure[i].Yfix     ;
      dt[i]       = measure[i].dt       ;
      psfQF[i]    = measure[i].psfQF    ;
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

    // fprintf (stderr, "--------------- after set_bintable RA --------------");
    // ohana_memdump_file (stderr, TRUE);
    
    gfits_set_bintable_column (&theader, &ftable, "DEC",  	D,         catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MAG_SYS",  	M,         catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MCAL_PSF",  	McalPSF,   catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MCAL_APER", 	McalAPER,  catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "MAG_ERR",  	dM,        catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "AIRMASS",  	airmass,   catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "X_CCD",    	Xccd,      catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "Y_CCD",    	Yccd,      catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "X_FIX",    	Xfix,      catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "Y_FIX",    	Yfix,      catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "EXPTIME",  	dt,        catalog->Nmeasure);
    gfits_set_bintable_column (&theader, &ftable, "PSF_QF",  	psfQF,     catalog->Nmeasure);
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
    free (McalPSF );
    free (McalAPER);
    free (dM      );
    free (airmass );
    free (Xccd    );
    free (Yccd    );
    free (Xfix    );
    free (Yfix    );
    free (dt      );
    free (psfQF   );
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

  /*** Average ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "AVERAGE");

    gfits_define_bintable_column (&theader, "D", "RA",             "RA", 	         	  	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "D", "DEC",            "DEC", 	         	 	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "RA_ERR",         "RA error", 	         	         		  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "DEC_ERR",        "DEC error", 	         	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "U_RA",           "RA*cos(D) proper-motion", 	         		  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "U_DEC",          "DEC proper-motion", 	         	 	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "U_RA_ERR",       "RA*cos(D) p-m error", 	         	 	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "U_DEC_ERR",      "DEC p-m error", 	         	 		  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "U_RA_GAL",       "RA*cos(D) p-m error", 	         	 	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "U_DEC_GAL",      "DEC p-m error", 	         	 		  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "PAR",            "parallax", 	                         		  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "PAR_ERR",        "parallax error", 	                 		  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "CHISQ_POS",      "astrometry analysis chisq", 	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "CHISQ_PM",       "astrometry analysis chisq", 	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "CHISQ_PAP",      "astrometry analysis chisq", 	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "MEAN_EPOCH",     "mean epoch (PM-PAR ref)", 	         		  "", 1.0, 0.00);
    gfits_define_bintable_column (&theader, "J", "TIME_RANGE",     "mean epoch (PM-PAR ref)", 	         		  "", 1.0, 0.00);
    gfits_define_bintable_column (&theader, "E", "STARGAL_SEP",    "star/galaxy separator",                		  "", 1.0, 0.0);   
    gfits_define_bintable_column (&theader, "I", "NUMBER_POS",     "number of detections used for astrometry", 	          "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NMEASURE",       "number of psf measurements", 	                  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NMISSING",       "number of missings", 	          	          "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NGALPHOT",      "number of galaxy shape measurements",                 "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "OFF_MEASURE",    "offset to first psf measurement", 	 	          "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "OFF_MISSING",    "offset to first missing obs", 	         	  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "REF_COLOR_BLUE", "reference color", 	                                  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "REF_COLOR_RED",  "reference color", 	                                  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "FLAGS",          "average object flags (star; ghost; etc)", 	          "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "PHOTFLAGS_U",    "upper bit of 2 bit summary of per-measure photflags", "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "PHOTFLAGS_L",    "lower bit of 2 bit summary of per-measure photflags", "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "OBJ_ID",         "unique ID for object in table", 	                  "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "CAT_ID",         "unique ID for table in which object was first realized", "", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "K", "EXT_ID",         "external ID for object (eg PSPS objID)", 	          "", 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // create intermediate storage arrays
    double   *R             ; ALLOCATE (R             , double  , catalog->Naverage);
    double   *D             ; ALLOCATE (D             , double  , catalog->Naverage);
    float    *dR            ; ALLOCATE (dR            , float   , catalog->Naverage);
    float    *dD            ; ALLOCATE (dD            , float   , catalog->Naverage);
    float    *uR            ; ALLOCATE (uR            , float   , catalog->Naverage);
    float    *uD            ; ALLOCATE (uD            , float   , catalog->Naverage);
    float    *duR           ; ALLOCATE (duR           , float   , catalog->Naverage);
    float    *duD           ; ALLOCATE (duD           , float   , catalog->Naverage);
    float    *uRgal         ; ALLOCATE (uRgal         , float   , catalog->Naverage);
    float    *uDgal         ; ALLOCATE (uDgal         , float   , catalog->Naverage);
    float    *P             ; ALLOCATE (P             , float   , catalog->Naverage);
    float    *dP            ; ALLOCATE (dP            , float   , catalog->Naverage);
    float    *ChiSqAve      ; ALLOCATE (ChiSqAve      , float   , catalog->Naverage);
    float    *ChiSqPM       ; ALLOCATE (ChiSqPM       , float   , catalog->Naverage);
    float    *ChiSqPar      ; ALLOCATE (ChiSqPar      , float   , catalog->Naverage);
    int      *Tmean         ; ALLOCATE (Tmean         , int     , catalog->Naverage);
    int      *Trange        ; ALLOCATE (Trange        , int     , catalog->Naverage);
    float    *stargal       ; ALLOCATE (stargal       , float   , catalog->Naverage);
    short    *Npos          ; ALLOCATE (Npos          , short   , catalog->Naverage);
    short    *Nmeasure      ; ALLOCATE (Nmeasure      , short   , catalog->Naverage);
    short    *Nmissing      ; ALLOCATE (Nmissing      , short   , catalog->Naverage);
    short    *Ngalphot     ; ALLOCATE (Ngalphot     , short   , catalog->Naverage);
    int      *measureOffset ; ALLOCATE (measureOffset , int     , catalog->Naverage);
    int      *missingOffset ; ALLOCATE (missingOffset , int     , catalog->Naverage);
    float    *refColorBlue  ; ALLOCATE (refColorBlue  , float   , catalog->Naverage);
    float    *refColorRed   ; ALLOCATE (refColorRed   , float   , catalog->Naverage);
    int      *flags         ; ALLOCATE (flags         , int     , catalog->Naverage);
    int      *photFlagsUpper; ALLOCATE (photFlagsUpper, int     , catalog->Naverage);
    int      *photFlagsLower; ALLOCATE (photFlagsLower, int     , catalog->Naverage);
    int      *objID         ; ALLOCATE (objID         , int     , catalog->Naverage);
    int      *catID         ; ALLOCATE (catID         , int     , catalog->Naverage);
    uint64_t *extID         ; ALLOCATE (extID         , uint64_t, catalog->Naverage);

    // assign the storage arrays
    Average *average = catalog->average;
    for (i = 0; i < catalog->Naverage; i++) {
      R[i]               = average[i].R               ;
      D[i]               = average[i].D               ;
      dR[i]              = average[i].dR              ;
      dD[i]              = average[i].dD              ;
      uR[i]              = average[i].uR              ;
      uD[i]              = average[i].uD              ;
      duR[i]             = average[i].duR             ;
      duD[i]             = average[i].duD             ;
      uRgal[i]           = average[i].uRgal           ;
      uDgal[i]           = average[i].uDgal           ;
      P[i]               = average[i].P               ;
      dP[i]              = average[i].dP              ;
      ChiSqAve[i]        = average[i].ChiSqAve        ;
      ChiSqPM[i]         = average[i].ChiSqPM         ;
      ChiSqPar[i]        = average[i].ChiSqPar        ;
      Tmean[i]           = average[i].Tmean           ;
      Trange[i]          = average[i].Trange          ;
      stargal[i]         = average[i].stargal         ;
      Npos[i]            = average[i].Npos            ;
      Nmeasure[i]        = average[i].Nmeasure        ;
      Nmissing[i]        = average[i].Nmissing        ;
      Ngalphot[i]       = average[i].Ngalphot       ;
      measureOffset[i]   = average[i].measureOffset   ; 
      missingOffset[i]   = average[i].missingOffset   ; 
      refColorBlue[i]    = average[i].refColorBlue    ;  
      refColorRed[i]     = average[i].refColorRed     ;  
      flags[i]           = average[i].flags           ;
      photFlagsUpper[i]  = average[i].photFlagsUpper  ;
      photFlagsLower[i]  = average[i].photFlagsLower  ;
      objID[i]           = average[i].objID           ;
      catID[i]           = average[i].catID           ;
      extID[i]           = average[i].extID           ;
    }

    // add the columns to the output array
    gfits_set_bintable_column (&theader, &ftable, "RA",             R,               catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "DEC",            D,               catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "RA_ERR",         dR,              catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "DEC_ERR",        dD,              catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "U_RA",           uR,              catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "U_DEC",          uD,              catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "U_RA_ERR",       duR,             catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "U_DEC_ERR",      duD,             catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "U_RA_GAL",       uRgal,           catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "U_DEC_GAL",      uDgal,           catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "PAR",            P,               catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "PAR_ERR",        dP,              catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "CHISQ_POS",      ChiSqAve,        catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "CHISQ_PM",       ChiSqPM,         catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "CHISQ_PAP",      ChiSqPar,        catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "MEAN_EPOCH",     Tmean,           catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "TIME_RANGE",     Trange,          catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "STARGAL_SEP",    stargal,         catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "NUMBER_POS",     Npos,            catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "NMEASURE",       Nmeasure,        catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "NMISSING",       Nmissing,        catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "NGALPHOT",       Ngalphot,        catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "OFF_MEASURE",    measureOffset,   catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "OFF_MISSING",    missingOffset,   catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "REF_COLOR_BLUE", refColorBlue,    catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "REF_COLOR_RED",  refColorRed,     catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "FLAGS",          flags,           catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "PHOTFLAGS_U",    photFlagsUpper,  catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "PHOTFLAGS_L",    photFlagsLower,  catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "OBJ_ID",         objID,           catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "CAT_ID",         catID,           catalog->Naverage);
    gfits_set_bintable_column (&theader, &ftable, "EXT_ID",         extID,           catalog->Naverage);

    free (R);
    free (D);
    free (dR);
    free (dD);
    free (uR);
    free (uD);
    free (duR);
    free (duD);
    free (uRgal);
    free (uDgal);
    free (P);
    free (dP);
    free (ChiSqAve);
    free (ChiSqPM);
    free (ChiSqPar);
    free (Tmean);
    free (Trange);
    free (stargal);
    free (Npos);
    free (Nmeasure);
    free (Nmissing);
    free (Ngalphot);
    free (measureOffset);
    free (missingOffset);
    free (refColorBlue);
    free (refColorRed);
    free (flags);
    free (photFlagsUpper);
    free (photFlagsLower);
    free (objID);
    free (catID);
    free (extID);

    gfits_fwrite_Theader (f, &theader);
    gfits_fwrite_table  (f, &ftable);
    gfits_free_header (&theader);
    gfits_free_table (&ftable);
  }

  /*** SecFilt ***/
  {
    gfits_create_table_header (&theader, "BINTABLE", "SECFILT");

    gfits_define_bintable_column (&theader, "E", "MAG",      "ra offset",                "arcsec", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_ERR",  "dec offset",               "arcsec", 1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_CHI",  "magnitude (sys)",           NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "J", "FLAGS",    "magnitude (cal)",           NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NCODE",    "magnitude (err)",           NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "I", "NUSED",    "airmass",                   NULL,    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_MIN",   "ccd x coord",              "pix",    1.0, 0.0);
    gfits_define_bintable_column (&theader, "E", "MAG_MAX",   "ccd y coord",              "pix",    1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&theader, &ftable);

    // Nsecfilt is number of average filters, Nsec is total number of values
    int Nsecfilt = GetPhotcodeNsecfilt();
    int Nsec = Nsecfilt * catalog->Naverage;

    // create intermediate storage arrays
    float *M        ; ALLOCATE (M      ,  float, Nsec);
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
    gfits_set_bintable_column (&theader, &ftable, "MAG_ERR",  dM    , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_CHI",  Mchisq, Nsec);
    gfits_set_bintable_column (&theader, &ftable, "FLAGS",    flags , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "NCODE",    Ncode , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "NUSED",    Nused , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_MIN",  Mmin  , Nsec);
    gfits_set_bintable_column (&theader, &ftable, "MAG_MAX",  Mmax  , Nsec);

    free (M      );
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
  ALLOCATE (bcatalog[0].average, Average,     Naves);
  ALLOCATE (bcatalog[0].secfilt, SecFilt,     Naves*Nsecfilt);
  
  int Nm = 0;
  int Na = 0;
  for (i = 0; i < Ncatalog; i++) {
    if (!catalog[i].Naverage) continue;
    for (j = 0; j < catalog[i].Naverage; j++) {
      bcatalog[0].average[Na] = catalog[i].average[j];
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

  unsigned int ID;
  for (ID = 0; ID <= catalogs->maxID; ID++) catalogs->index[ID] = -1;

  catalogs->Nsecfilt = Nsecfilt;

  catalogs->Ncatalog =  0;
  catalogs->NCATALOG = 16;
  catalogs->catalog = NULL;
  ALLOCATE (catalogs->catalog, Catalog, catalogs->NCATALOG);

  ALLOCATE (catalogs->catIDs,   unsigned int, catalogs->NCATALOG);
  ALLOCATE (catalogs->NAVERAGE, off_t,        catalogs->NCATALOG);
  ALLOCATE (catalogs->NMEASURE, off_t,        catalogs->NCATALOG);

  for (i = 0; i < catalogs->NCATALOG; i++) {
    dvo_catalog_init (&catalogs->catalog[i], TRUE);
    catalogs->catIDs[i] = 0;
    catalogs->NAVERAGE[i] = 100;
    catalogs->NMEASURE[i] = 100;
    catalogs->catalog[i].Naverage = 0;
    catalogs->catalog[i].Nmeasure = 0;
    ALLOCATE (catalogs->catalog[i].average,  Average,     catalogs->NAVERAGE[i]);
    ALLOCATE (catalogs->catalog[i].measureT, MeasureTiny, catalogs->NMEASURE[i]);
    ALLOCATE (catalogs->catalog[i].secfilt,  SecFilt,     catalogs->NAVERAGE[i]*Nsecfilt);
  }
  return catalogs;
}

// distribute a bright catalog across separate catalogs
int BrightCatalogSplitFree (CatalogSplitter *catalogs) {

# if (0)
  int i;

  for (i = 0; i < catalogs->NCATALOG; i++) {
    FREE (catalogs->catalog[i].average);
    FREE (catalogs->catalog[i].secfilt);
    FREE (catalogs->catalog[i].measureT);
  }
  FREE (catalogs->catalog);
# endif

  FREE (catalogs->catIDs);
  FREE (catalogs->NAVERAGE);
  FREE (catalogs->NMEASURE);
  FREE (catalogs->index);
  FREE (catalogs);
  return TRUE;
}

// distribute a bright catalog across separate catalogs
int BrightCatalogSplit (CatalogSplitter *catalogs, BrightCatalog *bcatalog) {

  int i;

  int Nsecfilt = catalogs->Nsecfilt;

  // find the max value of catID in this BrightCatalog
  unsigned int catIDmax = 0;
  for (i = 0; i < bcatalog->Naverage; i++) {
    catIDmax = MAX(catIDmax, bcatalog->average[i].catID);
  }
  // XXX validate the measure ID range here
    
  unsigned int maxIDold = catalogs->maxID;
  catalogs->maxID = MAX (maxIDold, catIDmax);
    
  // extend the index array and init
  REALLOCATE (catalogs->index, int, catalogs->maxID + 1);

  unsigned int id;
  for (id = maxIDold + 1; id <= catalogs->maxID; id++) catalogs->index[id] = -1;

  // identify the new catID values
  for (i = 0; i < bcatalog->Naverage; i++) {
    unsigned int catID = bcatalog->average[i].catID;
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
      int oldNCATALOG = catalogs->NCATALOG;
      catalogs->NCATALOG += 16;
      // fprintf (stderr, "realloc catalogs->catalog: old: %llx  ", (long long) catalogs->catalog);
      REALLOCATE (catalogs->catalog, Catalog, catalogs->NCATALOG);
      // fprintf (stderr, "new: %llx  -  %llx\n", (long long) catalogs->catalog, (long long) (catalogs->catalog + sizeof(Catalog)*catalogs->NCATALOG));

      // fprintf (stderr, "realloc catalogs->NAVERAGE: old: %llx  ", (long long) catalogs->NAVERAGE);
      REALLOCATE (catalogs->NAVERAGE, off_t,  catalogs->NCATALOG);
      // fprintf (stderr, "new: %llx  -  %llx\n", (long long) catalogs->NAVERAGE, (long long) (catalogs->NAVERAGE + sizeof(off_t)*catalogs->NCATALOG));

      // fprintf (stderr, "realloc catalogs->NMEASURE: old: %llx  ", (long long) catalogs->NMEASURE);
      REALLOCATE (catalogs->NMEASURE, off_t,  catalogs->NCATALOG);
      // fprintf (stderr, "new: %llx  -  %llx\n", (long long) catalogs->NMEASURE, (long long) (catalogs->NMEASURE + sizeof(off_t)*catalogs->NCATALOG));

      REALLOCATE (catalogs->catIDs, unsigned int, catalogs->NCATALOG);

      int j;
      for (j = oldNCATALOG; j < catalogs->NCATALOG; j++) {
	dvo_catalog_init (&catalogs->catalog[j], TRUE);
	catalogs->catIDs[j] = 0;
	catalogs->NAVERAGE[j] = 100;
	catalogs->NMEASURE[j] = 100;
	catalogs->catalog[j].Naverage = 0;
	catalogs->catalog[j].Nmeasure = 0;
	ALLOCATE (catalogs->catalog[j].average,  Average,     catalogs->NAVERAGE[j]);
	ALLOCATE (catalogs->catalog[j].measureT, MeasureTiny, catalogs->NMEASURE[j]);
	ALLOCATE (catalogs->catalog[j].secfilt,  SecFilt,     catalogs->NAVERAGE[j]*Nsecfilt);
      }
    }
  }

  // each bcatalog (from each host) has a different set of catID ranges
  // they also (probably) have contiguous ranges.  But, it is a bit tricky to be sure I
  // can use that info, so perhaps ignore it

  // assign the averages to the corresponding catalog
  for (i = 0; i < bcatalog->Naverage; i++) {
    unsigned int ID = bcatalog->average[i].catID;
    int Nc = catalogs->index[ID];
    assert (Nc > -1);
    assert (Nc < catalogs->NCATALOG);

    int Na = catalogs->catalog[Nc].Naverage;
    catalogs->catalog[Nc].average[Na] = bcatalog->average[i];

    // secfilt entries are grouped in blocks for each average
    int k;
    for (k = 0; k < Nsecfilt; k++) {
      catalogs->catalog[Nc].secfilt[Na*Nsecfilt + k] = bcatalog->secfilt[i*Nsecfilt + k];
    }      

    catalogs->catalog[Nc].Naverage ++;
    if (catalogs->catalog[Nc].Naverage >= catalogs->NAVERAGE[Nc]) {
      catalogs->NAVERAGE[Nc] += 100;
      REALLOCATE (catalogs->catalog[Nc].average,  Average, catalogs->NAVERAGE[Nc]);
      REALLOCATE (catalogs->catalog[Nc].secfilt,  SecFilt, catalogs->NAVERAGE[Nc]*Nsecfilt);
    }	       
  }

  // assign the measures to the corresponding catalog
  // XXX what about averef and related links?  Do I need them?
  for (i = 0; i < bcatalog->Nmeasure; i++) {
    unsigned int ID = bcatalog->measure[i].catID;
    int Nc = catalogs->index[ID];
    assert (Nc > -1);
    assert (Nc < catalogs->NCATALOG);
    int Na = catalogs->catalog[Nc].Nmeasure;
    catalogs->catalog[Nc].measureT[Na] = bcatalog->measure[i];
    catalogs->catalog[Nc].Nmeasure ++;
    if (catalogs->catalog[Nc].Nmeasure >= catalogs->NMEASURE[Nc]) {
      catalogs->NMEASURE[Nc] += 100;
      REALLOCATE (catalogs->catalog[Nc].measureT, MeasureTiny, catalogs->NMEASURE[Nc]);
    }	       
  }

  return TRUE;
}
