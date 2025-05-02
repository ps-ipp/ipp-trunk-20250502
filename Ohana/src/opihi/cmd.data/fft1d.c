# include "data.h"

int fft1d (int argc, char **argv) {
  
  int N, Npix, Nbit, ZeroImaginary, forward;
  Vector *Ire, *Iim, *Ore, *Oim;

  forward = TRUE;
  if ((N = get_argument (argc, argv, "-inverse"))) {
    remove_argument (N, &argc, argv);
    forward = FALSE;
  }

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
  
  REQUIRE_VECTOR_FLT (Ire, FALSE); 
  if (Iim) { REQUIRE_VECTOR_FLT (Iim, FALSE); }

  // check the input data (match lengths? binary length?)
  Npix = Ire[0].Nelements;
  if (!ZeroImaginary && (Npix != Iim[0].Nelements)) {
    gprint (GP_ERR, "vector size mismatch\n");
    return (FALSE);
  }
  if (!IsBinary (Npix, &Nbit)) {
    gprint (GP_ERR, "Npix is not a binary number!\n");
    return (FALSE);
  }

  // select or create the output vectors
  if ((Ore = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Oim = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  // allocate sufficient output space, force output to be FLT
  ResetVector (Ore, OPIHI_FLT, Npix);
  ResetVector (Oim, OPIHI_FLT, Npix);
 
  // copy data to output vectors (fft is done in place)
  memcpy (Ore[0].elements.Flt, Ire[0].elements.Flt, Npix*sizeof(opihi_flt));

  // copy imaginary vector or create a zero vector
  if (ZeroImaginary) {
    memset (Oim[0].elements.Flt, 0, Npix*sizeof(opihi_flt));
  } else {
    memcpy (Oim[0].elements.Flt, Iim[0].elements.Flt, Npix*sizeof(opihi_flt));
  }    

  dfft1D (Ore[0].elements.Flt, Oim[0].elements.Flt, Npix, Nbit, forward); 
  
  return (TRUE);
}
