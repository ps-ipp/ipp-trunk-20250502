# include "addstar.h"
# include "loadICRF.h"

int loadICRF_save_stars (char *filename, ICRF_Stars *stars, int Nstars) {

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

  gfits_create_table_header (&theader, "BINTABLE", "ICRF");

  gfits_define_bintable_column (&theader, "D", "RA",       "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",      "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "RA_ERR",   "", "arcsec", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "DEC_ERR",  "", "arcsec", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "F_PSF",    "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dF_PSF",   "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "F_AP",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dF_AP",    "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "M_PSF",    "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dM_PSF",   "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "M_AP",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "dM_AP",    "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "PHOTCODE", "", "", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "FLAGS",    "", "", 1.0, FT_BZERO_INT32);

  int i;
  double       *ra       = NULL; ALLOCATE (ra      , double	 , Nstars); for (i = 0; i < Nstars; i++) ra      [i] = stars[i].R ;
  double       *dec      = NULL; ALLOCATE (dec     , double	 , Nstars); for (i = 0; i < Nstars; i++) dec     [i] = stars[i].D ;
  float        *dR       = NULL; ALLOCATE (dR      , float 	 , Nstars); for (i = 0; i < Nstars; i++) dR      [i] = stars[i].measure.dXccd   ;
  float        *dD       = NULL; ALLOCATE (dD      , float 	 , Nstars); for (i = 0; i < Nstars; i++) dD      [i] = stars[i].measure.dYccd   ;
  float        *Fpsf     = NULL; ALLOCATE (Fpsf    , float	 , Nstars); for (i = 0; i < Nstars; i++) Fpsf    [i] = stars[i].measure.FluxPSF ;
  float        *dFpsf    = NULL; ALLOCATE (dFpsf   , float	 , Nstars); for (i = 0; i < Nstars; i++) dFpsf   [i] = stars[i].measure.dFluxPSF;
  float        *Fap      = NULL; ALLOCATE (Fap     , float	 , Nstars); for (i = 0; i < Nstars; i++) Fap     [i] = stars[i].measure.FluxAp  ;
  float        *dFap     = NULL; ALLOCATE (dFap    , float	 , Nstars); for (i = 0; i < Nstars; i++) dFap    [i] = stars[i].measure.dFluxAp ;
  float        *Mpsf     = NULL; ALLOCATE (Mpsf    , float	 , Nstars); for (i = 0; i < Nstars; i++) Mpsf    [i] = stars[i].measure.M       ;
  float        *dMpsf    = NULL; ALLOCATE (dMpsf   , float	 , Nstars); for (i = 0; i < Nstars; i++) dMpsf   [i] = stars[i].measure.dM      ;
  float        *Map      = NULL; ALLOCATE (Map     , float	 , Nstars); for (i = 0; i < Nstars; i++) Map     [i] = stars[i].measure.Map     ;
  float        *dMap     = NULL; ALLOCATE (dMap    , float	 , Nstars); for (i = 0; i < Nstars; i++) dMap    [i] = stars[i].measure.dMap    ;
  unsigned int *photcode = NULL; ALLOCATE (photcode, unsigned int, Nstars); for (i = 0; i < Nstars; i++) photcode[i] = stars[i].measure.photcode;
  unsigned int *flags    = NULL; ALLOCATE (flags   , unsigned int, Nstars); for (i = 0; i < Nstars; i++) flags   [i] = stars[i].measure.dbFlags ;

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",       ra      , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "DEC",      dec     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "RA_ERR",   dR      , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "DEC_ERR",  dD      , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "F_PSF",    Fpsf    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dF_PSF",   dFpsf   , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "F_AP",     Fap     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dF_AP",    dFap    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "M_PSF",    Mpsf    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dM_PSF",   dMpsf   , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "M_AP",     Map     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "dM_AP",    dMap    , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "PHOTCODE", photcode, Nstars);
  gfits_set_bintable_column (&theader, &ftable, "FLAGS",    flags   , Nstars);
    
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table (f, &ftable);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  fclose (f);

  return TRUE;
}


# define GET_COLUMN(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

ICRF_Stars *loadICRF_load_stars (char *filename, int *nstars) {

  int i, Ncol;
  off_t Nrow;
  char type[16];

  ICRF_Stars *stars = NULL;

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
 
  GET_COLUMN(ra      , "RA",          double);
  GET_COLUMN(dec     , "DEC",         double);
  GET_COLUMN(dR      , "RA_ERR",      float);
  GET_COLUMN(dD      , "DEC_ERR",     float);
  GET_COLUMN(Fpsf    , "F_PSF",       float);
  GET_COLUMN(dFpsf   , "dF_PSF",      float);
  GET_COLUMN(Fap     , "F_AP",        float);
  GET_COLUMN(dFap    , "dF_AP",       float);
  GET_COLUMN(Mpsf    , "M_PSF",       float);
  GET_COLUMN(dMpsf   , "dM_PSF",      float);
  GET_COLUMN(Map     , "M_AP",        float);
  GET_COLUMN(dMap    , "dM_AP",       float);
  GET_COLUMN(photcode, "PHOTCODE",    int);
  GET_COLUMN(flags   , "FLAGS",       int);

  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  ALLOCATE (stars, ICRF_Stars, Nrow);

  for (i = 0; i < Nrow; i++) {
    InitICRF_Star (&stars[i]);
    stars[i].R               = ra      [i];
    stars[i].D	             = dec     [i];
    stars[i].measure.R       = ra      [i];
    stars[i].measure.D       = dec     [i];
    stars[i].measure.dXccd   = dR      [i];
    stars[i].measure.dYccd   = dD      [i];
    stars[i].measure.FluxPSF = Fpsf    [i];
    stars[i].measure.dFluxPSF= dFpsf   [i];
    stars[i].measure.FluxAp  = Fap     [i];
    stars[i].measure.dFluxAp = dFap    [i];
    stars[i].measure.M       = Mpsf    [i];
    stars[i].measure.dM      = dMpsf   [i];
    stars[i].measure.Map     = Map     [i];
    stars[i].measure.dMap    = dMap    [i];
    stars[i].measure.photcode= photcode[i];
    stars[i].measure.dbFlags = flags   [i];
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
