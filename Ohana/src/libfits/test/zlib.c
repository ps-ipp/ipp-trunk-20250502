# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# include "tap_ohana.h"

# define VERBOSE 0
# define NSRC 10
# define NTGT 20
# define NOUT 125

# define SWAP_DBLE { \
  char tmp; \
  tmp = Pout[0]; Pout[0] = Pout[7]; Pout[7] = tmp; \
  tmp = Pout[1]; Pout[1] = Pout[6]; Pout[6] = tmp; \
  tmp = Pout[2]; Pout[2] = Pout[5]; Pout[5] = tmp; \
  tmp = Pout[3]; Pout[3] = Pout[4]; Pout[4] = tmp; }

int main (void) {
  
  plan_tests (14);

  diag ("libfits zlib.c tests");

  diag ("zlib version: %s", ZLIB_VERSION);
  diag ("zlib vernum:  %x", ZLIB_VERNUM);

  int i, err;
  double srcbuf[NSRC], tgtbuf[NTGT], outbuf[NOUT];
  
  { 
    z_stream zdn;
    zdn.next_in = (unsigned char *) srcbuf;
    zdn.avail_in = NSRC * sizeof(double);
    zdn.next_out = (unsigned char *) tgtbuf;
    zdn.avail_out = NTGT * sizeof(double);
  
    zdn.zalloc = Z_NULL;
    zdn.zfree  = Z_NULL;
    zdn.opaque = Z_NULL;

    for (i = 0; i < NSRC; i++) srcbuf[i] = 1.001*(i + 10); 
    memset (tgtbuf, 0, NTGT * sizeof(double));
    memset (outbuf, 0, NOUT * sizeof(double));

    for (i = 0; i < NSRC; i++) {
      char *Pout = (char *) &srcbuf[i];
      SWAP_DBLE;
    }

    char *rawdata = (char *) srcbuf;
    if (VERBOSE) fprintf (stderr, "inp: ");
    for (i = 0; VERBOSE && (i < NSRC*8); i++) {
      fprintf (stderr, "0x%02hhx ", rawdata[i]);
    }
    if (VERBOSE) fprintf (stderr, "\n");

    // the '5' is the compression level: make this a user argument
    err = deflateInit(&zdn, 5);
    if (VERBOSE) fprintf (stderr, "inp buffers cmp 0: %d => %d => %d\n", zdn.avail_in, zdn.avail_out, (int) zdn.total_out);
    ok (err == Z_OK, "result is Z_OK: %d vs %d", err, Z_OK);
  
    err = deflate(&zdn, Z_FINISH);
    if (VERBOSE) fprintf (stderr, "out buffers cmp 1: %d => %d => %d\n", zdn.avail_in, zdn.avail_out, (int) zdn.total_out);
    ok (err == Z_STREAM_END, "result is Z_STREAM_END: %d vs %d", err, Z_STREAM_END);

    // dump out the data which failed to uncompress (and the input data)
    char *cmpdata = (char *) tgtbuf;
    if (VERBOSE) fprintf (stderr, "out: ");
    for (i = 0; VERBOSE && (i < zdn.total_out); i++) {
      fprintf (stderr, "0x%02hhx ", cmpdata[i]);
    }
    if (VERBOSE) fprintf (stderr, "\n");

    ok (zdn.total_out <= NTGT*sizeof(double), "output compressed size is good");

    char *tgtdata = (char *) tgtbuf;
    if (VERBOSE) fprintf (stderr, "result: %d bytes\n", (int) zdn.total_out);
    for (i = 0; VERBOSE && i < zdn.total_out; i++) {
      fprintf (stderr, "%d : 0x%02hhx\n", i, tgtdata[i]);
    }
  }

  {
    z_stream zup;
    zup.next_in = (unsigned char *) tgtbuf;
    zup.avail_in = NTGT * sizeof(double);
    zup.next_out = (unsigned char *) outbuf;
    zup.avail_out = NOUT * sizeof(double);
  
    zup.zalloc = Z_NULL;
    zup.zfree  = Z_NULL;
    zup.opaque = Z_NULL;

    // the '5' is the compression level: make this a user argument
    err = inflateInit(&zup);
    if (VERBOSE) fprintf (stderr, "out buffers unc 0: %d => %d => %d\n", zup.avail_in, zup.avail_out, (int) zup.total_out);
    ok (err == Z_OK, "result is Z_OK: %d vs %d", err, Z_OK);
  
    err = inflate(&zup, Z_FINISH);
    if (VERBOSE) fprintf (stderr, "out buffers unc 1: %d => %d => %d\n", zup.avail_in, zup.avail_out, (int) zup.total_out);
    ok (err == Z_STREAM_END, "result is Z_STREAM_END: %d vs %d", err, Z_STREAM_END);

    char *outdata = (char *) outbuf;
    if (VERBOSE) fprintf (stderr, "unc: ");
    for (i = 0; VERBOSE && (i < zup.total_out); i++) {
      fprintf (stderr, "0x%02hhx ", outdata[i]);
    }
    if (VERBOSE) fprintf (stderr, "\n");

    ok (zup.total_out <= NTGT*sizeof(double), "uncompressed size is good");

    int Nbad = 0;
    for (i = 0; 0 && i < zup.total_out / sizeof(double); i++) {
      if (outbuf[i] != srcbuf[i]) Nbad++;
    }
    ok (Nbad == 0, "no mismatched bytes");
  }

  /**** TEST 2 : use gzip headers ****/
  { 
    z_stream zdn;
    zdn.next_in = (unsigned char *) srcbuf;
    zdn.avail_in = NSRC * sizeof(double);
    zdn.next_out = (unsigned char *) tgtbuf;
    zdn.avail_out = NTGT * sizeof(double);
  
    zdn.zalloc = Z_NULL;
    zdn.zfree  = Z_NULL;
    zdn.opaque = Z_NULL;

    for (i = 0; i < NSRC; i++) srcbuf[i] = 1.001*(i + 10); 
    memset (tgtbuf, 0, NTGT * sizeof(double));
    memset (outbuf, 0, NOUT * sizeof(double));

    for (i = 0; i < NSRC; i++) {
      char *Pout = (char *) &srcbuf[i];
      SWAP_DBLE;
    }

    char *rawdata = (char *) srcbuf;
    if (VERBOSE) fprintf (stderr, "inp: ");
    for (i = 0; VERBOSE && (i < NSRC*8); i++) {
      fprintf (stderr, "0x%02hhx ", rawdata[i]);
    }
    if (VERBOSE) fprintf (stderr, "\n");

    // the '5' is the compression level: make this a user argument
    // windowBits = 31 = (15 + 16) = (2^15 window bits) + (create a gzip stream)
    err = deflateInit2(&zdn, 1, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
    if (VERBOSE) fprintf (stderr, "inp buffers cmp 0: %d => %d => %d\n", zdn.avail_in, zdn.avail_out, (int) zdn.total_out);
    ok (err == Z_OK, "result is Z_OK: %d vs %d", err, Z_OK);
  
    err = deflate(&zdn, Z_FINISH);
    if (VERBOSE) fprintf (stderr, "out buffers cmp 1: %d => %d => %d\n", zdn.avail_in, zdn.avail_out, (int) zdn.total_out);
    ok (err == Z_STREAM_END, "result is Z_STREAM_END: %d vs %d", err, Z_STREAM_END);

    // dump out the data which failed to uncompress (and the input data)
    char *cmpdata = (char *) tgtbuf;
    if (VERBOSE) fprintf (stderr, "out: ");
    for (i = 0; VERBOSE && (i < zdn.total_out); i++) {
      fprintf (stderr, "0x%02hhx ", cmpdata[i]);
    }
    if (VERBOSE) fprintf (stderr, "\n");

    ok (zdn.total_out <= NTGT*sizeof(double), "output compressed size is good");

    char *tgtdata = (char *) tgtbuf;
    if (VERBOSE) fprintf (stderr, "result: %d bytes\n", (int) zdn.total_out);
    for (i = 0; VERBOSE && (i < zdn.total_out); i++) {
      fprintf (stderr, "%d : 0x%02hhx\n", i, tgtdata[i]);
    }
  }

  {
    z_stream zup;
    zup.next_in = (unsigned char *) tgtbuf;
    zup.avail_in = NTGT * sizeof(double);
    zup.next_out = (unsigned char *) outbuf;
    zup.avail_out = NOUT * sizeof(double);
  
    zup.zalloc = Z_NULL;
    zup.zfree  = Z_NULL;
    zup.opaque = Z_NULL;

    // a value of 32 tells inflate to use the window size used in the compression AND to look for both zlib and gzip headers
    // windowBits = 47 = (15 + 32) = (2^15 window bits) + (test for either gzip or zlib streams)
    err = inflateInit2(&zup, 47);
    if (VERBOSE) fprintf (stderr, "out buffers unc 0: %d => %d => %d\n", zup.avail_in, zup.avail_out, (int) zup.total_out);
    ok (err == Z_OK, "inflateInit2 result is Z_OK: %d vs %d", err, Z_OK);
  
    err = inflate(&zup, Z_FINISH);
    if (VERBOSE) fprintf (stderr, "out buffers unc 1: %d => %d => %d\n", zup.avail_in, zup.avail_out, (int) zup.total_out);
    ok (err == Z_STREAM_END, "inflate result is Z_STREAM_END: %d vs %d", err, Z_STREAM_END);

    char *outdata = (char *) outbuf;
    if (VERBOSE) fprintf (stderr, "unc: ");
    for (i = 0; VERBOSE && (i < zup.total_out); i++) {
      fprintf (stderr, "0x%02hhx ", outdata[i]);
    }
    if (VERBOSE) fprintf (stderr, "\n");

    ok (zup.total_out <= NTGT*sizeof(double), "uncompressed size is good");

    int Nbad = 0;
    for (i = 0; 0 && i < zup.total_out / sizeof(double); i++) {
      if (outbuf[i] != srcbuf[i]) Nbad++;
    }
    ok (Nbad == 0, "no mismatched bytes");
  }

  return exit_status();
}
