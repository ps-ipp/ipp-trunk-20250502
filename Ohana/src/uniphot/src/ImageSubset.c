# include "setphot.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

ImageSubset *ImageSubsetLoad(char *filename, off_t *nimage) {

  int i, Ncol;
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

  // a bit annoying : we read the entire block of data (180M), then extract the columns, then set the image structure values.
  // this means I need 3 copies in memory at some point.  ugh.

  char type[16];

  GET_COLUMN (McalPSF,  "MCAL_PSF",   float);
  GET_COLUMN (McalAPER, "MCAL_APER",  float);
  GET_COLUMN (dMcal,    "MCAL_ERR",   float);
  GET_COLUMN (imageID,  "IMAGE_ID",   int);
  GET_COLUMN (map,      "PHOTOM_MAP", int);
  GET_COLUMN (flags,    "FLAGS",      int);

  // XXX free the fits table data here 

  ALLOCATE (image, ImageSubset, Nrow);
  for (i = 0; i < Nrow; i++) {
    image[i].imageID       = imageID[i];
    image[i].photom_map_id = map[i];
    image[i].flags         = flags[i];
    image[i].McalPSF       = McalPSF[i];
    image[i].McalAPER      = McalAPER[i];
    image[i].dMcal         = dMcal[i];
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (McalPSF);
  free (McalAPER);
  free (dMcal);
  free (imageID);
  free (map);
  free (flags);

  *nimage = Nrow;
  return image;
}

int ImageSubsetSave(char *filename, Image *image, off_t Nimage) {

  int i;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_SUBSET");

  gfits_define_bintable_column (&theader, "E", "MCAL",       "zero point offset", "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_ERR",   "zero point error",  "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "IMAGE_ID",   "image ID", 	  NULL, 	1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "PHOTOM_MAP", "map",      	  NULL, 	1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "FLAGS",      "flags",    	  NULL, 	1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // create intermediate storage arrays
  ALLOCATE_PTR (McalPSF,  float,        Nimage);
  ALLOCATE_PTR (McalAPER, float,        Nimage);
  ALLOCATE_PTR (dMcal,    float,        Nimage);
  ALLOCATE_PTR (imageID,  unsigned int, Nimage);
  ALLOCATE_PTR (map,      unsigned int, Nimage);
  ALLOCATE_PTR (flags,    unsigned int, Nimage);

  // assign the storage arrays
  for (i = 0; i < Nimage; i++) {
    imageID[i]  =  image[i].imageID;
    map[i]      =  image[i].photom_map_id;
    flags[i]    =  image[i].flags;
    McalPSF[i]  =  image[i].McalPSF;
    McalAPER[i] =  image[i].McalAPER;
    dMcal[i]    =  image[i].dMcal;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "MCAL_PSF",   McalPSF,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_APER",  McalAPER, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_ERR",   dMcal,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",   imageID,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PHOTOM_MAP", map,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "FLAGS",      flags,    Nimage);

  free (McalPSF);
  free (McalAPER);
  free (dMcal);
  free (imageID);
  free (map);
  free (flags);

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

Image *ImagesFromSubset (ImageSubset *subset, off_t N) {

  off_t i;

  // we have been given an ImageSubset array, containing a reduced set of image fields
  // create full a Image array and save the needed values
  Image *image = NULL;
  ALLOCATE (image, Image, N);
  for (i = 0; i < N; i++) {
    image[i].imageID       = subset[i].imageID      ;
    image[i].photom_map_id = subset[i].photom_map_id;
    image[i].flags         = subset[i].flags        ;
    image[i].McalPSF       = subset[i].McalPSF      ;
    image[i].McalAPER      = subset[i].McalAPER     ;
    image[i].dMcal         = subset[i].dMcal        ;
  }
  return image;
}

