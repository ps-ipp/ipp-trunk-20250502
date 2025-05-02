# include "setposangle.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

ImageSubset *ImageSubsetLoad(char *filename, off_t *nimage) {

  int Ncol;
  off_t i, j, k;
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

  // a bit annoying : we read the entire block of data (180M), then extract the columns,
  // then set the image structure values.  this means I need 3 copies in memory at some
  // point.  ugh.

  char type[16];

  GET_COLUMN (imageID,    "IMAGE_ID",   int);
  GET_COLUMN (tzero,      "TZERO",      float);
  GET_COLUMN (trate,      "TRATE",      float);
  GET_COLUMN (NX,         "NX",         int);
  GET_COLUMN (NY,         "NY",         int);
  GET_COLUMN (crval1,     "CRVAL1",     double);
  GET_COLUMN (crval2,     "CRVAL2",     double);
  GET_COLUMN (crpix1,     "CRPIX1",     float);
  GET_COLUMN (crpix2,     "CRPIX2",     float);
  GET_COLUMN (cdelt1,     "CDELT1",     float);
  GET_COLUMN (cdelt2,     "CDELT2",     float);
  GET_COLUMN (pc1_1,      "PC1_1",      float);
  GET_COLUMN (pc1_2,      "PC1_2",      float);
  GET_COLUMN (pc2_1,      "PC2_1",      float);
  GET_COLUMN (pc2_2,      "PC2_2",      float);
  GET_COLUMN (polyterms,  "POLYTERMS",  float);
  GET_COLUMN (ctype,      "CTYPE",      char);
  GET_COLUMN (Npolyterms, "NPOLYTERMS", int);

  // GET_COLUMN (polyterms,  "POLYTERMS",  float[7][2]);
  // GET_COLUMN (ctype,      "CTYPE",      char[15]);

  // XXX free the fits table data here 

  ALLOCATE (image, ImageSubset, Nrow);
  for (i = 0; i < Nrow; i++) {
    image[i].imageID           = imageID[i];
    image[i].tzero             = tzero[i];
    image[i].trate             = trate[i];
    image[i].NX                = NX[i];
    image[i].NY                = NY[i];
    image[i].coords.crval1     = crval1[i];
    image[i].coords.crval2     = crval2[i];
    image[i].coords.crpix1     = crpix1[i];
    image[i].coords.crpix2     = crpix2[i];
    image[i].coords.cdelt1     = cdelt1[i];
    image[i].coords.cdelt2     = cdelt2[i];
    image[i].coords.pc1_1      = pc1_1[i];
    image[i].coords.pc1_2      = pc1_2[i];
    image[i].coords.pc2_1      = pc2_1[i];
    image[i].coords.pc2_2      = pc2_2[i];
    for (j = 0; j < 7; j++) {
      for (k = 0; k < 2; k++) {
	image[i].coords.polyterms[j][k] = polyterms[i*14 + j*2 + k];
      }
    }
    for (j = 0; j < 15; j++) {
      image[i].coords.ctype[j] = ctype[i*15 + j];
    }
    image[i].coords.Npolyterms = Npolyterms[i];
  }
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  free (imageID);
  free (tzero);
  free (trate);
  free (NX);
  free (NY);
  free (crval1);
  free (crval2);
  free (crpix1);
  free (crpix2);
  free (cdelt1);
  free (cdelt2);
  free (pc1_1);
  free (pc1_2);
  free (pc2_1);
  free (pc2_2);
  free (polyterms);
  free (ctype);
  free (Npolyterms);

  *nimage = Nrow;
  return image;
}

int ImageSubsetSave(char *filename, ImageSubset *image, off_t Nimage) {

  off_t i, j, k;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_SUBSET");

  gfits_define_bintable_column (&theader, "J",   "IMAGE_ID",   "image ID", NULL, 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "E",   "TZERO",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E",   "TRATE",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "NX",         "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "NY",         "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", 	 "CRVAL1",     "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", 	 "CRVAL2",     "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "CRPIX1",     "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "CRPIX2",     "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "CDELT1",     "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "CDELT2",     "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "PC1_1",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "PC1_2",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "PC2_1",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", 	 "PC2_2",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "14E", "POLYTERMS",  "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "15A", "CTYPE",      "tmp", 	   "mp", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J",   "NPOLYTERMS", "tmp", 	   "mp", 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  double *crval1, *crval2;
  float *tzero, *trate, *crpix1, *crpix2, *cdelt1, *cdelt2, *pc1_1, *pc1_2, *pc2_1, *pc2_2, *polyterms;
  unsigned int *imageID;
  int *NX, *NY, *Npolyterms;
  char *ctype;

  // create intermediate storage arrays
  ALLOCATE (imageID,    unsigned int, Nimage);
  ALLOCATE (tzero,      float,        Nimage);
  ALLOCATE (trate,      float,        Nimage);
  ALLOCATE (NX,         int,          Nimage);
  ALLOCATE (NY,         int,          Nimage);
  ALLOCATE (crval1,     double,       Nimage);
  ALLOCATE (crval2,     double,       Nimage);
  ALLOCATE (crpix1,     float,        Nimage);
  ALLOCATE (crpix2,     float,        Nimage);
  ALLOCATE (cdelt1,     float,        Nimage);
  ALLOCATE (cdelt2,     float,        Nimage);
  ALLOCATE (pc1_1,      float,        Nimage);
  ALLOCATE (pc1_2,      float,        Nimage);
  ALLOCATE (pc2_1,      float,        Nimage);
  ALLOCATE (pc2_2,      float,        Nimage);
  ALLOCATE (polyterms,  float,     14*Nimage);
  ALLOCATE (ctype,      char,      15*Nimage);
  ALLOCATE (Npolyterms, int,          Nimage);

  // assign the storage arrays
  for (i = 0; i < Nimage; i++) {
    imageID[i] = image[i].imageID       ;
    tzero[i]   = image[i].tzero         ;  
    trate[i]   = image[i].trate         ;  
    NX[i]      = image[i].NX            ;	    
    NY[i]      = image[i].NY            ;	    
    crval1[i]  = image[i].coords.crval1 ; 
    crval2[i]  = image[i].coords.crval2 ; 
    crpix1[i]  = image[i].coords.crpix1 ; 
    crpix2[i]  = image[i].coords.crpix2 ; 
    cdelt1[i]  = image[i].coords.cdelt1 ; 
    cdelt2[i]  = image[i].coords.cdelt2 ; 
    pc1_1[i]   = image[i].coords.pc1_1  ;  
    pc1_2[i]   = image[i].coords.pc1_2  ;  
    pc2_1[i]   = image[i].coords.pc2_1  ;  
    pc2_2[i]   = image[i].coords.pc2_2  ;  
    for (j = 0; j < 7; j++) {
      for (k = 0; k < 2; k++) {
        polyterms[i*14 + j*2 + k] = image[i].coords.polyterms[j][k];
      }
    }
    for (j = 0; j < 15; j++) {
      ctype[i*15 + j] = image[i].coords.ctype[j];
    }
    Npolyterms[i] = image[i].coords.Npolyterms;
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_ID",   imageID,    Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TZERO",      tzero,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "TRATE",      trate,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NX",         NX,         Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NY",         NY,         Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CRVAL1",     crval1,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CRVAL2",     crval2,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CRPIX1",     crpix1,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CRPIX2",     crpix2,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CDELT1",     cdelt1,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CDELT2",     cdelt2,     Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PC1_1",      pc1_1,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PC1_2",      pc1_2,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PC2_1",      pc2_1,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "PC2_2",      pc2_2,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "POLYTERMS",  polyterms,  Nimage);
  gfits_set_bintable_column (&theader, &ftable, "CTYPE",      ctype,      Nimage);
  gfits_set_bintable_column (&theader, &ftable, "NPOLYTERMS", Npolyterms, Nimage);

  free (imageID   );
  free (tzero     );
  free (trate     );
  free (NX        );
  free (NY        );
  free (crval1    );
  free (crval2    );
  free (crpix1    );
  free (crpix2    );
  free (cdelt1    );
  free (cdelt2    );
  free (pc1_1     );
  free (pc1_2     );
  free (pc2_1     );
  free (pc2_2     );
  free (polyterms );
  free (ctype     );
  free (Npolyterms);

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
    image[i].imageID       = subset[i].imageID;
    image[i].NX            = subset[i].NX     ;
    image[i].NY            = subset[i].NY     ;
    image[i].tzero         = subset[i].tzero  ;
    image[i].trate         = subset[i].trate  ;
    image[i].coords        = subset[i].coords ;
  }
  return image;
}

ImageSubset *ImagesToSubset (Image *image, off_t N) {

  off_t i;

  // we have been given an ImageSubset array, containing a reduced set of image fields
  // create full a Image array and save the needed values
  ImageSubset *subset = NULL;
  ALLOCATE (subset, ImageSubset, N);

  for (i = 0; i < N; i++) {
    subset[i].imageID = image[i].imageID;
    subset[i].NX      = image[i].NX     ;
    subset[i].NY      = image[i].NY     ;
    subset[i].tzero   = image[i].tzero  ;
    subset[i].trate   = image[i].trate  ;
    subset[i].coords  = image[i].coords ;
  }
  return subset;
}

