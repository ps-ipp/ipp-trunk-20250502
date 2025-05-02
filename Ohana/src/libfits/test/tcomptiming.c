# include <ohana.h>
# include <gfitsio.h>
# include "tap_ohana.h"

int test_compress_timing (char *zcmptype, int Ntile);
void gfits_uncompress_timing ();

char *cmptype[] = {"NONE", "NONE_1", "NONE_2", "GZIP_1", "GZIP_2", "RICE_1", "RICE_ONE", "AUTO", NULL};

static int Nval = 1000000;

int main (int argc, char **argv) {
  
  plan_tests (668);
  // plan_skip_all ("skipping");

  diag ("libfits tablecomp.c tests");

  // test_compress_empty ("GZIP_1");
  // exit (0);

  float lNtileMax = log10(Nval) + 0.1;
  float lNtile = 0.0;
  for (lNtile = 4.0; lNtile < lNtileMax; lNtile += 0.5) {
    int Ntile = pow (10.0, lNtile);
    fprintf (stderr, "--- Ntile = %d ---\n", Ntile);
    test_compress_timing ("GZIP_2", Ntile);
    gfits_compress_timing();
    gfits_uncompress_timing();
  }
  ok (ohana_memcheck (TRUE), "no memory corruption");

  return exit_status();
}

int test_compress_timing (char *zcmptype, int Ntile) { // test 2: make a table, compress, uncompress, compare : use compression "NONE"

  Header header;
  FTable ftable;

  diag ("--- starting test_compress with zcmptype %s ---", zcmptype);
  ok (gfits_init_header (&header), "inited the header");
  ok (gfits_init_table (&ftable), "inited the table");

  ok (gfits_create_table_header (&header, "BINTABLE", "TESTDATA"), "created the table header");

  ok (gfits_define_bintable_column (&header, "B", "VAL_B", "byte",   "none", 1.0, 0.0), "defined byte column");
  ok (gfits_define_bintable_column (&header, "I", "VAL_I", "short",  "none", 1.0, 0.0), "defined short column");
  ok (gfits_define_bintable_column (&header, "J", "VAL_J", "int",    "none", 1.0, 0.0),  "defined int column");
  ok (gfits_define_bintable_column (&header, "K", "VAL_K", "long",   "none", 1.0, 0.0),  "defined long column");
  ok (gfits_define_bintable_column (&header, "E", "VAL_E", "float",  "degree", 1.0, 0.0),   "defined float column");
  ok (gfits_define_bintable_column (&header, "D", "VAL_D", "double", "degree", 1.0, 0.0),   "defined double column");
  
  // generate the output array that carries the data
  ok (gfits_create_table (&header, &ftable), "created the basic table");

  // XXX EAM:
  struct timeval startTimer, stopTimer;
  float dtime;
  gettimeofday (&startTimer, (void *) NULL);

  float timeSum1 = 0.0;
  float timeSum2 = 0.0;
  float timeSum3 = 0.0;
  float timeSum4 = 0.0;
  float timeSum5 = 0.0;
  float timeSum6 = 0.0;
  float timeSum7 = 0.0;
  float timeSum8 = 0.0;

  char    *VAL_B;  ALLOCATE (VAL_B, char,    Nval);
  short   *VAL_I;  ALLOCATE (VAL_I, short,   Nval);
  int     *VAL_J;  ALLOCATE (VAL_J, int,     Nval);
  int64_t *VAL_K;  ALLOCATE (VAL_K, int64_t, Nval);
  float   *VAL_E;  ALLOCATE (VAL_E, float,   Nval);
  double  *VAL_D;  ALLOCATE (VAL_D, double,  Nval);
  
  long A = time(NULL);
  srand48(A);

  // XXX TIMER 1 (ALLOCATE)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum1 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  int i;
  for (i = 0; i < Nval; i++) {
    VAL_B[i] =       0xff & lrand48();
    VAL_I[i] =     0xffff & lrand48();
    VAL_J[i] = 0xffffffff & lrand48();
    VAL_K[i] = (((int64_t)lrand48()) << 32) + (int64_t)lrand48();
    VAL_E[i] = FLT_MAX * (2.0*drand48() - 1.0);
    VAL_D[i] = DBL_MAX * (2.0*drand48() - 1.0);
  }     

  // XXX TIMER 2 (set values)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum2 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  // add the columns to the output array
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_B", VAL_B, Nval), "set byte   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_I", VAL_I, Nval), "set short  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_J", VAL_J, Nval), "set int    table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_K", VAL_K, Nval), "set long   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_E", VAL_E, Nval), "set float  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_D", VAL_D, Nval), "set double table column");

  // XXX TIMER 3 (set bintable column)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum3 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  Header *outheader = &header;
  FTable *outtable = &ftable;

  if (zcmptype) {
    Header cmpheader;
    FTable cmptable;
    cmptable.header = &cmpheader;
    ok (gfits_compress_table (&ftable, &cmptable, Ntile, zcmptype), "compressed table");

    // XXX TIMER 4 (compress table)
    gettimeofday (&stopTimer, (void *) NULL); 
    dtime = DTIME (stopTimer, startTimer);
    timeSum4 += dtime;
    gettimeofday (&startTimer, (void *) NULL);

    Header rawheader;
    FTable rawtable;
    rawtable.header = &rawheader;
    ok (gfits_uncompress_table (&cmptable, &rawtable), "uncompressed table");
    
    gfits_free_header (&cmpheader);
    gfits_free_table (&cmptable);

    outheader = &rawheader;
    outtable  = &rawtable;
  }

  char type[16];
  int Ncol;
  off_t Nrow;

  // XXX TIMER 5 (uncompress table)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum5 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  char    *VAL_B_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_B", type, &Nrow, &Ncol); ok (!strcmp (type, "byte"),    "read byte    table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  short   *VAL_I_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_I", type, &Nrow, &Ncol); ok (!strcmp (type, "short"),   "read short   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int     *VAL_J_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_J", type, &Nrow, &Ncol); ok (!strcmp (type, "int"),     "read int     table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int64_t *VAL_K_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_K", type, &Nrow, &Ncol); ok (!strcmp (type, "int64_t"), "read int64_t table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  float   *VAL_E_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_E", type, &Nrow, &Ncol); ok (!strcmp (type, "float"),   "read float   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  double  *VAL_D_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_D", type, &Nrow, &Ncol); ok (!strcmp (type, "double"),  "read double  table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");

  // XXX TIMER 6 (get bintable column)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum6 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  // count mismatched values
  int NVAL_B_bad = 0;
  int NVAL_I_bad = 0;
  int NVAL_J_bad = 0;
  int NVAL_K_bad = 0;
  int NVAL_E_bad = 0;
  int NVAL_D_bad = 0;

  for (i = 0; i < Nval; i++) {
    if (VAL_B[i] != VAL_B_t[i]) NVAL_B_bad++;
    if (VAL_I[i] != VAL_I_t[i]) NVAL_I_bad++;
    if (VAL_J[i] != VAL_J_t[i]) NVAL_J_bad++;
    if (VAL_K[i] != VAL_K_t[i]) NVAL_K_bad++;
    if (VAL_E[i] != VAL_E_t[i]) NVAL_E_bad++;
    if (VAL_D[i] != VAL_D_t[i]) NVAL_D_bad++;
  }

  // XXX TIMER 7 (read values)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum7 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  ok (!NVAL_B_bad, "byte    values match (input vs output)");
  ok (!NVAL_I_bad, "short   values match (input vs output)");
  ok (!NVAL_J_bad, "int     values match (input vs output)");
  ok (!NVAL_K_bad, "int64_t values match (input vs output)");
  ok (!NVAL_E_bad, "float   values match (input vs output)");
  ok (!NVAL_D_bad, "double  values match (input vs output)");

  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (zcmptype) {
    gfits_free_header (outheader);
    gfits_free_table (outtable);
  }
  // XXX TIMER 8 (free data)
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum8 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  fprintf (stderr, "out times: %f %f %f %f %f %f %f %f\n", timeSum1, timeSum2, timeSum3, timeSum4, timeSum5, timeSum6, timeSum7, timeSum8);

  return TRUE;
}


