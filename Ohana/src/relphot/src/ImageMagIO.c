# include "relphot.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
ImageMag *ImageMagLoad(char *filename, off_t *nimage_mags) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nimage_mags = 0;
  ImageMag *image_mags = NULL;

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

  GET_COLUMN (McalPSF,           "MCAL_PSF",       float);
  GET_COLUMN (McalAPER,          "MCAL_APER",      float);
  GET_COLUMN (dMcal,             "MCAL_ERR",       float);
  GET_COLUMN (dMagSys,           "MCAL_SYSERR",    float);
  GET_COLUMN (McalChiSq,         "MCAL_CHISQ",     float);
  GET_COLUMN (nFitPhotom,        "NFIT",           int);
  GET_COLUMN (flags,             "FLAGS",          int);
  GET_COLUMN (ubercalDist,       "UDIST",          int);
  GET_COLUMN (imageID,           "ID",             int);

  // free the memory associated with the FITS files
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  ALLOCATE (image_mags, ImageMag, Nrow);
  for (i = 0; i < Nrow; i++) {
    image_mags[i].McalPSF                   = McalPSF    [i];
    image_mags[i].McalAPER                  = McalAPER   [i];
    image_mags[i].dMcal                     = dMcal      [i];
    image_mags[i].dMagSys                   = dMagSys    [i];
    image_mags[i].McalChiSq                 = McalChiSq  [i];
    image_mags[i].nFitPhotom                = nFitPhotom [i];
    image_mags[i].flags                     = flags      [i];
    image_mags[i].ubercalDist               = ubercalDist[i];
    image_mags[i].imageID                   = imageID    [i];
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (McalPSF    );
  free (McalAPER   );
  free (dMcal      );
  free (dMagSys    );
  free (McalChiSq  );
  free (nFitPhotom );
  free (flags      );
  free (ubercalDist);
  free (imageID    );

  *nimage_mags = Nrow;
  return image_mags;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)                                   \
  if (!(STATUS)) {                                                      \
    fprintf (stderr, MSG, __VA_ARGS__);                                 \
    return FALSE;                                                       \
  }

int ImageMagSave(char *filename, ImageMag *image_mags, off_t Nimage_mags) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_MAGS");

  gfits_define_bintable_column (&theader, "E", "MCAL_PSF",       "PSF cal offset", 	       "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_APER",      "APER cal offset", 	       "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_ERR",       "cal error",  		       "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_SYSERR",    "systematic error", 	       "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_CHISQ",     "cal chisq", 		       "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "NFIT",           "number of fitted stars",     "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "FLAGS",          "analysis flags", 	       "unitless",   1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "UDIST",          "distance to ubercal images", "images",     1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "ID",             "image ID",  		       "unitless",   1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  float *McalPSF     ;
  float *McalAPER    ;
  float *dMcal       ;
  float *dMagSys     ;
  float *McalChiSq   ;
  int   *nFitPhotom  ;
  int   *flags       ;
  int   *ubercalDist ;
  int   *imageID     ;

  // create intermediate storage arrays
  ALLOCATE (McalPSF     ,         float,          Nimage_mags);
  ALLOCATE (McalAPER    ,         float,          Nimage_mags);
  ALLOCATE (dMcal       ,         float,          Nimage_mags);
  ALLOCATE (dMagSys     ,         float,          Nimage_mags);
  ALLOCATE (McalChiSq   ,         float,          Nimage_mags);
  ALLOCATE (nFitPhotom  ,           int,          Nimage_mags);
  ALLOCATE (flags       ,           int,          Nimage_mags);
  ALLOCATE (ubercalDist ,           int,          Nimage_mags);
  ALLOCATE (imageID     ,           int,          Nimage_mags);

  // assign the storage arrays
  for (i = 0; i < Nimage_mags; i++) {
    McalPSF    [i]   = image_mags[i].McalPSF    ;
    McalAPER   [i]   = image_mags[i].McalAPER   ;
    dMcal      [i]   = image_mags[i].dMcal      ;
    dMagSys    [i]   = image_mags[i].dMagSys    ;
    McalChiSq  [i]   = image_mags[i].McalChiSq  ;
    nFitPhotom [i]   = image_mags[i].nFitPhotom ;
    flags      [i]   = image_mags[i].flags      ;
    ubercalDist[i]   = image_mags[i].ubercalDist;
    imageID    [i]   = image_mags[i].imageID    ;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "MCAL_PSF",       McalPSF    ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_APER",      McalAPER   ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_ERR",       dMcal      ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_SYSERR",    dMagSys    ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_CHISQ",     McalChiSq  ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "NFIT",           nFitPhotom ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "FLAGS",          flags      ,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "UDIST",          ubercalDist,         Nimage_mags);
  gfits_set_bintable_column (&theader, &ftable, "ID",             imageID    ,         Nimage_mags);

  free (McalPSF    );
  free (McalAPER   );
  free (dMcal      );
  free (dMagSys    );
  free (McalChiSq  );
  free (nFitPhotom );
  free (flags      );
  free (ubercalDist);
  free (imageID    );

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image_mags file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for image_mags %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for image_mags %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for image_mags %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for image_mags %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file image_mags %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file image_mags %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing image_mags file %s\n", filename);

  return TRUE;
}
