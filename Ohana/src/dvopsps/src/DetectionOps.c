# include "dvopsps.h"

# define GET_COLUMN(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

Detections *DetectionsLoad(char *filename, int *Ndetections) {

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
  Detections *detections = NULL;

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

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) goto escape;

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
  // need to create and assign to flat-field correction
  GET_COLUMN(objID       , "objID",       int64_t);
  GET_COLUMN(detectID    , "detectID",    int64_t);
  GET_COLUMN(ippObjID    , "ippObjID",    int64_t);
  GET_COLUMN(ippDetectID , "ippDetectID", int);
  GET_COLUMN(imageID     , "imageID",     int);
  GET_COLUMN(catID       , "catID",       int);
  GET_COLUMN(ra          , "ra",          double); // XXX signed vs unsigned?
  GET_COLUMN(dec         , "dec",         double);
  GET_COLUMN(raErr       , "raErr",       float);
  GET_COLUMN(decErr      , "decErr",      float);
  GET_COLUMN(zpPSF       , "zpPSF",       float);
  GET_COLUMN(zpFactorPSF , "zpFactorPSF", float);
  GET_COLUMN(zpAPER      , "zpAPER",      float);
  GET_COLUMN(zpFactorAPER, "zpFactorAPER",float);
  GET_COLUMN(telluricExt , "telluricExt", float);
  GET_COLUMN(airmass     , "airmass",     float);
  GET_COLUMN(expTime     , "expTime",     float);
  GET_COLUMN(Mpsf        , "Mpsf",        float);
  GET_COLUMN(dMpsf       , "dMpsf",       float);
  GET_COLUMN(Mkron       , "Mkron",       float);
  GET_COLUMN(dMkron      , "dMkron",      float);
  GET_COLUMN(Map         , "Map",         float);
  GET_COLUMN(dMap        , "dMap",        float);
  GET_COLUMN(flags       , "flags",       int);
  GET_COLUMN(objflags    , "objflags",    int);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  ALLOCATE (detections, Detections, Nrow);
  for (i = 0; i < Nrow; i++) {
    detections[i].objID        = objID[i];      
    detections[i].detectID     = detectID[i];   
    detections[i].ippObjID     = ippObjID[i];   
    detections[i].ippDetectID  = ippDetectID[i];
    detections[i].imageID      = imageID[i];    
    detections[i].catID        = catID[i];    
    detections[i].ra           = ra[i];         
    detections[i].dec          = dec[i];        
    detections[i].raErr        = raErr[i];      
    detections[i].decErr       = decErr[i];     
    detections[i].zpPSF        = zpPSF[i];         
    detections[i].zpFactorPSF  = zpFactorPSF[i];         
    detections[i].zpAPER       = zpAPER[i];         
    detections[i].zpFactorAPER = zpFactorAPER[i];         
    detections[i].telluricExt  = telluricExt[i];      
    detections[i].airmass      = airmass[i];    
    detections[i].expTime      = expTime[i];    
    detections[i].Mpsf         = Mpsf[i];    
    detections[i].dMpsf        = dMpsf[i];    
    detections[i].Mkron        = Mkron[i];    
    detections[i].dMkron       = dMkron[i];    
    detections[i].Map          = Map[i];    
    detections[i].dMap         = dMap[i];    
    detections[i].flags        = flags[i];      
    detections[i].objflags     = objflags[i];      
  }
  fprintf (stderr, "loaded data for %lld detections\n", (long long) Nrow);

  free (objID      );
  free (detectID   );
  free (ippObjID   );
  free (ippDetectID);
  free (imageID    );
  free (catID      );
  free (ra         );
  free (dec        );
  free (raErr      );
  free (decErr     );
  free (zpPSF      );
  free (zpFactorPSF);
  free (zpAPER      );
  free (zpFactorAPER);
  free (telluricExt);
  free (airmass    );
  free (expTime    );
  free (Mpsf       );
  free (dMpsf      );
  free (Mkron      );
  free (dMkron     );
  free (Map        );
  free (dMap       );
  free (flags      );
  free (objflags   );

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);

  *Ndetections = Nrow;
  return detections;

 escape:
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  if (detections) free (detections);

  fclose (f);
  return NULL;
}

// we are passed a Detections structure, write it to a FITS table (3 ext)
int DetectionsSave(char *filename, Detections *detections, int Ndetections) {

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

  gfits_create_table_header (&theader, "BINTABLE", "DETECTIONS");

  // XXX need to get the bzero values right
  gfits_define_bintable_column (&theader, "K", "objID",       NULL, NULL, 1.0, 0);
  gfits_define_bintable_column (&theader, "K", "detectID",    NULL, NULL, 1.0, 0);
  gfits_define_bintable_column (&theader, "K", "ippObjID",    NULL, NULL, 1.0, 0);
  gfits_define_bintable_column (&theader, "J", "ippDetectID", NULL, NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "imageID",     NULL, NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "catID",       NULL, NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "D", "ra",          NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "dec",         NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "raErr",       NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "decErr",      NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "zpPSF",          NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "zpFactorPSF",    NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "zpAPER",         NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "zpFactorAPER",   NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "telluricExt", NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "airmass",     NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "expTime",     NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "Mpsf",        NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dMpsf",       NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "Mkron",       NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dMkron",      NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "Map",         NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dMap",        NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "flags",       NULL, NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "objflags",    NULL, NULL, 1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // create intermediate storage arrays
  uint64_t   *objID       ; ALLOCATE (objID       ,  uint64_t, Ndetections);
  uint64_t   *detectID    ; ALLOCATE (detectID    ,  uint64_t, Ndetections);
  uint64_t   *ippObjID    ; ALLOCATE (ippObjID    ,  uint64_t, Ndetections);
  uint32_t   *ippDetectID ; ALLOCATE (ippDetectID ,  uint32_t, Ndetections);
   int32_t   *imageID     ; ALLOCATE (imageID     ,   int32_t, Ndetections);
   int32_t   *catID       ; ALLOCATE (catID       ,   int32_t, Ndetections);
  double     *ra          ; ALLOCATE (ra          ,  double,   Ndetections);
  double     *dec         ; ALLOCATE (dec         ,  double,   Ndetections);
  float      *raErr       ; ALLOCATE (raErr       ,  float,    Ndetections);
  float      *decErr      ; ALLOCATE (decErr      ,  float,    Ndetections);
  float      *zpPSF       ; ALLOCATE (zpPSF       ,  float,    Ndetections);
  float      *zpFactorPSF ; ALLOCATE (zpFactorPSF ,  float,    Ndetections);
  float      *zpAPER      ; ALLOCATE (zpAPER      ,  float,    Ndetections);
  float      *zpFactorAPER; ALLOCATE (zpFactorAPER,  float,    Ndetections);
  float      *telluricExt ; ALLOCATE (telluricExt ,  float,    Ndetections);
  float      *airmass     ; ALLOCATE (airmass     ,  float,    Ndetections);
  float      *expTime     ; ALLOCATE (expTime     ,  float,    Ndetections);
  float      *Mpsf        ; ALLOCATE (Mpsf        ,  float,    Ndetections);
  float      *dMpsf       ; ALLOCATE (dMpsf       ,  float,    Ndetections);
  float      *Mkron       ; ALLOCATE (Mkron       ,  float,    Ndetections);
  float      *dMkron      ; ALLOCATE (dMkron      ,  float,    Ndetections);
  float      *Map         ; ALLOCATE (Map         ,  float,    Ndetections);
  float      *dMap        ; ALLOCATE (dMap        ,  float,    Ndetections);
  uint32_t   *flags       ; ALLOCATE (flags       ,  uint32_t, Ndetections);
  uint32_t   *objflags    ; ALLOCATE (objflags    ,  uint32_t, Ndetections);

  // assign the storage arrays
  for (i = 0; i < Ndetections; i++) {
    objID[i]       = detections[i].objID       ;
    detectID[i]    = detections[i].detectID    ;
    ippObjID[i]    = detections[i].ippObjID    ;
    ippDetectID[i] = detections[i].ippDetectID ;
    imageID[i]     = detections[i].imageID     ;
    catID[i]       = detections[i].catID       ;
    ra[i]          = detections[i].ra          ;
    dec[i]         = detections[i].dec         ;
    raErr[i]       = detections[i].raErr       ;
    decErr[i]      = detections[i].decErr      ;
    zpPSF[i]       = detections[i].zpPSF       ;
    zpFactorPSF[i] = detections[i].zpFactorPSF ;
    zpAPER[i]      = detections[i].zpAPER      ;
    zpFactorAPER[i]= detections[i].zpFactorAPER;
    telluricExt[i] = detections[i].telluricExt ;
    airmass[i]     = detections[i].airmass     ;
    expTime[i]     = detections[i].expTime     ;
    Mpsf[i]        = detections[i].Mpsf        ;
    dMpsf[i]       = detections[i].dMpsf       ;
    Mkron[i]       = detections[i].Mkron       ;
    dMkron[i]      = detections[i].dMkron      ;
    Map[i]         = detections[i].Map         ;
    dMap[i]        = detections[i].dMap        ;
    flags[i]       = detections[i].flags       ;
    objflags[i]    = detections[i].objflags    ;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "objID",       objID       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "detectID",    detectID    , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "ippObjID",    ippObjID    , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "ippDetectID", ippDetectID , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "imageID",     imageID     , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "catID",       catID       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "ra",          ra          , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "dec",         dec         , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "raErr",       raErr       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "decErr",      decErr      , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "zpPSF",       zpPSF       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "zpFactorPSF", zpFactorPSF , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "zpAPER",      zpAPER      , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "zpFactorAPER",zpFactorAPER, Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "telluricExt", telluricExt , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "airmass",     airmass     , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "expTime",     expTime     , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "Mpsf",        Mpsf        , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "dMpsf",       dMpsf       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "Mkron",       Mkron       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "dMkron",      dMkron      , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "Map",         Map         , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "dMap",        dMap        , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "flags",       flags       , Ndetections);
  gfits_set_bintable_column (&theader, &ftable, "objflags",    objflags    , Ndetections);

  free (objID       );
  free (detectID    );
  free (ippObjID    );
  free (ippDetectID );
  free (imageID     );
  free (catID       );
  free (ra          );
  free (dec         );
  free (raErr       );
  free (decErr      );
  free (zpPSF       );
  free (zpFactorPSF );
  free (zpAPER      );
  free (zpFactorAPER);
  free (telluricExt );
  free (airmass     );
  free (expTime     );
  free (Mpsf        );
  free (dMpsf       );
  free (Mkron       );
  free (dMkron      );
  free (Map         );
  free (dMap        );
  free (flags       );
  free (objflags    );

  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);
  return TRUE;
}

