%module pslib

%{
#define PS_ALLOW_MALLOC
#define SWIG
#include "pslib.h"

/* SWIG uses malloc/free - make it use the pslib memory functions instead. */
/*
#define malloc(S)    psAlloc(S)
#define realloc(P,S) psRealloc(P,S)
#define free(P)      psFree(P)
*/

%}

/* XXX: this is temporary -- not portable, but should work with any current OS supported */
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;
typedef char           int8_t;
typedef short          int16_t;
typedef int            int32_t;
typedef long long      int64_t;

/* grab the typedefs used throughout psLib, e.g. psU8, psU16,... */
%include "cpointer.i"
%include "typemaps.i"

/**
 * Typemap to map FILE* input to the script's native file descriptor wrapper.
 * Used for such functions as psErrorStackPrint and psTraceSetDestination.
 */
%typemap(in) FILE * {
#if defined(SWIGPERL)
   if (!SvOK($input)) {
      $1 = NULL;
   } else {
      $1 = PerlIO_findFILE(IoIFP(sv_2io($input)));
   }
#elif defined(SWIGPYTHON)
    if ($input == Py_None) {
	$1 = NULL;
    } else if (!PyFile_Check($input)) {
	PyErr_SetString(PyExc_TypeError, "Need a file!");
	goto fail;
    } else {
	$1 = PyFile_AsFile($input);
    }
#endif
}

%{
typedef struct { float re, im; } swig_psC32;  ///< 32-bit complex floating point
typedef struct { double re, im; } swig_psC64; ///< 64-bit complex floating point
%}

/* the actual including of headers are found in each of the directories. */
%include "astro.i"
%include "db.i"
%include "fft.i"
%include "fits.i"
%include "imageops.i"
%include "math.i"
%include "mathtypes.i"
%include "sys.i"
%include "types.i"
%include "xml.i"
