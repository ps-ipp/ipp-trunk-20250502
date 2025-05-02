# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# define EXTRA_VERBOSE 0

/** the two functions in this file re-implement the uncompress function of zlib.  
    For reasons I don't understand, it seems that I need to parse the header 
    segment of the compressed data myself rather than ship it to 'uncompress'.  
    It could be that CFITSIO is doing something bad, or it could be my 
    misunderstanding **/

/* zlib.h -- interface of the 'zlib' general purpose compression library
   version 1.2.3, July 18th, 2005

   Copyright (C) 1995-2005 Jean-loup Gailly and Mark Adler

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   Jean-loup Gailly        Mark Adler
   jloup@gzip.org          madler@alumni.caltech.edu


   The data format used by the zlib library is described by RFCs (Request for
   Comments) 1950 to 1952 in the files http://www.ietf.org/rfc/rfc1950.txt
   (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
*/

/*
  The 'zlib' compression library provides in-memory compression and
  decompression functions, including integrity checks of the uncompressed
  data.  This version of the library supports only one compression method
  (deflation) but other algorithms will be added later and will have the same
  stream interface.

  Compression can be done in a single step if the buffers are large
  enough (for example if an input file is mmap'ed), or can be done by
  repeated calls of the compression function.  In the latter case, the
  application must provide more input and/or consume the output
  (providing more output space) before each call.

  The compressed data format used by default by the in-memory functions is
  the zlib format, which is a zlib wrapper documented in RFC 1950, wrapped
  around a deflate stream, which is itself documented in RFC 1951.

  The library also supports reading and writing files in gzip (.gz) format
  with an interface similar to that of stdio using the functions that start
  with "gz".  The gzip format is different from the zlib format.  gzip is a
  gzip wrapper, documented in RFC 1952, wrapped around a deflate stream.

  This library can optionally read and write gzip streams in memory as well.

  The zlib format was designed to be compact and fast for use in memory
  and on communications channels.  The gzip format was designed for single-
  file compression on file systems, has a larger header than zlib to maintain
  directory information, and uses a different, slower check method than zlib.

  The library does not install any signal handler. The decoder checks
  the consistency of the compressed data, so the library should never
  crash even in case of corrupted input.
*/

// the header segments of a gzip file uses bytes and should not be swapped...
int gfits_gz_stripheader (unsigned char *data, int *ndata) {
  
  int nstart, Ndata;

  if (data[0] != 0x1f) return FALSE; // magic
  if (data[1] != 0x8b) return FALSE; // magic
  if (data[2] != 0x08) return FALSE; // DEFLATED
  if (data[3] &  0xe0) return FALSE; // uses reserved flags?
  
  // data[4] - data[9] : time, xflags, OS code

  nstart = 10; // data[nstart] is first byte after header

  // check for an extra field
  if (data[3] & 0x04) {
    nstart += data[10];
    nstart += (data[11] << 8); // data[10], data[11] are a single 2byte word that should be swapped
  }
  
  // check for filename
  if (data[3] & 0x08) {
    while (data[nstart]) nstart ++;
  }

  // check for comment
  if (data[3] & 0x10) {
    while (data[nstart]) nstart ++;
  }

  // check for CRC
  if (data[3] & 0x02) {
    nstart ++;
    nstart ++;
  }

  Ndata = *ndata - nstart;
  memmove (data, &data[nstart], Ndata);
  *ndata = Ndata;

  return TRUE;
}

# define ESCAPE(RET) { fprintf (stderr, "gzip error in %s @ %s:%d\n", __func__, __FILE__, __LINE__); return (RET); }

int gfits_compress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen) {

  z_stream stream;
  int err;
  
  stream.next_in = (Bytef*)source;
  stream.avail_in = (uInt)sourceLen;

  /* Check for source > 64K on 16-bit machine: */
  if ((uLong)stream.avail_in != sourceLen) ESCAPE (Z_BUF_ERROR);

  stream.next_out = dest;
  stream.avail_out = (uInt)*destLen; // allocated space
  if ((uLong)stream.avail_out != *destLen) ESCAPE (Z_BUF_ERROR);

  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;

  if (EXTRA_VERBOSE) {
    fprintf (stderr, "inp cmp: ");
    unsigned long int i;
    for (i = 0; i < sourceLen; i++) {
      fprintf (stderr, "0x%02hhx ", source[i]);
    }
    fprintf (stderr, "\n");
  }

  // the '1' is the compression level: make this a user argument
  // NOTE: cfitsio uses a fixed value of 1
  // I am using deflateInit2 because cfitsio expects gzip, not just zlib
  // windowBits = 31 = (15 + 16) = (2^15 window bits) + (create a gzip stream)
  err = deflateInit2(&stream, 1, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
  if (0) fprintf (stderr, "inp buffers cmp 0: %d => %d => %d\n", stream.avail_in, stream.avail_out, (int) stream.total_out);

  if (err != Z_OK) ESCAPE(err);

  // XXX this is written to do the compression in a single pass.  it could be re-done to have 
  // the compression occur in a series of steps
  err = deflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    err = deflateEnd(&stream);
    ESCAPE (Z_BUF_ERROR);
  }

  if (EXTRA_VERBOSE) {
    fprintf (stderr, "out cmp: ");
    unsigned long int i;
    for (i = 0; i < stream.total_out; i++) {
      fprintf (stderr, "0x%02hhx ", dest[i]);
    }
    fprintf (stderr, "\n");
  }

  assert (stream.total_out <= *destLen);
  *destLen = stream.total_out;
  
  err = deflateEnd(&stream);
  return err;
}

// NOTE: CFITSIO uses gzip format, not just zlib
int gfits_uncompress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen) {

  z_stream stream;
  int err;
  
  stream.next_in = (Bytef*)source;
  stream.avail_in = (uInt)sourceLen;

  /* Check for source > 64K on 16-bit machine: */
  if ((uLong)stream.avail_in != sourceLen) ESCAPE (Z_BUF_ERROR);
  
  stream.next_out = dest;
  stream.avail_out = (uInt)*destLen;
  if ((uLong)stream.avail_out != *destLen) ESCAPE (Z_BUF_ERROR);
  
  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;
  
  if (EXTRA_VERBOSE) {
    fprintf (stderr, "inp unc: ");
    unsigned long int i;
    for (i = 0; i < sourceLen; i++) {
      fprintf (stderr, "0x%02hhx ", source[i]);
    }
    fprintf (stderr, "\n");
  }
  
  // windowBits = 47 = (15 + 32) = (2^15 window bits) + (test for either gzip or zlib
  // streams).  NOTE: modern versions of zlib (version > 1.2.3.4) allow for windowBits = 0
  // (or 16 or 32) to use the compressed stream windowBits value.  IPP cluster still has
  // zlib version = 1.2.3, so we cannot use this feature.
  err = inflateInit2(&stream, 47);
  if (err != Z_OK) ESCAPE (err);
  
  err = inflate(&stream, Z_FINISH);
  if (err != Z_STREAM_END) {
    if (err == Z_DATA_ERROR) {
      fprintf (stderr, "compressed data is corrupted\n");
      ESCAPE(err);
    }
    inflateEnd(&stream);
    if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0)) ESCAPE (Z_DATA_ERROR);
    ESCAPE(err);
  }
  
  if (EXTRA_VERBOSE) {
    fprintf (stderr, "out unc: ");
    unsigned long int i;
    for (i = 0; i < stream.total_out; i++) {
      fprintf (stderr, "0x%02hhx ", dest[i]);
    }
    fprintf (stderr, "\n");
  }

  assert (stream.total_out <= *destLen);
  *destLen = stream.total_out;

  err = inflateEnd(&stream);
  return err;
}

