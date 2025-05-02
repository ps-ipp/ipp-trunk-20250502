# include "addstar.h"
# include "setobjflags.h"

int setobjflags_save_stars (char *filename, MyStars *stars, int Nstars) {

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

  gfits_create_table_header (&theader, "BINTABLE", "SETOBJFLAGS");

  gfits_define_bintable_column (&theader, "D", "RA",       "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",      "", "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "MYBIT",    "", "", 1.0, FT_BZERO_INT32);

  int i;
  double       *ra       = NULL; ALLOCATE (ra      , double	 , Nstars); for (i = 0; i < Nstars; i++) ra      [i] = stars[i].R      ;
  double       *dec      = NULL; ALLOCATE (dec     , double	 , Nstars); for (i = 0; i < Nstars; i++) dec     [i] = stars[i].D      ;
  unsigned int *myBit    = NULL; ALLOCATE (myBit   , unsigned int, Nstars); for (i = 0; i < Nstars; i++) myBit   [i] = stars[i].myBit  ;

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",       ra      , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "DEC",      dec     , Nstars);
  gfits_set_bintable_column (&theader, &ftable, "MYBIT",    myBit   , Nstars);
    
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table (f, &ftable);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  free (ra      );
  free (dec     );
  free (myBit   );

  fflush (f);
  fclose (f);

  return TRUE;
}

# define GET_COLUMN(OUT,NAME,TYPE,RTYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #RTYPE), "wrong column type");

MyStars *setobjflags_load_stars (char *filename, int *nstars) {

  int i, Ncol;
  off_t Nrow;
  char type[16];

  MyStars *stars = NULL;

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
 
  GET_COLUMN(ra      , "RA"        ,   double      ,   double      );
  GET_COLUMN(dec     , "DEC"       ,   double      ,   double      );
  GET_COLUMN(myBit   , "MYBIT"     ,   unsigned int,   int         );

  gfits_free_header (&theader);
  gfits_free_table  (&ftable);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  ALLOCATE (stars, MyStars, Nrow);

  for (i = 0; i < Nrow; i++) {
    stars[i].R                = ra     [i];
    stars[i].D	              = dec    [i];
    stars[i].myBit            = myBit  [i];
    stars[i].flag             = FALSE;
    stars[i].found            = FALSE;
  }

  free (ra      );
  free (dec     );
  free (myBit   );

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
