# include "dvo.h"
# define VERBOSE 1

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot' and 'relphot'
ImageMetadata *ImageMetadataLoad(char *filename, off_t *nimage) {

  int i, Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nimage = 0;
  ImageMetadata *image = NULL;

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

  // a bit annoying : we read the entire block of data, then extract the columns, then set the image structure values.
  // this means I need 3 copies in memory at some point.  ugh.

  char type[16];

  GET_COLUMN (imageID,  "IMAGE_ID",       int);
  GET_COLUMN (externID, "EXTERN_ID",      int);
  GET_COLUMN (expname,  "EXPNAME_AS_INT", int);
  GET_COLUMN (crval1,   "CRVAL1",         double);
  GET_COLUMN (crval2,   "CRVAL2",         double);
  GET_COLUMN (theta,    "THETA",          float);
  GET_COLUMN (McalPSF,  "MCAL",           float);
  GET_COLUMN (secz,     "SECZ",           float);
  GET_COLUMN (Xcenter,  "X_CENTER",       float);
  GET_COLUMN (Ycenter,  "Y_CENTER",       float);

  ALLOCATE (image, ImageMetadata, Nrow);
  for (i = 0; i < Nrow; i++) {
    image[i].imageID  = imageID[i] ;
    image[i].externID = externID[i];
    image[i].expname  = expname[i] ;
    image[i].crval1   = crval1[i]  ;
    image[i].crval2   = crval2[i]  ;
    image[i].theta    = theta[i]  ;
    image[i].Mcal     = McalPSF[i]    ;
    image[i].secz     = secz[i]    ;
    image[i].Xcenter  = Xcenter[i] ;
    image[i].Ycenter  = Ycenter[i] ;
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (imageID);
  free (externID);
  free (expname);
  free (crval1);
  free (crval2);
  free (theta);
  free (McalPSF);
  free (secz);
  free (Xcenter);
  free (Ycenter);

  *nimage = Nrow;
  return image;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

// save a minimal set of metadata needed to mextract joins
int ImageMetadataSave(char *filename, Image *image, off_t Nimage) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  BuildChipMatch (image, Nimage);

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_SUBSET");

  gfits_define_bintable_column (&theader, "J", "IMAGE_ID", 	 "image ID",  	       NULL, 	     1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "EXTERN_ID",      "extern ID", 	       NULL, 	     1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "EXPNAME_AS_INT", "expname as integer", NULL, 	     1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "D", "CRVAL1", 	 "ra at center",       "degrees",    1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "CRVAL2", 	 "dec at center",      "degrees",    1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "THETA", 	 "camera rot angle",   "degrees",    1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL",  	 "zero point offset",  "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "SECZ",  	 "airmass",            "none", 	     1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "X_CENTER", 	 "chip center",        "none", 	     1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "Y_CENTER", 	 "chip center",        "none", 	     1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  unsigned int *imageID, *externID, *expname;
  double *crval1, *crval2;
  float *Mcal, *Xcenter, *Ycenter, *secz, *theta;

  // create intermediate storage arrays
  ALLOCATE (imageID,  unsigned int,   Nimage);
  ALLOCATE (externID, unsigned int,   Nimage);
  ALLOCATE (expname,  unsigned int,   Nimage);
  ALLOCATE (crval1,   double,  	      Nimage);
  ALLOCATE (crval2,   double, 	      Nimage);
  ALLOCATE (theta,    float, 	      Nimage);
  ALLOCATE (Mcal,     float, 	      Nimage);
  ALLOCATE (secz,     float, 	      Nimage);
  ALLOCATE (Xcenter,  float, 	      Nimage);
  ALLOCATE (Ycenter,  float, 	      Nimage);

  // assign the storage arrays
  for (i = 0; i < Nimage; i++) {
    if (!image[i].coords.mosaic) continue;
    imageID[i]  = image[i].imageID;
    externID[i] = image[i].externID;
    Coords *mosaic = image[i].coords.mosaic;

    crval1[i]   = mosaic->crval1;
    crval2[i]   = mosaic->crval2;

    theta[i]    = DEG_RAD*atan2(mosaic->pc1_2, mosaic->pc1_1);

    Mcal[i]     = image[i].McalPSF;
    secz[i]     = image[i].secz;
    Xcenter[i]  = 0.5*image[i].NX;
    Ycenter[i]  = 0.5*image[i].NY;

    expname[i]  = 0;
    if ((image[i].name[0] == 'o') && (image[i].name[5] == 'g') && (image[i].name[10] == 'o')) {
      int mjd  = atoi(&image[i].name[1]);
      int Nexp = atoi(&image[i].name[6]);
      expname[i] = mjd * 10000 + Nexp;
    }
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",       imageID,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "EXTERN_ID",      externID, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "EXPNAME_AS_INT", expname,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CRVAL1",         crval1,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CRVAL2",         crval2,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "THETA",          theta,   Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MCAL",           Mcal,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "SECZ",           secz,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "X_CENTER",       Xcenter, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "Y_CENTER",       Ycenter, Nimage);

  free (imageID);
  free (externID);
  free (expname);
  free (crval1);
  free (crval2);
  free (theta);
  free (Mcal);
  free (secz);
  free (Xcenter);
  free (Ycenter);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for image subset %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for image subset %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for image subset %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for image subset %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file image subset %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file image subset %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing image subset file %s\n", filename);

  return TRUE;
}
