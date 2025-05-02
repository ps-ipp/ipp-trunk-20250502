# include <ohana.h>
# include <gfitsio.h>
# include "tap_ohana.h"

# define MEM_VERBOSE FALSE

int test_compress (int bitpix, char *zcmptype);
int test_compress_fullrange (int bitpix, char *zcmptype);
int test_compress_fulltile (int bitpix, char *zcmptype);

// XXX FULL SET : char *cmptype[] = {"NONE", "NONE_2", "GZIP_1", "GZIP_2", "PLIO_1", "RICE_1", "RICE_ONE", "HCOMPRESS_1", NULL};
char *cmptype[] = {"NONE", "NONE_1", "NONE_2", "GZIP_1", "GZIP_2", "RICE_1", "RICE_ONE", NULL};
// XXX NOT WORKING: char *cmptype[] = {"HCOMPRESS_1", NULL};
int bitpix[] = {8, 16, 32, -32, -64, 0};

// char *cmptype[] = {"NONE", NULL};
// int bitpix[] = {16, 0};

static int NX = 10;
static int NY = 10;

int main (void) {
  
  plan_tests (3*3*234);

  diag ("libfits imagecomp.c tests");

  int i, j;
  for (i = 0; cmptype[i]; i++) {
    for (j = 0; bitpix[j]; j++) {
      if (!strcasecmp (cmptype[i], "RICE_1") && (bitpix[j] < 0)) continue;
      if (!strcasecmp (cmptype[i], "RICE_ONE") && (bitpix[j] < 0)) continue;
      test_compress (bitpix[j], cmptype[i]);
      test_compress_fulltile (bitpix[j], cmptype[i]);
      test_compress_fullrange (bitpix[j], cmptype[i]);
      ok (ohana_memcheck (MEM_VERBOSE), "no memory corruption");
    }
  }

  NX = 100; NY = 33;
  for (i = 0; cmptype[i]; i++) {
    for (j = 0; bitpix[j]; j++) {
      if (!strcasecmp (cmptype[i], "RICE_1") && (bitpix[j] < 0)) continue;
      if (!strcasecmp (cmptype[i], "RICE_ONE") && (bitpix[j] < 0)) continue;
      test_compress (bitpix[j], cmptype[i]);
      test_compress_fulltile (bitpix[j], cmptype[i]);
      test_compress_fullrange (bitpix[j], cmptype[i]);
      ok (ohana_memcheck (MEM_VERBOSE), "no memory corruption");
    }
  }

  NX = 357; NY = 245;
  for (i = 0; cmptype[i]; i++) {
    for (j = 0; bitpix[j]; j++) {
      if (!strcasecmp (cmptype[i], "RICE_1") && (bitpix[j] < 0)) continue;
      if (!strcasecmp (cmptype[i], "RICE_ONE") && (bitpix[j] < 0)) continue;
      test_compress (bitpix[j], cmptype[i]);
      test_compress_fulltile (bitpix[j], cmptype[i]);
      test_compress_fullrange (bitpix[j], cmptype[i]);
      ok (ohana_memcheck (MEM_VERBOSE), "no memory corruption");
    }
  }
  return exit_status();
}

int test_compress (int bitpix, char *zcmptype) { // make a table, compress, uncompress, compare : use compression zcmptype

  Header rawheader;
  Matrix rawmatrix;
  Header outheader;
  Matrix outmatrix;

  Header theader;
  FTable ftable;
  ftable.header = &theader;

  diag ("--- starting test_compress with bitpix %d, zcmptype %s ---", bitpix, zcmptype);
  ok (gfits_init_header (&rawheader), "init'ed the raw header");
  ok (gfits_init_matrix (&rawmatrix), "init'ed the raw matrix");
  ok (gfits_init_header (&outheader), "init'ed the out header");
  ok (gfits_init_matrix (&outmatrix), "init'ed the out matrix");

  /* assign the necessary internal values */
  rawheader.bitpix   = bitpix;
  rawheader.Naxes = 2;
  rawheader.Naxis[0] = NX;
  rawheader.Naxis[1] = NY;

  /* create the appropriate header and matrix */
  ok (gfits_create_header (&rawheader),          "created header");
  ok (gfits_create_matrix (&rawheader, &rawmatrix), "created matrix");
  
  char   *rawdata_char   = (char   *) rawmatrix.buffer;
  short  *rawdata_short  = (short  *) rawmatrix.buffer;
  int    *rawdata_int    = (int    *) rawmatrix.buffer;
  float  *rawdata_float  = (float  *) rawmatrix.buffer;
  double *rawdata_double = (double *) rawmatrix.buffer;
  
  int ix, iy;
  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      int IY = iy + 1;
      switch (bitpix) {
	case   8: rawdata_char  [ix + NX*iy] = ix + IY*IY; break;
	case  16: rawdata_short [ix + NX*iy] = ix + IY*IY; break;
	case  32: rawdata_int   [ix + NX*iy] = ix + IY*IY; break;
	case -32: rawdata_float [ix + NX*iy] = ix + IY*IY; break;
	case -64: rawdata_double[ix + NX*iy] = ix + IY*IY; break;
	default: myAbort ("bad bitpix value");
      }
    }
  }     

  // int Ztile[2] = {NX, NY};
  // ok (gfits_compress_image (&rawheader, &rawmatrix, &ftable, (int *) &Ztile, zcmptype), "compressed image");
  ok (gfits_compress_image (&rawheader, &rawmatrix, &ftable, NULL, zcmptype), "compressed image");
  ok (gfits_uncompress_image (&outheader, &outmatrix, &ftable), "uncompressed image");
  
  gfits_free_header (&theader);
  gfits_free_table (&ftable);
  
  int Nbad = 0;

  char   *outdata_char   = (char   *) outmatrix.buffer;
  short  *outdata_short  = (short  *) outmatrix.buffer;
  int    *outdata_int    = (int    *) outmatrix.buffer;
  float  *outdata_float  = (float  *) outmatrix.buffer;
  double *outdata_double = (double *) outmatrix.buffer;

  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      switch (bitpix) {
	case   8: if (rawdata_char  [ix + NX*iy] != outdata_char  [ix + NX*iy]) Nbad ++; break;
	case  16: if (rawdata_short [ix + NX*iy] != outdata_short [ix + NX*iy]) Nbad ++; break;
	case  32: if (rawdata_int   [ix + NX*iy] != outdata_int   [ix + NX*iy]) Nbad ++; break;
	case -32: if (rawdata_float [ix + NX*iy] != outdata_float [ix + NX*iy]) Nbad ++; break;
	case -64: if (rawdata_double[ix + NX*iy] != outdata_double[ix + NX*iy]) Nbad ++; break;
	default: myAbort ("bad bitpix value");
      }
    }
  }     
  
  // diag ("--- Image has a total of %d pixels of which %d are bad ---", NX*NY, Nbad);
  ok (!Nbad, "all image pixels match");

  gfits_free_header (&rawheader);
  gfits_free_matrix (&rawmatrix);

  gfits_free_header (&outheader);
  gfits_free_matrix (&outmatrix);
  return TRUE;
}

int test_compress_fulltile (int bitpix, char *zcmptype) { // make a table, compress, uncompress, compare : use compression zcmptype

  Header rawheader;
  Matrix rawmatrix;
  Header outheader;
  Matrix outmatrix;

  Header theader;
  FTable ftable;
  ftable.header = &theader;

  diag ("--- starting test_compress with bitpix %d, zcmptype %s ---", bitpix, zcmptype);
  ok (gfits_init_header (&rawheader), "init'ed the raw header");
  ok (gfits_init_matrix (&rawmatrix), "init'ed the raw matrix");
  ok (gfits_init_header (&outheader), "init'ed the out header");
  ok (gfits_init_matrix (&outmatrix), "init'ed the out matrix");

  /* assign the necessary internal values */
  rawheader.bitpix   = bitpix;
  rawheader.Naxes = 2;
  rawheader.Naxis[0] = NX;
  rawheader.Naxis[1] = NY;

  /* create the appropriate header and matrix */
  ok (gfits_create_header (&rawheader),          "created header");
  ok (gfits_create_matrix (&rawheader, &rawmatrix), "created matrix");
  
  char   *rawdata_char   = (char   *) rawmatrix.buffer;
  short  *rawdata_short  = (short  *) rawmatrix.buffer;
  int    *rawdata_int    = (int    *) rawmatrix.buffer;
  float  *rawdata_float  = (float  *) rawmatrix.buffer;
  double *rawdata_double = (double *) rawmatrix.buffer;
  
  int ix, iy;
  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      int IY = iy + 1;
      switch (bitpix) {
	case   8: rawdata_char  [ix + NX*iy] = ix + IY*IY; break;
	case  16: rawdata_short [ix + NX*iy] = ix + IY*IY; break;
	case  32: rawdata_int   [ix + NX*iy] = ix + IY*IY; break;
	case -32: rawdata_float [ix + NX*iy] = ix + IY*IY; break;
	case -64: rawdata_double[ix + NX*iy] = ix + IY*IY; break;
	default: myAbort ("bad bitpix value");
      }
    }
  }     

  unsigned long int Ztile[2] = {NX, NY};
  ok (gfits_compress_image (&rawheader, &rawmatrix, &ftable, (unsigned long int *) &Ztile, zcmptype), "compressed image");
  // ok (gfits_compress_image (&rawheader, &rawmatrix, &ftable, NULL, zcmptype), "compressed image");
  ok (gfits_uncompress_image (&outheader, &outmatrix, &ftable), "uncompressed image");
  
  gfits_free_header (&theader);
  gfits_free_table (&ftable);
  
  int Nbad = 0;

  char   *outdata_char   = (char   *) outmatrix.buffer;
  short  *outdata_short  = (short  *) outmatrix.buffer;
  int    *outdata_int    = (int    *) outmatrix.buffer;
  float  *outdata_float  = (float  *) outmatrix.buffer;
  double *outdata_double = (double *) outmatrix.buffer;

  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      switch (bitpix) {
	case   8: if (rawdata_char  [ix + NX*iy] != outdata_char  [ix + NX*iy]) Nbad ++; break;
	case  16: if (rawdata_short [ix + NX*iy] != outdata_short [ix + NX*iy]) Nbad ++; break;
	case  32: if (rawdata_int   [ix + NX*iy] != outdata_int   [ix + NX*iy]) Nbad ++; break;
	case -32: if (rawdata_float [ix + NX*iy] != outdata_float [ix + NX*iy]) Nbad ++; break;
	case -64: if (rawdata_double[ix + NX*iy] != outdata_double[ix + NX*iy]) Nbad ++; break;
	default: myAbort ("bad bitpix value");
      }
    }
  }     
  
  ok (!Nbad, "all image pixels match");

  gfits_free_header (&rawheader);
  gfits_free_matrix (&rawmatrix);

  gfits_free_header (&outheader);
  gfits_free_matrix (&outmatrix);
  return TRUE;
}

int test_compress_fullrange (int bitpix, char *zcmptype) { // make a table, compress, uncompress, compare : use compression zcmptype

  Header rawheader;
  Matrix rawmatrix;
  Header outheader;
  Matrix outmatrix;

  Header theader;
  FTable ftable;
  ftable.header = &theader;

  diag ("--- starting test_compress with bitpix %d, zcmptype %s ---", bitpix, zcmptype);
  ok (gfits_init_header (&rawheader), "init'ed the raw header");
  ok (gfits_init_matrix (&rawmatrix), "init'ed the raw matrix");
  ok (gfits_init_header (&outheader), "init'ed the out header");
  ok (gfits_init_matrix (&outmatrix), "init'ed the out matrix");

  /* assign the necessary internal values */
  rawheader.bitpix   = bitpix;
  rawheader.Naxes = 2;
  rawheader.Naxis[0] = NX;
  rawheader.Naxis[1] = NY;

  /* create the appropriate header and matrix */
  ok (gfits_create_header (&rawheader),          "created header");
  ok (gfits_create_matrix (&rawheader, &rawmatrix), "created matrix");
  
  char   *rawdata_char   = (char   *) rawmatrix.buffer;
  short  *rawdata_short  = (short  *) rawmatrix.buffer;
  int    *rawdata_int    = (int    *) rawmatrix.buffer;
  float  *rawdata_float  = (float  *) rawmatrix.buffer;
  double *rawdata_double = (double *) rawmatrix.buffer;
  
  long A = time(NULL);
  srand48(A);
  // srand48(1);

  int ix, iy;
  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      switch (bitpix) {
	case   8: rawdata_char  [ix + NX*iy] =       0xff & lrand48(); break;
	case  16: rawdata_short [ix + NX*iy] =     0xffff & lrand48(); break;
	case  32: rawdata_int   [ix + NX*iy] = 0xffffffff & lrand48(); break;
	case -32: rawdata_float [ix + NX*iy] = FLT_MAX * (2.0*drand48() - 1.0); break;
	case -64: rawdata_double[ix + NX*iy] = DBL_MAX * (2.0*drand48() - 1.0); break;
	default: myAbort ("bad bitpix value");
      }
    }
  }     

  // int Ztile[2] = {NX, NY};
  ok (gfits_compress_image (&rawheader, &rawmatrix, &ftable, NULL, zcmptype), "compressed image");

  ok (gfits_uncompress_image (&outheader, &outmatrix, &ftable), "uncompressed image");
  
  gfits_free_header (&theader);
  gfits_free_table (&ftable);
  
  int Nbad = 0;

  char   *outdata_char   = (char   *) outmatrix.buffer;
  short  *outdata_short  = (short  *) outmatrix.buffer;
  int    *outdata_int    = (int    *) outmatrix.buffer;
  float  *outdata_float  = (float  *) outmatrix.buffer;
  double *outdata_double = (double *) outmatrix.buffer;

  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      switch (bitpix) {
	case   8: if (rawdata_char  [ix + NX*iy] != outdata_char  [ix + NX*iy]) Nbad ++; break;
	case  16: if (rawdata_short [ix + NX*iy] != outdata_short [ix + NX*iy]) Nbad ++; break;
	case  32: if (rawdata_int   [ix + NX*iy] != outdata_int   [ix + NX*iy]) Nbad ++; break;
	case -32: if (rawdata_float [ix + NX*iy] != outdata_float [ix + NX*iy]) Nbad ++; break;
	case -64: if (rawdata_double[ix + NX*iy] != outdata_double[ix + NX*iy]) Nbad ++; break;
	default: myAbort ("bad bitpix value");
      }
    }
  }     
  
  ok (!Nbad, "all image pixels match");

  gfits_free_header (&rawheader);
  gfits_free_matrix (&rawmatrix);

  gfits_free_header (&outheader);
  gfits_free_matrix (&outmatrix);
  return TRUE;
}


// fprintf (stderr, "%2d %2d : 0x%08x vs 0x%08x vs 0x%08x : %d\n", ix, iy, rawdata_int   [ix + NX*iy], outdata_int   [ix + NX*iy], tmpbuff[ix + NX*iy], outdata_int   [ix + NX*iy] - rawdata_int   [ix + NX*iy]);
