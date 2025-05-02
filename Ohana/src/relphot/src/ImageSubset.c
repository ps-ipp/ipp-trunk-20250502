# include "relphot.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// this is nearly identical to the one in 'uniphot/src' used by setphot / setphot_client.
// Here, we use a handful of different columns (if not, we could move to libdvo)
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
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    return NULL;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    gfits_free_header (&theader);
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
    return (NULL);
  }
  fclose (f);

  // a bit annoying : we read the entire block of data, then extract the columns, then set the image structure values.
  // this means I need 3 copies in memory at some point.  ugh.

  char type[16];

  GET_COLUMN (McalPSF, 	 "MCAL_PSF",     float);
  GET_COLUMN (McalAPER,  "MCAL_APER",    float);
  GET_COLUMN (dMcal,   	 "MCAL_ERR",     float);
  GET_COLUMN (imageID, 	 "IMAGE_ID",     int);
  GET_COLUMN (map,     	 "PHOTOM_MAP",   int);
  GET_COLUMN (flags,   	 "FLAGS",        int);
  GET_COLUMN (tessID,  	 "TESS_ID",      int);
  GET_COLUMN (projID,  	 "PROJ_ID",      int);
  GET_COLUMN (skycellID, "SKYCELL_ID",   int);
  GET_COLUMN (tzero,     "TZERO",        int);
  GET_COLUMN (trate,     "TRATE",        short);
  GET_COLUMN (ucdist,    "UBERCAL_DIST", short);

  // XXX free the fits table data here 

  ALLOCATE (image, ImageSubset, Nrow);
  for (i = 0; i < Nrow; i++) {
    image[i].McalPSF       = McalPSF[i];
    image[i].McalAPER      = McalAPER[i];
    image[i].dMcal         = dMcal[i];
    image[i].imageID       = imageID[i];
    image[i].photom_map_id = map[i];
    image[i].flags         = flags[i];
    image[i].tessID        = tessID[i];
    image[i].projID        = projID[i];
    image[i].skycellID     = skycellID[i];
    image[i].tzero         = tzero[i];
    image[i].trate         = trate[i];
    image[i].ubercalDist   = ucdist[i];
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (McalPSF);
  free (McalAPER);
  free (dMcal);
  free (imageID);
  free (map);
  free (flags);
  free (tessID);
  free (projID);
  free (skycellID);
  free (tzero);
  free (trate);
  free (ucdist);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  *nimage = Nrow;
  return image;
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int ImageSubsetSave(char *filename, ImageSubset *image, off_t Nimage) {

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

  gfits_define_bintable_column (&theader, "E", "MCAL_PSF",     "zero point offset", "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_APER",    "zero point offset", "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL_ERR",     "zero point error",  "magnitudes", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "IMAGE_ID",     "image ID", 	    NULL,         1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "PHOTOM_MAP",   "map",      	    NULL,         1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "FLAGS",        "flags",    	    NULL,         1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "TESS_ID",      "ID", 	   	    NULL,         1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "PROJ_ID",      "ID", 	   	    NULL,         1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "SKYCELL_ID",   "ID", 	   	    NULL,         1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "TZERO",        "exposure start",    NULL,         1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "I", "TRATE",        "tti rate",          NULL,         1.0, FT_BZERO_INT16);
  gfits_define_bintable_column (&theader, "I", "UBERCAL_DIST", "ubercal distance",  NULL,         1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  float *McalPSF, *McalAPER, *dMcal;
  unsigned int *imageID, *map, *flags, *tzero;
  int *tessID, *projID, *skycellID;
  unsigned short *trate;
  short *ucdist;

  // create intermediate storage arrays
  ALLOCATE (McalPSF,   float, 	       Nimage);
  ALLOCATE (McalAPER,  float, 	       Nimage);
  ALLOCATE (dMcal,     float, 	       Nimage);
  ALLOCATE (imageID,   unsigned int,   Nimage);
  ALLOCATE (map,       unsigned int,   Nimage);
  ALLOCATE (flags,     unsigned int,   Nimage);
  ALLOCATE (tessID,    int,            Nimage);
  ALLOCATE (projID,    int,            Nimage);
  ALLOCATE (skycellID, int,            Nimage);
  ALLOCATE (tzero,     unsigned int,   Nimage);
  ALLOCATE (trate,     unsigned short, Nimage);
  ALLOCATE (ucdist,    short,          Nimage);

  // assign the storage arrays
  for (i = 0; i < Nimage; i++) {
    McalPSF[i]   = image[i].McalPSF;
    McalAPER[i]  = image[i].McalAPER;
    dMcal[i]     = image[i].dMcal;
    imageID[i]   = image[i].imageID;
    map[i]       = image[i].photom_map_id;
    flags[i]     = image[i].flags;
    tzero[i]     = image[i].tzero;
    trate[i]     = image[i].trate;
    ucdist[i]    = image[i].ubercalDist;
    tessID[i]    = image[i].tessID;
    projID[i]    = image[i].projID;
    skycellID[i] = image[i].skycellID;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "MCAL_PSF",     McalPSF,   Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_APER",    McalAPER,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "MCAL_ERR",     dMcal,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",     imageID,   Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PHOTOM_MAP",   map,       Nimage);
  gfits_set_bintable_column (&theader, &ftable, "FLAGS",        flags,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TESS_ID",      tessID,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PROJ_ID",      projID,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "SKYCELL_ID",   skycellID, Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TZERO",        tzero,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TRATE",        trate,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "UBERCAL_DIST", ucdist,    Nimage);

  free (McalPSF);
  free (McalAPER);
  free (dMcal);
  free (imageID);
  free (map);
  free (flags);
  free (tessID);
  free (projID);
  free (skycellID);
  free (tzero);
  free (trate);
  free (ucdist);

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
