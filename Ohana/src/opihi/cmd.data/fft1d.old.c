# include "data.h"

int fft1dold (int argc, char **argv) {
  
  int i, Npix, ZeroImaginary;
  float *t1, *t2, *temp;
  Vector *Ire, *Iim, *Ore, *Oim;

  if ((argc != 6) || (strcmp (argv[3], "to"))) {
    gprint (GP_ERR, "USAGE: fft1d (real) (imag) to (real) (imag)\n");
    return (FALSE);
  }

  Iim = NULL;
  ZeroImaginary = TRUE;
  if (strcmp (argv[2], "0")) {
    ZeroImaginary = FALSE;
    if ((Iim = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  }    
  if ((Ire = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Ore = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Oim = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Npix = Ire[0].Nelements;
  if (!ZeroImaginary && (Npix != Iim[0].Nelements)) {
    gprint (GP_ERR, "vector mismatch in size\n");
    return (FALSE);
  }

  if (!IsBinaryOld (Npix)) {
    gprint (GP_ERR, "Npix is not a binary number!\n");
    return (FALSE);
  }
  
  ALLOCATE (temp, float, 2*Npix);
  if (ZeroImaginary) {
    t1 = Ire[0].elements;
    for (i = 0; i < Npix; i++, t1++) {
      temp[2*i  ] = *t1;
      temp[2*i+1] = 0;
    }
  } else {
    t1 = Ire[0].elements;
    t2 = Iim[0].elements;
    for (i = 0; i < Npix; i++, t1++, t2++) {
      temp[2*i  ] = *t1;
      temp[2*i+1] = *t2;
    }
  }    
    
  fftold (temp, Npix, 1); 

  Ore[0].Nelements = Npix;
  Oim[0].Nelements = Npix;
  REALLOCATE (Ore[0].elements, float, Npix);
  REALLOCATE (Oim[0].elements, float, Npix);
 
  t1 = Ore[0].elements;
  t2 = Oim[0].elements;
  for (i = 0; i < Npix; i++, t1++, t2++) {
    *t1 = temp[2*i  ] / Npix;
    *t2 = temp[2*i+1] / Npix;
  }    
  
  free (temp);
  
  return (TRUE);
}

  
