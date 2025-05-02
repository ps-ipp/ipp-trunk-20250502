# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <zlib.h>

# define NSRC 100
# define NTGT 200

int main (int argc, char **argv) {
  
  int i, err;
  char srcbuf[NSRC], tgtbuf[NTGT], outbuf[NTGT];
  
  { 
    z_stream zdn;
    zdn.next_in = srcbuf;
    zdn.avail_in = NSRC;
    zdn.next_out = tgtbuf;
    zdn.avail_out = NTGT;
  
    zdn.zalloc = Z_NULL;
    zdn.zfree  = Z_NULL;
    zdn.opaque = Z_NULL;

    for (i = 0; i < NSRC; i++) srcbuf[i] = i; 
    memset (tgtbuf, 0, NTGT);
    memset (outbuf, 0, NTGT);

    // the '5' is the compression level: make this a user argument
    err = deflateInit(&zdn, 5);
    if (err != Z_OK) { fprintf (stderr, "error 1: %d vs %d\n", err, Z_OK); exit (1); }
  
    err = deflate(&zdn, Z_FINISH);
    if (err != Z_STREAM_END) { fprintf (stderr, "error 2: %d vs %d\n", err, Z_STREAM_END); exit (2); }

    assert (zdn.total_out <= NTGT);

    fprintf (stderr, "result: %d bytes\n", (int) zdn.total_out);
    for (i = 0; i < zdn.total_out; i++) {
      fprintf (stderr, "%d : 0x%02hhx\n", i, tgtbuf[i]);
    }
  }

  {
    z_stream zup;
    zup.next_in = tgtbuf;
    zup.avail_in = NTGT;
    zup.next_out = outbuf;
    zup.avail_out = NTGT;
  
    zup.zalloc = Z_NULL;
    zup.zfree  = Z_NULL;
    zup.opaque = Z_NULL;

    // the '5' is the compression level: make this a user argument
    err = inflateInit(&zup);
    if (err != Z_OK) { fprintf (stderr, "error 1: %d vs %d\n", err, Z_OK); exit (1); }
  
    err = inflate(&zup, Z_FINISH);
    if (err != Z_STREAM_END) { fprintf (stderr, "error 2: %d vs %d\n", err, Z_STREAM_END); exit (2); }

    assert (zup.total_out <= NTGT);

    fprintf (stderr, "result: %d bytes\n", (int) zup.total_out);
    for (i = 0; i < zup.total_out; i++) {
      fprintf (stderr, "%d : 0x%02hhx : 0x%02hhx\n", i, outbuf[i], srcbuf[i]);
      assert (outbuf[i] == srcbuf[i]);
    }
  }

  exit (0);
}
