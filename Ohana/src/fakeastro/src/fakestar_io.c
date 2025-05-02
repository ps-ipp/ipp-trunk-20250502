# include "fakeastro.h"

int fakestar_save_stars (char *filename, FakeAstro_Stars *stars, int Nstars) {

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
    myAbortF ("ERROR: cannot open file for output %s\n", filename);
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  gfits_create_table_header (&theader, "BINTABLE", "STARPAR");

  gfits_define_bintable_column (&theader, "D", "RA",       "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",      "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "GLON",     "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "GLAT",     "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "Ebv",      "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dEbv",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "DistMag",  "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dDistMag", "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "M_r",      "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dM_r",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "FeH",      "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dFeH",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "averef",   "", "", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "objID",    "", "", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "catID",    "", "", 1.0, FT_BZERO_INT32);

  int i;
  double       *ra       = NULL; ALLOCATE (ra      , double	 , Nstars); for (i = 0; i < Nstars; i++) ra      [i] = stars[i].R               ;
  double       *dec      = NULL; ALLOCATE (dec     , double	 , Nstars); for (i = 0; i < Nstars; i++) dec     [i] = stars[i].D               ;
  float        *glon     = NULL; ALLOCATE (glon    , float 	 , Nstars); for (i = 0; i < Nstars; i++) glon    [i] = stars[i].starpar.galLon  ;
  float        *glat     = NULL; ALLOCATE (glat    , float 	 , Nstars); for (i = 0; i < Nstars; i++) glat    [i] = stars[i].starpar.galLat  ;
  float        *Ebv      = NULL; ALLOCATE (Ebv     , float	 , Nstars); for (i = 0; i < Nstars; i++) Ebv     [i] = stars[i].starpar.Ebv     ;
  float        *dEbv     = NULL; ALLOCATE (dEbv    , float	 , Nstars); for (i = 0; i < Nstars; i++) dEbv    [i] = stars[i].starpar.dEbv    ;
  float        *DistMag  = NULL; ALLOCATE (DistMag , float	 , Nstars); for (i = 0; i < Nstars; i++) DistMag [i] = stars[i].starpar.DistMag ;
  float        *dDistMag = NULL; ALLOCATE (dDistMag, float	 , Nstars); for (i = 0; i < Nstars; i++) dDistMag[i] = stars[i].starpar.dDistMag;
  float        *M_r      = NULL; ALLOCATE (M_r     , float	 , Nstars); for (i = 0; i < Nstars; i++) M_r     [i] = stars[i].starpar.M_r     ;
  float        *dM_r     = NULL; ALLOCATE (dM_r    , float	 , Nstars); for (i = 0; i < Nstars; i++) dM_r    [i] = stars[i].starpar.dM_r    ;
  float        *FeH      = NULL; ALLOCATE (FeH     , float	 , Nstars); for (i = 0; i < Nstars; i++) FeH     [i] = stars[i].starpar.FeH     ;
  float        *dFeH     = NULL; ALLOCATE (dFeH    , float	 , Nstars); for (i = 0; i < Nstars; i++) dFeH    [i] = stars[i].starpar.dFeH    ;
  unsigned int *averef   = NULL; ALLOCATE (averef  , unsigned int, Nstars); for (i = 0; i < Nstars; i++) averef  [i] = stars[i].starpar.averef  ;
  unsigned int *objID    = NULL; ALLOCATE (objID   , unsigned int, Nstars); for (i = 0; i < Nstars; i++) objID   [i] = stars[i].starpar.objID   ;
  unsigned int *catID    = NULL; ALLOCATE (catID   , unsigned int, Nstars); for (i = 0; i < Nstars; i++) catID   [i] = stars[i].starpar.catID   ;

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",       ra      , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "DEC",      dec     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "GLON",     glon    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "GLAT",     glat    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "Ebv",      Ebv     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dEbv",     dEbv    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "DistMag",  DistMag , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dDistMag", dDistMag, Nstars);
  gfits_set_bintable_column (&theader, &ftable, "M_r",      M_r     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dM_r",     dM_r    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "FeH",      FeH     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dFeH",     dFeH    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "averef",   averef  , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "objID",    objID   , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "catID",    catID   , Nstars);
    
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table (f, &ftable);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  return TRUE;
}


# define GET_COLUMN(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

FakeAstro_Stars *fakestar_load_stars (char *filename, int *nstars) {

  int i, Ncol;
  off_t Nrow;
  char type[16];

  FakeAstro_Stars *stars = NULL;

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  header.buffer = NULL;
  matrix.buffer = NULL;
  ftable.buffer = NULL;
  theader.buffer = NULL;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read header\n");
    goto escape;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read matrix\n");
    goto escape;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) goto escape;

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
 
  GET_COLUMN(ra      , "RA"        ,   double);
  GET_COLUMN(dec     , "DEC"       ,   double);
  GET_COLUMN(glon    , "GLON"      ,   float);
  GET_COLUMN(glat    , "GLAT"      ,   float);
  GET_COLUMN(Ebv     , "Ebv"       ,   float);
  GET_COLUMN(dEbv    , "dEbv"      ,   float);
  GET_COLUMN(DistMag , "DistMag"   ,   float);
  GET_COLUMN(dDistMag, "dDistMag"  ,   float);
  GET_COLUMN(M_r     , "M_r"       ,   float);
  GET_COLUMN(dM_r    , "dM_r"      ,   float);
  GET_COLUMN(FeH     , "FeH"       ,   float);
  GET_COLUMN(dFeH    , "dFeH"      ,   float);
  GET_COLUMN(averef  , "averef"    ,   unsigned int);
  GET_COLUMN(objID   , "objID"     ,   unsigned int);
  GET_COLUMN(catID   , "catID"     ,   unsigned int);

  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  ALLOCATE (stars, FakeAstro_Stars, Nrow);

  for (i = 0; i < Nrow; i++) {
    stars[i].R                = ra      [i];
    stars[i].D                = dec     [i];
    stars[i].starpar.R        = ra      [i];
    stars[i].starpar.D        = dec     [i];
    stars[i].starpar.galLon   = glon    [i];
    stars[i].starpar.galLat   = glat    [i];
    stars[i].starpar.Ebv      = Ebv     [i];
    stars[i].starpar.dEbv     = dEbv    [i];
    stars[i].starpar.DistMag  = DistMag [i];
    stars[i].starpar.dDistMag = dDistMag[i];
    stars[i].starpar.M_r      = M_r     [i];
    stars[i].starpar.dM_r     = dM_r    [i];
    stars[i].starpar.FeH      = FeH     [i];
    stars[i].starpar.dFeH     = dFeH    [i];
    stars[i].starpar.averef   = averef  [i];
    stars[i].starpar.objID    = objID   [i];
    stars[i].starpar.catID    = catID   [i];
  }

  fclose (f);
  *nstars = Nrow;
  return stars;

escape:
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  fclose (f);
  return NULL;
}
