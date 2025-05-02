# include <ohana.h>
# include <gfitsio.h>

# ifndef NAN
# ifndef BYTE_SWAP
#  define __nan_bytes           { 0x7f, 0xc0, 0, 0 }
# else
#  define __nan_bytes           { 0, 0, 0xc0, 0x7f }
# endif
static union { unsigned char __c[4]; float __d; } __nan_union
__attribute_used__ = { __nan_bytes };
# define NAN    (__nan_union.__d)
# endif

# define CONVERTDOWN(MY_NAN)				\
  inMode  *in;						\
  outMode *out;						\
  out = (outMode *) matrix[0].buffer;			\
  in  = (inMode  *) matrix[0].buffer;			\
  for (i = 0; i < Npixels; i++, out++, in++)		\
    if (*in == (inMode) inBlank) {			\
      *out = (MY_NAN);					\
    } else {						\
      *out = *in*A + B;					\
    }							\
  REALLOCATE (matrix[0].buffer, char, matrix[0].datasize); 

# define CONVERTSAME(MY_NAN)			\
  inMode  *in;					\
  outMode *out;					\
  out = (outMode *) matrix[0].buffer;		\
  in  = (inMode  *) matrix[0].buffer;		\
  for (i = 0; i < Npixels; i++, out++, in++)	\
    if (*in == (inMode) inBlank) {			\
      *out = (MY_NAN);				\
    } else {					\
      *out = *in*A + B;				\
    }

# define CONVERTUP(MY_NAN)				\
  inMode  *in;						\
  outMode *out;						\
  REALLOCATE (matrix[0].buffer, char, matrix[0].datasize);	\
  out = (outMode *)matrix[0].buffer + Npixels - 1;	\
  in  = (inMode  *)matrix[0].buffer + Npixels - 1;	\
  for (i = 0; i < Npixels; i++, out--, in--)		\
    if (*in == (inMode) inBlank) {				\
      *out = (MY_NAN);					\
    } else {						\
      *out = *in*A + B;					\
    }

# define CONVERTDOWN_FF(MY_NAN)				\
  inMode  *in;						\
  outMode *out;						\
  out = (outMode *) matrix[0].buffer;			\
  in  = (inMode  *) matrix[0].buffer;			\
  for (i = 0; i < Npixels; i++, out++, in++)		\
    if (isnan(*in) || isinf(*in)) {			\
      *out = (MY_NAN);					\
    } else {						\
      *out = *in*A + B;					\
    }							\
  REALLOCATE (matrix[0].buffer, char, matrix[0].datasize); 

# define CONVERTSAME_FF(MY_NAN)			\
  inMode  *in;					\
  outMode *out;					\
  out = (outMode *) matrix[0].buffer;		\
  in  = (inMode  *) matrix[0].buffer;		\
  for (i = 0; i < Npixels; i++, out++, in++)	\
    if (isnan(*in) || isinf(*in)) {			\
      *out = (MY_NAN);				\
    } else {					\
      *out = *in*A + B;				\
    }

# define CONVERTUP_FF(MY_NAN)				\
  inMode  *in;						\
  outMode *out;						\
  REALLOCATE (matrix[0].buffer, char, matrix[0].datasize);	\
  out = (outMode *)matrix[0].buffer + Npixels - 1;	\
  in  = (inMode  *)matrix[0].buffer + Npixels - 1;	\
  for (i = 0; i < Npixels; i++, out--, in--)		\
    if (isnan(*in) || isinf(*in)) {			\
      *out = (MY_NAN);					\
    } else {						\
      *out = *in*A + B;					\
    }

/*********************** fits convert format ***********************************/
/* this function is safe in the number of axes (can even be 0) */
int gfits_convert_format (Header *header, Matrix *matrix, int outBitpix, double outScale, double outZero, int inBlank, int outUnsign) {

  off_t i, nbytes;
  int    inBitpix, inUnsign;
  double inScale, inZero;
  double A, B;

  inBitpix         = header[0].bitpix;
  inScale          = header[0].bscale;
  inZero           = header[0].bzero;
  inUnsign         = header[0].unsign;

  if ((inBitpix == outBitpix) && (inScale == outScale) && 
      (inZero == outZero) && (inUnsign == outUnsign))
    return (TRUE);

  matrix[0].bitpix   = header[0].bitpix = outBitpix;
  matrix[0].bscale   = header[0].bscale = outScale;
  matrix[0].bzero    = header[0].bzero  = outZero;
  matrix[0].unsign   = header[0].unsign = outUnsign;
  matrix[0].datasize = gfits_data_size (header);
  gfits_modify (header, "BITPIX", "%d", 1, outBitpix);
  gfits_modify (header, "BSCALE", "%lf", 1, outScale);
  gfits_modify (header, "BZERO",  "%lf", 1, outZero);
  gfits_modify_alt (header, "UNSIGN", "%t", 1, outUnsign);

  off_t Npixels = gfits_npix_matrix (matrix);
  nbytes                = Npixels * (abs(outBitpix) / 8);

  A = inScale / outScale;
  B = (inZero - outZero) / outScale;

  if ((!outUnsign) && (!inUnsign)) {  /** BLOCK 1 **/
    switch (inBitpix) {
      case 8: 
	{ typedef unsigned char inMode;
	switch (outBitpix) {
	  case 8:   { typedef unsigned char  outMode; CONVERTSAME(0); break; }
	  case 16:  { typedef short          outMode; CONVERTUP(0);   break; }
	  case -16: { typedef unsigned short outMode; CONVERTUP(0);   break; }
	  case 32:  { typedef int            outMode; CONVERTUP(0);   break; }
	  case -32: { typedef float          outMode; CONVERTUP(NAN);   break; }
	  case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	}
	}
	break;  
      case 16: 
	{ typedef short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef short          outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTSAME(0); break; }
	    case 32:  { typedef int            outMode; CONVERTUP(0);   break; }
	    case -32: { typedef float          outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case -16: 
	{ typedef unsigned short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef short          outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTSAME(0); break; }
	    case 32:  { typedef int            outMode; CONVERTUP(0);   break; }
	    case -32: { typedef float          outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case 32: 
	{ typedef int inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef short          outMode; CONVERTDOWN(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTDOWN(0); break; }
	    case 32:  { typedef int            outMode; CONVERTSAME(0); break; }
	    case -32: { typedef float          outMode; CONVERTSAME(NAN); break; }
	    case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;
      case -32: 
	{ typedef float inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef short          outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef int            outMode; CONVERTSAME_FF(0); break; }
	    case -32: { typedef float          outMode; CONVERTSAME_FF(NAN); break; }
	    case -64: { typedef double         outMode; CONVERTUP_FF(NAN);   break; }
	  }
	}
	break;
      case -64: 
	{ typedef double inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef short          outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef int            outMode; CONVERTDOWN_FF(0); break; }
	    case -32: { typedef float          outMode; CONVERTDOWN_FF(NAN); break; }
	    case -64: { typedef double         outMode; CONVERTSAME_FF(NAN); break; }
	  }
	}
    }
  }
  if ((outUnsign) && (!inUnsign)) {  /** BLOCK 3 **/
    switch (inBitpix) {
      case 8: 
	{ typedef unsigned char inMode;
	switch (outBitpix) {
	  case 8:   { typedef          char   outMode; CONVERTSAME(0); break; }
	  case 16:  { typedef unsigned short  outMode; CONVERTUP(0);   break; }
	  case -16: { typedef unsigned short  outMode; CONVERTUP(0);   break; }
	  case 32:  { typedef unsigned int    outMode; CONVERTUP(0);   break; }
	  case -32: { typedef          float  outMode; CONVERTUP(NAN);   break; }
	  case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	}
	}
	break;  
      case 16: 
	{ typedef short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef          char   outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTSAME(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTUP(0);   break; }
	    case -32: { typedef          float  outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case -16: 
	{ typedef unsigned short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef          char   outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTSAME(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTUP(0);   break; }
	    case -32: { typedef          float  outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case 32: 
	{ typedef int inMode;
	  switch (outBitpix) {
	    case 8:   { typedef          char   outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTSAME(0); break; }
	    case -32: { typedef          float  outMode; CONVERTSAME(NAN); break; }
	    case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;
      case -32: 
	{ typedef float inMode;
	  switch (outBitpix) {
	    case 8:   { typedef          char   outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTSAME_FF(0); break; }
	    case -32: { typedef          float  outMode; CONVERTSAME_FF(NAN); break; }
	    case -64: { typedef          double outMode; CONVERTUP_FF(NAN);   break; }
	  }
	}
	break;
      case -64: 
	{ typedef double inMode;
	  switch (outBitpix) {
	    case 8:   { typedef          char   outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTDOWN_FF(0); break; }
	    case -32: { typedef          float  outMode; CONVERTDOWN_FF(NAN); break; }
	    case -64: { typedef          double outMode; CONVERTSAME_FF(NAN); break; }
	  }
	}
    }
  }
  if ((!outUnsign) && (inUnsign)) {  /** BLOCK 2 **/
    switch (inBitpix) {
      case 8: 
	{ typedef unsigned char inMode;
	switch (outBitpix) {
	  case 8:   { typedef unsigned char  outMode; CONVERTSAME(0); break; }
	  case 16:  { typedef short          outMode; CONVERTUP(0);   break; }
	  case -16: { typedef unsigned short outMode; CONVERTUP(0);   break; }
	  case 32:  { typedef int            outMode; CONVERTUP(0);   break; }
	  case -32: { typedef float          outMode; CONVERTUP(NAN);   break; }
	  case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	}
	}
	break;  
      case 16: 
	{ typedef unsigned short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef short          outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTSAME(0); break; }
	    case 32:  { typedef int            outMode; CONVERTUP(0);   break; }
	    case -32: { typedef float          outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case -16: 
	{ typedef unsigned short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef short          outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTSAME(0); break; }
	    case 32:  { typedef int            outMode; CONVERTUP(0);   break; }
	    case -32: { typedef float          outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case 32: 
	{ typedef unsigned int inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef short          outMode; CONVERTDOWN(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTDOWN(0); break; }
	    case 32:  { typedef int            outMode; CONVERTSAME(0); break; }
	    case -32: { typedef float          outMode; CONVERTSAME(NAN); break; }
	    case -64: { typedef double         outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;
      case -32: 
	{ typedef float inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef short          outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef int            outMode; CONVERTSAME_FF(0); break; }
	    case -32: { typedef float          outMode; CONVERTSAME_FF(NAN); break; }
	    case -64: { typedef double         outMode; CONVERTUP_FF(NAN);   break; }
	  }
	}
	break;
      case -64: 
	{ typedef double inMode;
	  switch (outBitpix) {
	    case 8:   { typedef unsigned char  outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef short          outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef int            outMode; CONVERTDOWN_FF(0); break; }
	    case -32: { typedef float          outMode; CONVERTDOWN_FF(NAN); break; }
	    case -64: { typedef double         outMode; CONVERTSAME_FF(NAN); break; }
	  }
	}
    }
  }
  if ((outUnsign) && (inUnsign)) {
    switch (inBitpix) {
      case 8: 
	{ typedef char inMode;
	switch (outBitpix) {
	  case 8:   { typedef char            outMode; CONVERTSAME(0); break; }
	  case 16:  { typedef unsigned short  outMode; CONVERTUP(0);   break; }
	  case -16: { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	  case 32:  { typedef unsigned int    outMode; CONVERTUP(0);   break; }
	  case -32: { typedef          float  outMode; CONVERTUP(NAN);   break; }
	  case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	}
	}
	break;  
      case 16: 
	{ typedef unsigned short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef char            outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTUP(0);   break; }
	    case -32: { typedef          float  outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case -16: 
	{ typedef unsigned short inMode;
	  switch (outBitpix) {
	    case 8:   { typedef char            outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTSAME(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTUP(0);   break; }
	    case -32: { typedef          float  outMode; CONVERTUP(NAN);   break; }
	    case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	  }
	}
	break;  
      case 32: 
	{ typedef unsigned int inMode;
	  switch (outBitpix) {
	    case 8:   { typedef char            outMode; CONVERTDOWN(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTSAME(0); break; }
	    case -32: { typedef          float  outMode; CONVERTSAME(NAN); break; }
	    case -64: { typedef          double outMode; CONVERTUP(NAN);   break; }
	  }				    
	}					    
	break;				    
      case -32: 				    
	{ typedef float inMode;		    
	  switch (outBitpix) {		    
	    case 8:   { typedef char            outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTSAME_FF(0); break; }
	    case -32: { typedef          float  outMode; CONVERTSAME_FF(NAN); break; }
	    case -64: { typedef          double outMode; CONVERTUP_FF(NAN);   break; }
	  }				    
	}					    
	break;				    
      case -64: 				    
	{ typedef double inMode;		    
	  switch (outBitpix) {		    
	    case 8:   { typedef char            outMode; CONVERTDOWN_FF(0); break; }
	    case 16:  { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case -16: { typedef unsigned short  outMode; CONVERTDOWN_FF(0); break; }
	    case 32:  { typedef unsigned int    outMode; CONVERTDOWN_FF(0); break; }
	    case -32: { typedef          float  outMode; CONVERTDOWN_FF(NAN); break; }
	    case -64: { typedef          double outMode; CONVERTSAME_FF(NAN); break; }
	  }
	}
    }
  }

  /* zero the padding region */
  {
    char *out;
    off_t Nextra;
    
    out = (char *)matrix[0].buffer;
    Nextra = matrix[0].datasize - nbytes;
    bzero (&out[nbytes], Nextra);
  }

  return (TRUE);
}
 
/* WARNING --- this is STILL machine dependant.  It will now work on all 
   machines, as long as char, int, short, float, double have sizes of 
   8, 16, 32, 32, 64 bits resp.  However, the floating pt types are NOT
   universal (or FITS standard), and so can't exchange from machine to 
   machine. */

/* NOTICE -- This file looks rather strange.  It is set this way to make 
   all of the possible inter-conversions without too much excess code. 
   The macros CONVERTUP and CONVERTDOWN need to be distinct.
   Going to smaller or equal BITPIX, it is safe to run the array forward 
   and allocate after the conversion.
   Going to larger BITPIX, you need to reallocate first and run backwards 
   to avoid wiping out the pixels.
   OK, to deal with in and out UNSIGN, I have made four if blocks that are
   almost identical. this looks silly.  any suggestions? 
*/
