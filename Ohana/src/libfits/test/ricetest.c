# include <ohana.h>
# include <gfitsio.h>
# include <gfits_compress.h>
# include "tap_ohana.h"

# define NPIX 100
# define NBYTE 4*NPIX

int main (void) {
  
  plan_tests (45);

  diag ("libfits ricetest.c tests");

  // ok (1, "failure");

  if (1) { 
    // buffers to store max array
    char rawdata[NBYTE];
    unsigned char outdata[NBYTE];
    unsigned char cmpdata[NBYTE];
    char *rawvalue = (char *) rawdata;
    char *outvalue = (char *) outdata;

    int j;
    for (j = 0; j < 5; j++) { 
      int i;
      for (i = 0; i < NPIX; i++) {
	rawvalue[i] = i;
      }

      int Ncmp = fits_rcomp_byte (rawvalue, NPIX, cmpdata, NBYTE, 32);
      ok (Ncmp > 0, "compressed byte data");

      int status = fits_rdecomp_byte (cmpdata, Ncmp, outdata, NPIX, 32);
      ok (!status, "decompressed byte data");

      int Nbad = 0;
      for (i = 0; i < NPIX; i++) {
	if (rawvalue[i] != outvalue[i]) Nbad ++;
      }

      ok (!Nbad, "values match");
    }
  }

  if (1) { 
    // buffers to store max array
    char rawdata[NBYTE];
    unsigned short outdata[NBYTE];
    unsigned char cmpdata[NBYTE];

    short *rawvalue = (short *) rawdata;
    short *outvalue = (short *) outdata;

    int j;
    for (j = 0; j < 5; j++) { 
      int i;
      for (i = 0; i < NPIX; i++) {
	rawvalue[i] = i;
      }

      int Ncmp = fits_rcomp_short (rawvalue, NPIX, cmpdata, NBYTE, 32);
      ok (Ncmp > 0, "compressed short data");

      int status = fits_rdecomp_short (cmpdata, Ncmp, outdata, NPIX, 32);
      ok (!status, "decompressed short data");

      int Nbad = 0;
      for (i = 0; i < NPIX; i++) {
	if (rawvalue[i] != outvalue[i]) Nbad ++;
      }

      ok (!Nbad, "values match");
    }
  }

  if (1) { 
    // buffers to store max array
    char rawdata[NBYTE];
    unsigned int outdata[NBYTE];
    unsigned char cmpdata[NBYTE];

    int *rawvalue = (int *) rawdata;
    int *outvalue = (int *) outdata;

    int j;
    for (j = 0; j < 5; j++) { 
      int i;
      for (i = 0; i < NPIX; i++) {
	rawvalue[i] = i;
      }

      int Ncmp = fits_rcomp (rawvalue, NPIX, cmpdata, NBYTE, 32);
      ok (Ncmp > 0, "compressed int data");

      int status = fits_rdecomp (cmpdata, Ncmp, outdata, NPIX, 32);
      ok (!status, "decompressed int data");

      int Nbad = 0;
      for (i = 0; i < NPIX; i++) {
	if (rawvalue[i] != outvalue[i]) Nbad ++;
      }
      ok (!Nbad, "values match");
    }
  }
  return exit_status();
}
