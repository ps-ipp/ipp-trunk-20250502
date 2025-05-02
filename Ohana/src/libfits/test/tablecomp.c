# include <ohana.h>
# include <gfitsio.h>
# include "tap_ohana.h"

int test_compress (char *zcmptype);
int test_compress_empty (char *zcmptype);
int test_compress_fullrange (char *zcmptype);
int test_compress_single_full (char *zcmptype);

char *cmptype[] = {"NONE", "NONE_1", "NONE_2", "GZIP_1", "GZIP_2", "RICE_1", "RICE_ONE", "AUTO", NULL};

int main (void) {
  
  plan_tests (668+428);

  diag ("libfits tablecomp.c tests");

  // test_compress_empty ("GZIP_1");
  // exit (0);

  test_compress (NULL);
  test_compress_fullrange (NULL);

  int i;
  for (i = 0; cmptype[i]; i++) {
    test_compress (cmptype[i]);
    test_compress_fullrange (cmptype[i]);
    test_compress_empty (cmptype[i]);
    ok (ohana_memcheck (TRUE), "no memory corruption");
    ohana_memdump (TRUE);
  }

  return exit_status();
}

int test_compress (char *zcmptype) { // test 2: make a table, compress, uncompress, compare : use compression "NONE"

  Header header;
  FTable ftable;

  diag ("--- starting test_compress with zcmptype %s ---", zcmptype);
  ok (gfits_init_header (&header), "inited the header");
  ok (gfits_init_table (&ftable), "inited the table");

  ok (gfits_create_table_header (&header, "BINTABLE", "TESTDATA"), "created the table header");

  ok (gfits_define_bintable_column (&header, "B", "VAL_B", "gfbyte", "none", 1.0, 0.0), "defined byte column");
  ok (gfits_define_bintable_column (&header, "I", "VAL_I", "short",  "none", 1.0, 0.0), "defined short column");
  ok (gfits_define_bintable_column (&header, "J", "VAL_J", "int",    "none", 1.0, 0.0),  "defined int column");
  ok (gfits_define_bintable_column (&header, "K", "VAL_K", "long",   "none", 1.0, 0.0),  "defined long column");
  ok (gfits_define_bintable_column (&header, "E", "VAL_E", "float",  "degree", 1.0, 0.0),   "defined float column");
  ok (gfits_define_bintable_column (&header, "D", "VAL_D", "double", "degree", 1.0, 0.0),   "defined double column");
  
  // generate the output array that carries the data
  ok (gfits_create_table (&header, &ftable), "created the basic table");

  int Nval = 1000;
  char    *VAL_B;  ALLOCATE (VAL_B, char,    Nval);
  short   *VAL_I;  ALLOCATE (VAL_I, short,   Nval);
  int     *VAL_J;  ALLOCATE (VAL_J, int,     Nval);
  int64_t *VAL_K;  ALLOCATE (VAL_K, int64_t, Nval);
  float   *VAL_E;  ALLOCATE (VAL_E, float,   Nval);
  double  *VAL_D;  ALLOCATE (VAL_D, double,  Nval);
  
  int i;
  for (i = 0; i < Nval; i++) {
    VAL_B[i] = (i % 255) - 128;
    VAL_I[i] = i;
    VAL_J[i] = 10000*i + 100*i;
    VAL_K[i] = 10000*i + 330*i;
    VAL_E[i] = 0.1*i;
    VAL_D[i] = 1.001*i;
  }     

  // add the columns to the output array
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_B", VAL_B, Nval), "set byte   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_I", VAL_I, Nval), "set short  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_J", VAL_J, Nval), "set int    table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_K", VAL_K, Nval), "set long   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_E", VAL_E, Nval), "set float  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_D", VAL_D, Nval), "set double table column");

  Header *outheader = &header;
  FTable *outtable = &ftable;

  if (zcmptype) {
    Header cmpheader;
    FTable cmptable;
    cmptable.header = &cmpheader;
    ok (gfits_compress_table (&ftable, &cmptable, 10, zcmptype), "compressed table");

    if (0) { int ix, iy;
      for (ix = 0; ix < 12; ix++) {
	for (iy = 0; iy < 16; iy++) { 
	  fprintf (stderr, "%x ", (int) cmptable.buffer[ix*16 + iy]);
	}
      }
    }

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

  char    *VAL_B_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_B", type, &Nrow, &Ncol); ok (!strcmp (type, "gfbyte"),  "read byte    table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  short   *VAL_I_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_I", type, &Nrow, &Ncol); ok (!strcmp (type, "short"),   "read short   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int     *VAL_J_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_J", type, &Nrow, &Ncol); ok (!strcmp (type, "int"),     "read int     table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int64_t *VAL_K_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_K", type, &Nrow, &Ncol); ok (!strcmp (type, "int64_t"), "read int64_t table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  float   *VAL_E_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_E", type, &Nrow, &Ncol); ok (!strcmp (type, "float"),   "read float   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  double  *VAL_D_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_D", type, &Nrow, &Ncol); ok (!strcmp (type, "double"),  "read double  table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");

  // count mismatched values
  int NVAL_B_bad = 0;
  int NVAL_I_bad = 0;
  int NVAL_J_bad = 0;
  int NVAL_K_bad = 0;
  int NVAL_E_bad = 0;
  int NVAL_D_bad = 0;

  // for (i = 0; i < Nval; i++) {
  //   fprintf (stderr, "%0llx : %0llx\n", (long long) VAL_K[i], (long long) VAL_K_t[i]);
  // }

  for (i = 0; i < Nval; i++) {
    if (VAL_B[i] != VAL_B_t[i]) NVAL_B_bad++;
    if (VAL_I[i] != VAL_I_t[i]) NVAL_I_bad++;
    if (VAL_J[i] != VAL_J_t[i]) NVAL_J_bad++;
    if (VAL_K[i] != VAL_K_t[i]) NVAL_K_bad++;
    if (VAL_E[i] != VAL_E_t[i]) NVAL_E_bad++;
    if (VAL_D[i] != VAL_D_t[i]) NVAL_D_bad++;
  }

  ok (!NVAL_B_bad, "gfbyte  values match (input vs output)");
  ok (!NVAL_I_bad, "short   values match (input vs output)");
  ok (!NVAL_J_bad, "int     values match (input vs output)");
  ok (!NVAL_K_bad, "int64_t values match (input vs output)");
  ok (!NVAL_E_bad, "float   values match (input vs output)");
  ok (!NVAL_D_bad, "double  values match (input vs output)");

  free (VAL_B);
  free (VAL_I);
  free (VAL_J);
  free (VAL_K);
  free (VAL_E);
  free (VAL_D);

  free (VAL_B_t);
  free (VAL_I_t);
  free (VAL_J_t);
  free (VAL_K_t);
  free (VAL_E_t);
  free (VAL_D_t);

  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (zcmptype) {
    gfits_free_header (outheader);
    gfits_free_table (outtable);
  }
  return TRUE;
}

int test_compress_fullrange (char *zcmptype) { // test 2: make a table, compress, uncompress, compare : use compression "NONE"

  Header header;
  FTable ftable;

  diag ("--- starting test_compress with zcmptype %s ---", zcmptype);
  ok (gfits_init_header (&header), "inited the header");
  ok (gfits_init_table (&ftable), "inited the table");

  ok (gfits_create_table_header (&header, "BINTABLE", "TESTDATA"), "created the table header");

  ok (gfits_define_bintable_column (&header, "B", "VAL_B", "gfbyte", "none",   1.0, 0.0), "defined byte column");
  ok (gfits_define_bintable_column (&header, "I", "VAL_I", "short",  "none",   1.0, 0.0), "defined short column");
  ok (gfits_define_bintable_column (&header, "J", "VAL_J", "int",    "none",   1.0, 0.0), "defined int column");
  ok (gfits_define_bintable_column (&header, "K", "VAL_K", "long",   "none",   1.0, 0.0), "defined long column");
  ok (gfits_define_bintable_column (&header, "E", "VAL_E", "float",  "degree", 1.0, 0.0), "defined float column");
  ok (gfits_define_bintable_column (&header, "D", "VAL_D", "double", "degree", 1.0, 0.0), "defined double column");
  
  // generate the output array that carries the data
  ok (gfits_create_table (&header, &ftable), "created the basic table");

  int Nval = 1000;
  char    *VAL_B;  ALLOCATE (VAL_B, char,    Nval);
  short   *VAL_I;  ALLOCATE (VAL_I, short,   Nval);
  int     *VAL_J;  ALLOCATE (VAL_J, int,     Nval);
  int64_t *VAL_K;  ALLOCATE (VAL_K, int64_t, Nval);
  float   *VAL_E;  ALLOCATE (VAL_E, float,   Nval);
  double  *VAL_D;  ALLOCATE (VAL_D, double,  Nval);
  
  long A = time(NULL);
  srand48(A);

  int i;
  for (i = 0; i < Nval; i++) {
    VAL_B[i] =       0xff & lrand48();
    VAL_I[i] =     0xffff & lrand48();
    VAL_J[i] = 0xffffffff & lrand48();
    VAL_K[i] = (((int64_t)lrand48()) << 32) + (int64_t)lrand48();
    VAL_E[i] = FLT_MAX * (2.0*drand48() - 1.0);
    VAL_D[i] = DBL_MAX * (2.0*drand48() - 1.0);
  }     

  // add the columns to the output array
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_B", VAL_B, Nval), "set byte   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_I", VAL_I, Nval), "set short  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_J", VAL_J, Nval), "set int    table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_K", VAL_K, Nval), "set long   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_E", VAL_E, Nval), "set float  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_D", VAL_D, Nval), "set double table column");

  Header *outheader = &header;
  FTable *outtable = &ftable;

  if (zcmptype) {
    Header cmpheader;
    FTable cmptable;
    cmptable.header = &cmpheader;
    ok (gfits_compress_table (&ftable, &cmptable, 10, zcmptype), "compressed table");

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

  char    *VAL_B_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_B", type, &Nrow, &Ncol); ok (!strcmp (type, "gfbyte"),  "read byte    table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  short   *VAL_I_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_I", type, &Nrow, &Ncol); ok (!strcmp (type, "short"),   "read short   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int     *VAL_J_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_J", type, &Nrow, &Ncol); ok (!strcmp (type, "int"),     "read int     table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int64_t *VAL_K_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_K", type, &Nrow, &Ncol); ok (!strcmp (type, "int64_t"), "read int64_t table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  float   *VAL_E_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_E", type, &Nrow, &Ncol); ok (!strcmp (type, "float"),   "read float   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  double  *VAL_D_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_D", type, &Nrow, &Ncol); ok (!strcmp (type, "double"),  "read double  table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");

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

  ok (!NVAL_B_bad, "gfbyte  values match (input vs output)");
  ok (!NVAL_I_bad, "short   values match (input vs output)");
  ok (!NVAL_J_bad, "int     values match (input vs output)");
  ok (!NVAL_K_bad, "int64_t values match (input vs output)");
  ok (!NVAL_E_bad, "float   values match (input vs output)");
  ok (!NVAL_D_bad, "double  values match (input vs output)");

  free (VAL_B);
  free (VAL_I);
  free (VAL_J);
  free (VAL_K);
  free (VAL_E);
  free (VAL_D);

  free (VAL_B_t);
  free (VAL_I_t);
  free (VAL_J_t);
  free (VAL_K_t);
  free (VAL_E_t);
  free (VAL_D_t);

  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (zcmptype) {
    gfits_free_header (outheader);
    gfits_free_table (outtable);
  }
  return TRUE;
}

int test_compress_single_full (char *zcmptype) { // test 2: make a table, compress, uncompress, compare : use compression "NONE"

  Header header;
  FTable ftable;

  diag ("--- starting test_compress with zcmptype %s ---", zcmptype);
  ok (gfits_init_header (&header), "inited the header");
  ok (gfits_init_table (&ftable), "inited the table");

  ok (gfits_create_table_header (&header, "BINTABLE", "TESTDATA"), "created the table header");

  ok (gfits_define_bintable_column (&header, "I", "SEQ", "seq", "none", 1.0, 0.0), "defined short column");
  
  // generate the output array that carries the data
  ok (gfits_create_table (&header, &ftable), "created the basic table");

  int Nval = 1000;
  short *SEQ;  ALLOCATE (SEQ, short,  Nval);
  
  int i;
  for (i = 0; i < Nval; i++) {
    SEQ[i] = i;
  }     

  // add the columns to the output array
  ok (gfits_set_bintable_column (&header, &ftable, "SEQ", SEQ, Nval), "set short  table column");

  Header *outheader = &header;
  FTable *outtable = &ftable;

  if (zcmptype) {
    Header cmpheader;
    FTable cmptable;
    cmptable.header = &cmpheader;
    ok (gfits_compress_table (&ftable, &cmptable, 0, zcmptype), "compressed table");

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

  short  *SEQ_t = gfits_get_bintable_column_data (outheader, outtable, "SEQ", type, &Nrow, &Ncol); ok (!strcmp (type, "short"),  "read short  table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");

  // count mismatched values
  int Nseq = 0;
  for (i = 0; i < Nval; i++) {
    if (SEQ[i] != SEQ_t[i]) Nseq++;
  }

  ok (!Nseq,   "short  values match (input vs output)");

  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (zcmptype) {
    gfits_free_header (outheader);
    gfits_free_table (outtable);
  }
  return TRUE;
}

int test_compress_empty (char *zcmptype) { // test 2: make a table, compress, uncompress, compare : use compression "NONE"

  Header header;
  FTable ftable;

  diag ("--- starting test_compress with zcmptype %s ---", zcmptype);
  ok (gfits_init_header (&header), "inited the header");
  ok (gfits_init_table (&ftable), "inited the table");

  ok (gfits_create_table_header (&header, "BINTABLE", "TESTDATA"), "created the table header");

  ok (gfits_define_bintable_column (&header, "B", "VAL_B", "gfbyte", "none", 1.0, 0.0), "defined byte column");
  ok (gfits_define_bintable_column (&header, "I", "VAL_I", "short",  "none", 1.0, 0.0), "defined short column");
  ok (gfits_define_bintable_column (&header, "J", "VAL_J", "int",    "none", 1.0, 0.0),  "defined int column");
  ok (gfits_define_bintable_column (&header, "K", "VAL_K", "long",   "none", 1.0, 0.0),  "defined long column");
  ok (gfits_define_bintable_column (&header, "E", "VAL_E", "float",  "degree", 1.0, 0.0),   "defined float column");
  ok (gfits_define_bintable_column (&header, "D", "VAL_D", "double", "degree", 1.0, 0.0),   "defined double column");
  
  // generate the output array that carries the data
  ok (gfits_create_table (&header, &ftable), "created the basic table");

  int Nval = 0;
  char    *VAL_B;  ALLOCATE (VAL_B, char,    Nval);
  short   *VAL_I;  ALLOCATE (VAL_I, short,   Nval);
  int     *VAL_J;  ALLOCATE (VAL_J, int,     Nval);
  int64_t *VAL_K;  ALLOCATE (VAL_K, int64_t, Nval);
  float   *VAL_E;  ALLOCATE (VAL_E, float,   Nval);
  double  *VAL_D;  ALLOCATE (VAL_D, double,  Nval);
  
  int i;
  for (i = 0; i < Nval; i++) {
    VAL_B[i] = (i % 255) - 128;
    VAL_I[i] = i;
    VAL_J[i] = 10000*i + 100*i;
    VAL_K[i] = 10000*i + 330*i;
    VAL_E[i] = 0.1*i;
    VAL_D[i] = 1.001*i;
  }     

  // add the columns to the output array
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_B", VAL_B, Nval), "set byte   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_I", VAL_I, Nval), "set short  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_J", VAL_J, Nval), "set int    table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_K", VAL_K, Nval), "set long   table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_E", VAL_E, Nval), "set float  table column");
  ok (gfits_set_bintable_column (&header, &ftable, "VAL_D", VAL_D, Nval), "set double table column");

  Header *outheader = &header;
  FTable *outtable = &ftable;

  if (zcmptype) {
    Header cmpheader;
    FTable cmptable;
    cmptable.header = &cmpheader;
    ok (gfits_compress_table (&ftable, &cmptable, 0, zcmptype), "compressed table");

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

  char    *VAL_B_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_B", type, &Nrow, &Ncol); ok (!strcmp (type, "gfbyte"),  "read byte    table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  short   *VAL_I_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_I", type, &Nrow, &Ncol); ok (!strcmp (type, "short"),   "read short   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int     *VAL_J_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_J", type, &Nrow, &Ncol); ok (!strcmp (type, "int"),     "read int     table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  int64_t *VAL_K_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_K", type, &Nrow, &Ncol); ok (!strcmp (type, "int64_t"), "read int64_t table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  float   *VAL_E_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_E", type, &Nrow, &Ncol); ok (!strcmp (type, "float"),   "read float   table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");
  double  *VAL_D_t = gfits_get_bintable_column_data (outheader, outtable, "VAL_D", type, &Nrow, &Ncol); ok (!strcmp (type, "double"),  "read double  table column"); ok (Nrow == Nval, "right number of rows"); ok (Ncol == 1, "right number of cols");

  // count mismatched values
  int NVAL_B_bad = 0;
  int NVAL_I_bad = 0;
  int NVAL_J_bad = 0;
  int NVAL_K_bad = 0;
  int NVAL_E_bad = 0;
  int NVAL_D_bad = 0;

  // for (i = 0; i < Nval; i++) {
  //   fprintf (stderr, "%0llx : %0llx\n", (long long) VAL_K[i], (long long) VAL_K_t[i]);
  // }

  for (i = 0; i < Nval; i++) {
    if (VAL_B[i] != VAL_B_t[i]) NVAL_B_bad++;
    if (VAL_I[i] != VAL_I_t[i]) NVAL_I_bad++;
    if (VAL_J[i] != VAL_J_t[i]) NVAL_J_bad++;
    if (VAL_K[i] != VAL_K_t[i]) NVAL_K_bad++;
    if (VAL_E[i] != VAL_E_t[i]) NVAL_E_bad++;
    if (VAL_D[i] != VAL_D_t[i]) NVAL_D_bad++;
  }

  ok (!NVAL_B_bad, "gfbyte  values match (input vs output)");
  ok (!NVAL_I_bad, "short   values match (input vs output)");
  ok (!NVAL_J_bad, "int     values match (input vs output)");
  ok (!NVAL_K_bad, "int64_t values match (input vs output)");
  ok (!NVAL_E_bad, "float   values match (input vs output)");
  ok (!NVAL_D_bad, "double  values match (input vs output)");

  free (VAL_B);
  free (VAL_I);
  free (VAL_J);
  free (VAL_K);
  free (VAL_E);
  free (VAL_D);

  free (VAL_B_t);
  free (VAL_I_t);
  free (VAL_J_t);
  free (VAL_K_t);
  free (VAL_E_t);
  free (VAL_D_t);

  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (zcmptype) {
    gfits_free_header (outheader);
    gfits_free_table (outtable);
  }
  return TRUE;
}


