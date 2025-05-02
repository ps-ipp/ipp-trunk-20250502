# include "relastro.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
ImagePos *ImagePosLoad(char *filename, off_t *nimage_pos) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nimage_pos = 0;
  ImagePos *image_pos = NULL;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return NULL;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return NULL;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return (NULL);
  }
  fclose (f);

  // we read the entire block of data, then extract the columns, then set the image structure values.
  // I free the FITS table data after extracting the colums to avoid having 3 copies in memory.

  char type[16];

  GET_COLUMN (crval1,           "CRVAL1",            double);
  GET_COLUMN (crval2,           "CRVAL2",            double);
  GET_COLUMN (crpix1,           "CRPIX1",            float);
  GET_COLUMN (crpix2,           "CRPIX2",            float);
  GET_COLUMN (cdelt1,           "CDELT1",            float);
  GET_COLUMN (cdelt2,           "CDELT2",            float);
  GET_COLUMN (pc1_1,            "PC1_1",             float);
  GET_COLUMN (pc1_2,            "PC1_2",             float);
  GET_COLUMN (pc2_1,            "PC2_1",             float);
  GET_COLUMN (pc2_2,            "PC2_2",             float);
  GET_COLUMN (polyterms,        "POLYTERMS",         float); // verify that we got 14 columns?
  GET_COLUMN (ctype,            "CTYPE",             char);  // verify that we got 15 columns?
  GET_COLUMN (Npolyterms,       "NPOLYTERMS",        gfbyte);
  GET_COLUMN (dXpixSys,         "XPIX_SYS_ERR",      float);
  GET_COLUMN (dYpixSys,         "YPIX_SYS_ERR",      float);
  GET_COLUMN (refColorBlue,     "REF_COLOR_BLUE",    float);
  GET_COLUMN (refColorRed,      "REF_COLOR_RED",     float);
  GET_COLUMN (imageID,          "ID",                int);
  GET_COLUMN (nFitAstrom,       "NFIT",              int);
  GET_COLUMN (flags,            "FLAGS",             int);

  // free the memory associated with the FITS files
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  ALLOCATE (image_pos, ImagePos, Nrow);
  for (i = 0; i < Nrow; i++) {
    InitCoords(&image_pos[i].coords, NULL);
    image_pos[i].coords.crval1             = crval1      [i];
    image_pos[i].coords.crval2             = crval2      [i];
    image_pos[i].coords.crpix1             = crpix1      [i];
    image_pos[i].coords.crpix2             = crpix2      [i];
    image_pos[i].coords.cdelt1             = cdelt1      [i];
    image_pos[i].coords.cdelt2             = cdelt2      [i];
    image_pos[i].coords.pc1_1              = pc1_1       [i];
    image_pos[i].coords.pc1_2              = pc1_2       [i];
    image_pos[i].coords.pc2_1              = pc2_1       [i];
    image_pos[i].coords.pc2_2              = pc2_2       [i];
    image_pos[i].coords.Npolyterms         = Npolyterms  [i];
    image_pos[i].dXpixSys                  = dXpixSys    [i];
    image_pos[i].dYpixSys                  = dYpixSys    [i];
    image_pos[i].refColorBlue              = refColorBlue[i];
    image_pos[i].refColorRed               = refColorRed [i];
    image_pos[i].imageID                   = imageID     [i];
    image_pos[i].nFitAstrom                = nFitAstrom  [i];
    image_pos[i].flags                     = flags       [i];

    // polyterms and ctype are a bit different
    memcpy (&image_pos[i].coords.polyterms, &polyterms[i*14], 14*sizeof(float));
    memcpy (&image_pos[i].coords.ctype    , &ctype    [i*15], 15*sizeof(char));
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (crval1      );
  free (crval2      );
  free (crpix1      );
  free (crpix2      );
  free (cdelt1      );
  free (cdelt2      );
  free (pc1_1       );
  free (pc1_2       );
  free (pc2_1       );
  free (pc2_2       );
  free (polyterms   );
  free (ctype       );
  free (Npolyterms  );
  free (dXpixSys    );
  free (dYpixSys    );
  free (refColorBlue);
  free (refColorRed );
  free (imageID     );
  free (nFitAstrom  );
  free (flags       );

  *nimage_pos = Nrow;
  return image_pos;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)                                   \
  if (!(STATUS)) {                                                      \
    fprintf (stderr, MSG, __VA_ARGS__);                                 \
    return FALSE;                                                       \
  }

int ImagePosSave(char *filename, ImagePos *image_pos, off_t Nimage_pos) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_POS");

  gfits_define_bintable_column (&theader, "D",   "CRVAL1",         "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D",   "CRVAL2",         "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "CRPIX1",         "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "CRPIX2",         "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "CDELT1",         "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "CDELT2",         "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "PC1_1",          "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "PC1_2",          "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "PC2_1",          "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "PC2_2",          "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "14E", "POLYTERMS",      "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "15A", "CTYPE",          "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "L",   "NPOLYTERMS",     "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "XPIX_SYS_ERR",   "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "YPIX_SYS_ERR",   "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "REF_COLOR_BLUE", "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "REF_COLOR_RED",  "word", "unit", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "ID",             "image ID",               "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "NFIT",           "number of fitted stars", "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "FLAGS",          "analysis flags",         "unitless", 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  double *crval1      ;
  double *crval2      ;
  float  *crpix1      ;
  float  *crpix2      ;
  float  *cdelt1      ;
  float  *cdelt2      ;
  float  *pc1_1       ;
  float  *pc1_2       ;
  float  *pc2_1       ;
  float  *pc2_2       ;
  float  *polyterms   ;
  char   *ctype       ;
  char   *Npolyterms  ;
  float  *dXpixSys    ;
  float  *dYpixSys    ;
  float  *refColorBlue;
  float  *refColorRed ;
  int    *imageID     ;
  int    *nFitAstrom  ;
  int    *flags       ;

  // create intermediate storage arrays
  ALLOCATE (crval1    ,         double,          Nimage_pos);
  ALLOCATE (crval2    ,         double,          Nimage_pos);
  ALLOCATE (crpix1    ,         float ,          Nimage_pos);
  ALLOCATE (crpix2    ,         float ,          Nimage_pos);
  ALLOCATE (cdelt1    ,         float ,          Nimage_pos);
  ALLOCATE (cdelt2    ,         float ,          Nimage_pos);
  ALLOCATE (pc1_1     ,         float ,          Nimage_pos);
  ALLOCATE (pc1_2     ,         float ,          Nimage_pos);
  ALLOCATE (pc2_1     ,         float ,          Nimage_pos);
  ALLOCATE (pc2_2     ,         float ,          Nimage_pos);
  ALLOCATE (polyterms ,         float ,          14*Nimage_pos);
  ALLOCATE (ctype     ,         char  ,          15*Nimage_pos);
  ALLOCATE (Npolyterms,         char  ,          Nimage_pos);
  ALLOCATE (dXpixSys  ,         float ,          Nimage_pos);
  ALLOCATE (dYpixSys  ,         float ,          Nimage_pos);
  ALLOCATE (refColorBlue,       float ,          Nimage_pos);
  ALLOCATE (refColorRed ,       float ,          Nimage_pos);
  ALLOCATE (imageID   ,         int   ,          Nimage_pos);
  ALLOCATE (nFitAstrom,         int   ,          Nimage_pos);
  ALLOCATE (flags     ,         int   ,          Nimage_pos);

  // assign the storage arrays
  for (i = 0; i < Nimage_pos; i++) {
    crval1      [i] = image_pos[i].coords.crval1    ;
    crval2      [i] = image_pos[i].coords.crval2    ;
    crpix1      [i] = image_pos[i].coords.crpix1    ;
    crpix2      [i] = image_pos[i].coords.crpix2    ;
    cdelt1      [i] = image_pos[i].coords.cdelt1    ;
    cdelt2      [i] = image_pos[i].coords.cdelt2    ;
    pc1_1       [i] = image_pos[i].coords.pc1_1     ;
    pc1_2       [i] = image_pos[i].coords.pc1_2     ;
    pc2_1       [i] = image_pos[i].coords.pc2_1     ;
    pc2_2       [i] = image_pos[i].coords.pc2_2     ;
    Npolyterms  [i] = image_pos[i].coords.Npolyterms;
    dXpixSys    [i] = image_pos[i].dXpixSys         ;
    dYpixSys    [i] = image_pos[i].dYpixSys         ;
    refColorBlue[i] = image_pos[i].refColorBlue     ;
    refColorRed [i] = image_pos[i].refColorRed      ;
    imageID     [i] = image_pos[i].imageID          ;
    nFitAstrom  [i] = image_pos[i].nFitAstrom       ;
    flags       [i] = image_pos[i].flags            ;

    // polyterms and ctype are a bit different
    memcpy (&polyterms[i*14], &image_pos[i].coords.polyterms, 14*sizeof(float));
    memcpy (&ctype    [i*15], &image_pos[i].coords.ctype    , 15*sizeof(char));
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "CRVAL1",         crval1    ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "CRVAL2",         crval2    ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "CRPIX1",         crpix1    ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "CRPIX2",         crpix2    ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "CDELT1",         cdelt1    ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "CDELT2",         cdelt2    ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "PC1_1",          pc1_1     ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "PC1_2",          pc1_2     ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "PC2_1",          pc2_1     ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "PC2_2",          pc2_2     ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "POLYTERMS",      polyterms ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "CTYPE",          ctype     ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "NPOLYTERMS",     Npolyterms,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "XPIX_SYS_ERR",   dXpixSys  ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "YPIX_SYS_ERR",   dYpixSys  ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "REF_COLOR_BLUE", refColorBlue,       Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "REF_COLOR_RED",  refColorRed ,       Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "ID",             imageID   ,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "NFIT",           nFitAstrom,         Nimage_pos);
  gfits_set_bintable_column (&theader, &ftable, "FLAGS",          flags     ,         Nimage_pos);

  free (crval1      );
  free (crval2      );
  free (crpix1      );
  free (crpix2      );
  free (cdelt1      );
  free (cdelt2      );
  free (pc1_1       );
  free (pc1_2       );
  free (pc2_1       );
  free (pc2_2       );
  free (polyterms   );
  free (ctype       );
  free (Npolyterms  );
  free (dXpixSys    );
  free (dYpixSys    );
  free (refColorBlue);
  free (refColorRed );
  free (imageID     );
  free (nFitAstrom  );
  free (flags       );
  
  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image_pos file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for image_pos %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for image_pos %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for image_pos %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for image_pos %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file image_pos %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file image_pos %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing image_pos file %s\n", filename);

  return TRUE;
}
