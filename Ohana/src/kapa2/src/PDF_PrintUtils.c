# include "Ximage.h"
# include <zlib.h>

# define BASETYPE unsigned int

int PDF_ASCIIHexEncode (IOBuffer *output, IOBuffer *input);
int PDF_ASCII85Encode (IOBuffer *output, IOBuffer *input);
int PDF_BufferDeflate (IOBuffer *output, IOBuffer *input);
static int base85_encode(char *out, size_t max, const BASETYPE *data, size_t count);

PDF_FILE *PDF_Open (char *filename) {
  FILE *f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "can't open output file %s\n", filename);
    return NULL;  /* true because otherwise it quits kapa! */
  }

  ALLOCATE_PTR (obj, PDF_FILE, 1);
  
  obj->f = f;
  obj->Nsegment =  0;
  obj->NSEGMENT = 16;
  ALLOCATE (obj->offset, int, obj->NSEGMENT);
  ALLOCATE (obj->objnum, int, obj->NSEGMENT);

  obj->Nstream =  0;
  obj->NSTREAM = 16;
  ALLOCATE (obj->streamObjnum, int, obj->NSEGMENT);

  obj->Nimage =  0;
  obj->NIMAGE = 16;
  ALLOCATE (obj->imageObjnum, int, obj->NSEGMENT);
  return obj;
}

// nObject : 0 -> extend current object, >0 -> create a new object
int PDF_Print (PDF_FILE *obj, int nObject, char *format, ...) {

  va_list argp;  

  int N = obj->Nsegment;

  // record the start of the current object
  if (nObject > 0) {
    obj->offset[N] = ftell(obj->f);
    obj->objnum[N] = nObject;
    obj->Nsegment ++;
    if (obj->Nsegment >= obj->NSEGMENT) {
      obj->NSEGMENT += 16;
      REALLOCATE (obj->offset, int, obj->NSEGMENT);
      REALLOCATE (obj->objnum, int, obj->NSEGMENT);
    }
  }

  // do the formattted print
  va_start (argp, format);
  int status = vfprintf (obj->f, format, argp);
  va_end (argp);
  return status;
}

int PDF_Close (PDF_FILE *obj) {
  
  // Image list
  // Images are names /Image0 - /ImageNNN
  PDF_Print (obj, 4, "4 0 obj <<\n");
  for (int i = 0; i < obj->Nimage; i++) {
    PDF_Print (obj, 0, "/Image%d %d 0 R\n", i, obj->imageObjnum[i]);
  }
  PDF_Print (obj, 0, ">> endobj\n");

  // Content Stream list:
  PDF_Print (obj, 5, "5 0 obj [\n");
  for (int i = 0; i < obj->Nstream; i++) {
    PDF_Print (obj, 0, " %d 0 R\n", obj->streamObjnum[i]);
  }
  PDF_Print (obj, 0, "] endobj\n");

  PDF_AlphaDump (obj);

  // Cross-Ref Section
  int XrefStart = ftell(obj->f);

  // for the moment, fixed number of objects in table:
  // Nsegment counts the number of real segments (N > 0)
  fprintf (obj->f, "xref 0 %d\n", obj->Nsegment + 1);
  fprintf (obj->f, "%010d 65535 f \n", 0); // the zero entry is special

  // I need to sort the segment list by objectN:
  isortpair (obj->objnum, obj->offset, obj->Nsegment);
  
  for (int i = 0; i < obj->Nsegment; i++) {
    fprintf (obj->f, "%010d 00000 n \n", obj->offset[i]);
  }
  fprintf (obj->f, "trailer << /Size %d /Root 1 0 R >>\n", obj->Nsegment + 1);
  fprintf (obj->f, "startxref %d\n", XrefStart);
  fprintf (obj->f, "%%EOF\n");

  fclose  (obj->f);
  free (obj->offset);
  free (obj->objnum);
  free (obj);
  return TRUE;
}

// generate the buffer for the first stream and set the user-space scale & offset
int PDF_CreateStream (IOBuffer *buffer, float scale, int Xoff, int Yoff) {

  InitIOBuffer (buffer, 1024);
  
  PrintIOBuffer (buffer, "%6.2f 0.0 0.0 %6.2f %d %d cm\n", scale, scale, Xoff, Yoff);

  return TRUE;
}

typedef enum {
  PDF_RAW_ENCODE,
  PDF_ASCII_HEX,
  PDF_ASCII_ADOBE,
  PDF_ASCII_HEX_DEFLATE,
  PDF_ASCII_ADOBE_DEFLATE,
} PDF_Encoding_Mode;

// This code needs to generate a stream
int PDF_WriteStream (PDF_FILE *obj, IOBuffer *buffer) {

  if (!buffer->Nbuffer) return FALSE;

  // I need the segment object number for this stream

  // last object number:
  int Nlast = obj->Nsegment - 1;
  assert (Nlast > 0);

  // the object number for this stream is the number for the last stream plus 1
  int objnum = obj->objnum[Nlast] + 1;

  obj->streamObjnum[obj->Nstream] = objnum;
  obj->Nstream ++;
  if (obj->Nstream >= obj->NSTREAM) {
    obj->NSTREAM += 16;
    REALLOCATE (obj->streamObjnum, int, obj->NSTREAM);
  }

  // compression options:
  PDF_Encoding_Mode mode = PDF_ASCII_ADOBE_DEFLATE;
  // PDF_Encoding_Mode mode = PDF_RAW_ENCODE;
  switch (mode) {
    case PDF_ASCII_ADOBE: {
      IOBuffer encode;
      PDF_ASCII85Encode (&encode, buffer);

      // Content Stream
      PDF_Print (obj, objnum, "%d 0 obj << /Length %d /Filter [/ASCII85Decode] >> stream\n", objnum, encode.Nbuffer);
      fwrite (encode.buffer, 1, encode.Nbuffer, obj->f);
      break;
    } 
    case PDF_ASCII_ADOBE_DEFLATE: {
      IOBuffer compressed;
      PDF_BufferDeflate (&compressed, buffer);

      IOBuffer encode;
      PDF_ASCII85Encode (&encode, &compressed);

      // Content Stream
      PDF_Print (obj, objnum, "%d 0 obj << /Length %d /Filter [/ASCII85Decode /FlateDecode] >> stream\n", objnum, encode.Nbuffer);
      fwrite (encode.buffer, 1, encode.Nbuffer, obj->f);
      break;
    } 
    case PDF_ASCII_HEX: {
      IOBuffer encode;
      PDF_ASCIIHexEncode (&encode, buffer);

      // Content Stream
      PDF_Print (obj, objnum, "%d 0 obj << /Length %d /Filter [/ASCIIHexDecode] >> stream\n", objnum, encode.Nbuffer);
      fwrite (encode.buffer, 1, encode.Nbuffer, obj->f);
      break;
    } 
    case PDF_ASCII_HEX_DEFLATE: {
      IOBuffer compressed;
      PDF_BufferDeflate (&compressed, buffer);

      IOBuffer encode;
      PDF_ASCIIHexEncode (&encode, &compressed);

      // Content Stream
      PDF_Print (obj, objnum, "%d 0 obj << /Length %d /Filter [/ASCIIHexDecode /FlateDecode] >> stream\n", objnum, encode.Nbuffer);
      fwrite (encode.buffer, 1, encode.Nbuffer, obj->f);
      break;
    } 
    case PDF_RAW_ENCODE: {
      PDF_Print (obj, objnum, "%d 0 obj << /Length %d >> stream\n", objnum, buffer->Nbuffer);
      fwrite (buffer->buffer, 1, buffer->Nbuffer, obj->f);
      break;
    }
  }
  // XXX check fwrite return to make sure all went to disk

  fprintf (obj->f, "endstream endobj\n");

  FlushIOBuffer (buffer);
  return TRUE;
}

// This code needs to generate a stream
int PDF_WriteImage (PDF_FILE *obj, IOBuffer *buffer, int dX, int dY) {

  if (!buffer) return FALSE;
  // if (!buffer->Nbuffer) return FALSE;

  // I need the segment object number for this stream

  // last object number:
  int Nlast = obj->Nsegment - 1;
  assert (Nlast > 0);

  // the object number for this stream is the number for the last stream plus 1
  int objnum = obj->objnum[Nlast] + 1;

  obj->imageObjnum[obj->Nimage] = objnum;
  obj->Nimage ++;
  if (obj->Nimage >= obj->NIMAGE) {
    obj->NIMAGE += 16;
    REALLOCATE (obj->imageObjnum, int, obj->NIMAGE);
  }

  // Deflate buffer.buffer
  IOBuffer compressed;
  PDF_BufferDeflate (&compressed, buffer);

  // ASCII85 encode
  IOBuffer encode;
  PDF_ASCII85Encode (&encode, &compressed);

  // Image Dictionary
  PDF_Print (obj, objnum, "%d 0 obj <<\n", objnum);
  PDF_Print (obj, 0, " /Type /XObject /Subtype /Image\n");
  PDF_Print (obj, 0, " /ColorSpace /DeviceRGB /BitsPerComponent 8\n");
  PDF_Print (obj, 0, " /Width %d /Height %d\n", dX, dY);
  PDF_Print (obj, 0, " /Length %d /Filter [/ASCII85Decode /FlateDecode] >> stream\n", encode.Nbuffer);
 
  // PDF_Print (obj, 0, " /Length %d /Filter /ASCIIHexDecode >> stream\n", buffer->Nbuffer);

  // write image data here
  // XXX check return to make sure all went to disk
  fwrite (encode.buffer, 1, encode.Nbuffer, obj->f);

  PDF_Print (obj, 0, "\n  endstream endobj\n");

  FlushIOBuffer (buffer);
  return TRUE;
}

# define EXTRA_VERBOSE 0

# define ESCAPE(RET) { fprintf (stderr, "gzip error in %s @ %s:%d\n", __func__, __FILE__, __LINE__); return (RET); }

// allocate output 
int PDF_BufferDeflate (IOBuffer *output, IOBuffer *input) {

  Bytef *source = (Bytef *)input->buffer;
  uLong sourceLen = input->Nbuffer;

  // allocate an output buffer allowing for minimum expansion:
  InitIOBuffer (output, input->Nbuffer * 1.01 + 16);

  Bytef *dest = (Bytef *)output->buffer;
  uLongf destLen = output->Nalloc;

  z_stream stream;
  int err;
  
  stream.next_in = (Bytef*)source;
  stream.avail_in = (uInt)sourceLen;

  /* Check for source > 64K on 16-bit machine: */
  if ((uLong)stream.avail_in != sourceLen) ESCAPE (Z_BUF_ERROR);

  stream.next_out = dest;
  stream.avail_out = (uInt) destLen; // allocated space
  if ((uLong)stream.avail_out != destLen) ESCAPE (Z_BUF_ERROR);

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
  err = deflateInit(&stream, Z_BEST_COMPRESSION);
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

  assert (stream.total_out <= destLen);

  output->Nbuffer = stream.total_out;
  
  err = deflateEnd(&stream);
  return err;
}

int PDF_ASCIIHexEncode (IOBuffer *output, IOBuffer *input) {

  InitIOBuffer (output, 2*input->Nbuffer);
  
  for (int i = 0; i < input->Nbuffer; i++) {
    PrintIOBuffer (output, "%02x", (unsigned char) input->buffer[i]);
  }
  return TRUE;
}

int PDF_ASCII85Encode (IOBuffer *output, IOBuffer *input) {

  int Nint = (int)ceilf(input->Nbuffer / 4.0);
  int Nout = 5*Nint;

  InitIOBuffer (output, Nout);
  
  // the conversion code requires big-endian order; I should probably re-write it to use little-endian?
  ALLOCATE_PTR (buffer, BASETYPE, Nint);
  for (int i = 0; i < Nint; i++) {
    char *tmpbuf = (char *) &buffer[i];
    tmpbuf[0] = input->buffer[4*i + 3];
    tmpbuf[1] = input->buffer[4*i + 2];
    tmpbuf[2] = input->buffer[4*i + 1];
    tmpbuf[3] = input->buffer[4*i + 0];
  }
  base85_encode (output->buffer, output->Nalloc, (BASETYPE *) buffer, Nint);
  output->Nbuffer = Nout;

  return TRUE;
}

// EAM: The base85 conversion below is from http://orangetide.com/code/base85.c
// I have stripped down to just the Adobe portion
// Where the original author refers to 'ascii85', he means the adobe version

/* base85.c : encode numbers using two different techniques for Base-85 
 * PUBLIC DOMAIN - Jon Mayo - September 10, 2008 */

/* define this to use PDF/adobe Ascii85 encoding
 * default is to use an encoding derived from RFC1924, except 32-bits at a time instead of 128-bits
 * yes, I realize that RFC1924 is a joke RFC
 */
#define NR(x) (sizeof(x)/sizeof*(x))
#define BASE85_DIGITS	5	 /* log85 (2^32) is 4.9926740807112 */

# ifndef UCHAR_MAX
# define UCHAR_MAX 255
# endif

static const unsigned char base85[85] =  "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu";
static         signed char decode_table[UCHAR_MAX];

static int base85_init_done = 0;

/* create a look up table suitable for convering characters to base85 digits */
static void base85_init(void) {
  unsigned char ch;

  if (base85_init_done) return;
  base85_init_done = 1;
  
  assert((sizeof base85 == 85) && (base85[84] != 0)); /* make sure the array is exactly the right size */

  for (ch = 0; ch < UCHAR_MAX; ch++) {
    decode_table[ch] = -1;
  }

  for(ch = 0; ch < 85; ch++) {
    decode_table[base85[ch]] = ch;
  }
}

/* convert a list of 32-bit values into a base85 string.
 * if you wish to encode 8-bit values, load them into 32-bit values in Big Endian order
 * example:
 *   input: "Lion"
 *   ascii85: 9PJE_
 *   base85: !aflO
 */

static int base85_encode(char *out, size_t max, const BASETYPE *data, size_t count) {

  size_t i;
  BASETYPE n;

  base85_init();

  while(count) {
    if(max<1) return 0; /* failure */
    n=*data++;
    count--;
    /* Ascii85 (adobe) */
    if(n==0) {
      *out++='z'; /* this is a special zero character */
      max--;
    } else {
      if(max<5) return 0; /* no room */
      for(i=BASE85_DIGITS;i--;) {
	out[i]=base85[n%85];
	n/=85;
      }
      max-=5;
      out+=5;
    }
  }
  *out=0;
  return 1; /* success */
}

// base85_init();
// if (!base85_encode (output, Nout, (BASETYPE *) quote, 1)) { fprintf (stderr, "failure\n"); }

// test compression of the stream:
int PDF_Encode_Test () {

  if (0) {
    IOBuffer iTest, oTest;
    InitIOBuffer (&iTest, 8);
    strcpy (iTest.buffer, "Lion");
    iTest.Nbuffer = strlen(iTest.buffer);
    fprintf (stderr, "%s\n", iTest.buffer);
    PDF_ASCII85Encode (&oTest, &iTest);
    fprintf (stderr, "%s\n", oTest.buffer);
  }

  if (0) { 
    char quote[] = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";
    fprintf (stderr, "%d : %s\n", (int) strlen(quote), quote);
    IOBuffer iTest, oTest;
    InitIOBuffer (&iTest, strlen(quote) + 1);
    strcpy (iTest.buffer, quote);
    iTest.Nbuffer = strlen(iTest.buffer);
    PDF_ASCII85Encode (&oTest, &iTest);
    fprintf (stderr, "%s\n", oTest.buffer);
    return FALSE;
  }
  return TRUE;
}

