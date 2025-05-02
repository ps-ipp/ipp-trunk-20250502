# include "delstar.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

ImageSubset *ImageSubsetLoad(char *filename, off_t *nimage) {

  int Ncol;
  off_t i;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  *nimage = 0;
  ImageSubset *image = NULL;

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
    return NULL;
  }
  fclose (f);

  char type[16];

  GET_COLUMN (imageID,    "IMAGE_ID",   int);
  GET_COLUMN (externID,   "EXTERN_ID",  int);
  GET_COLUMN (NX,         "NX",         short);
  GET_COLUMN (NY,         "NY",         short);
  GET_COLUMN (nstar,      "nstar",      int);
  GET_COLUMN (tzero,      "TZERO",      int);
  GET_COLUMN (trate,      "TRATE",      short);
  GET_COLUMN (photcode,   "PHOTCODE"  , short);

  ALLOCATE (image, ImageSubset, Nrow);
  for (i = 0; i < Nrow; i++) {
    image[i].imageID           = imageID[i];
    image[i].externID          = externID[i];
    image[i].NX                = NX[i];
    image[i].NY                = NY[i];
    image[i].nstar             = nstar[i];
    image[i].tzero             = tzero[i];
    image[i].trate             = trate[i];
    image[i].photcode          = photcode[i];
    image[i].tmin              = image[i].tzero - 1;
    image[i].tmax              = image[i].tzero + image[i].trate*image[i].NY + 1;
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (imageID);
  free (externID);
  free (NX);
  free (NY);
  free (nstar);
  free (tzero);
  free (trate);
  free (photcode);

  *nimage = Nrow;
  return image;
}

int ImageSubsetSave(char *filename, ImageSubset *image, off_t Nimage) {

  off_t i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_SUBSET");

  // an unsigned int needs to have bzero of 0x8000
  gfits_define_bintable_column (&theader, "J",   "IMAGE_ID",   "image ID",  "tmp", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J",   "EXTERN_ID",  "extern ID", "tmp", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "I",   "NX",         "tmp", 	    "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "I",   "NY",         "tmp", 	    "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "NSTAR",      "tmp", 	    "tmp", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J",   "TZERO",      "tmp", 	    "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "I",   "TRATE",      "tmp", 	    "tmp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "I",   "PHOTCODE",   "tmp", 	    "tmp", 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  unsigned int *imageID, *externID, *nstar;
  short *NX, *NY;
  e_time *tzero;
  short *trate;
  short *photcode;

  // create intermediate storage arrays
  ALLOCATE (imageID,    unsigned int, Nimage);
  ALLOCATE (externID,   unsigned int, Nimage);
  ALLOCATE (NX,         short,        Nimage);
  ALLOCATE (NY,         short,        Nimage);
  ALLOCATE (nstar,      unsigned int, Nimage);
  ALLOCATE (tzero,      e_time,       Nimage);
  ALLOCATE (trate,      short,        Nimage);
  ALLOCATE (photcode,   short,        Nimage);

  // assign the storage arrays
  for (i = 0; i < Nimage; i++) {
    imageID[i] 	= image[i].imageID ;
    externID[i] = image[i].externID;
    NX[i]      	= image[i].NX      ;	    
    NY[i]      	= image[i].NY      ;	    
    nstar[i]  	= image[i].nstar   ;	    
    tzero[i]   	= image[i].tzero   ;  
    trate[i]   	= image[i].trate   ;  
    photcode[i] = image[i].photcode;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",   imageID,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "EXTERN_ID",  externID, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NX",         NX,       Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NY",         NY,       Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NSTAR",      nstar,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TZERO",      tzero,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TRATE",      trate,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PHOTCODE",   photcode, Nimage);

  free (imageID );
  free (externID);
  free (NX      );
  free (NY      );
  free (nstar   );
  free (tzero   );
  free (trate   );
  free (photcode);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file for output %s\n", filename);
    return FALSE;
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  fclose (f);
  fflush (f);

  return TRUE;
}

ImageSubset *ImagesToSubset (Image *image, off_t N) {

  off_t i;

  // we have been given an ImageSubset array, containing a reduced set of image fields
  // create full a Image array and save the needed values
  ImageSubset *subset = NULL;
  ALLOCATE (subset, ImageSubset, N);

  for (i = 0; i < N; i++) {
    subset[i].imageID  = image[i].imageID  ;
    subset[i].externID = image[i].externID ;
    subset[i].NX       = image[i].NX       ;
    subset[i].NY       = image[i].NY       ;
    subset[i].nstar    = image[i].nstar    ;
    subset[i].tzero    = image[i].tzero    ;
    subset[i].trate    = image[i].trate    ;
    subset[i].tmin     = image[i].tzero - 1;
    subset[i].tmax     = image[i].tzero + image[i].trate*image[i].NY + 1;
    subset[i].photcode = image[i].photcode ;
  }
  return subset;
}


