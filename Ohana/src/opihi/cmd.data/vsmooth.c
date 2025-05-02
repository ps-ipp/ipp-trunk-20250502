# include "data.h"

int vsmooth (int argc, char **argv) {
  
  int i, n, N, Nx, Ns, Ngauss, isFloat;
  opihi_flt *vf, *vo, *gauss, *gaussnorm;
  opihi_int *vi;
  float g, s, sigma, Nsigma, value;
  Vector *in;

  Nsigma = 3;
  if ((N = get_argument (argc, argv, "-Nsigma"))) {
    remove_argument (N, &argc, argv);
    Nsigma = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: vsmooth (input) sigma\n");
    return (FALSE);
  }
  
  if ((in  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  sigma = atof (argv[2]);
  Nx = in[0].Nelements;

  /* build a 1D gaussian */
  Ns = (int) (Nsigma*sigma + 0.5);
  Ngauss = 2*Ns + 1;
  ALLOCATE (gaussnorm, opihi_flt, Ngauss);
  gauss = &gaussnorm[Ns];
  for (i = -Ns; i < Ns + 1; i++) {
    gauss[i] = exp ((i*i)/(-2*sigma*sigma));
  }

  ALLOCATE (vo, opihi_flt, Nx);

  isFloat = (in[0].type == OPIHI_FLT);
  vf = in[0].elements.Flt;
  vi = in[0].elements.Int;

  for (i = 0; i < Nx; i++) {
    g = s = 0;
    for (n = -Ns; n < Ns + 1; n++) {
      if (i+n < 0) continue;
      if (i+n >= Nx) continue;
      value = isFloat ? vf[i+n] : vi[i+n];
      s += gauss[n]*value;
      g += gauss[n];
    }
    vo[i] = s / g;
  }

  free (gaussnorm);

  if (isFloat) {
    free (in[0].elements.Flt);
  } else {
    free (in[0].elements.Int);
  }

  // smoothing an int vector results in a float vector
  in[0].type = OPIHI_FLT;
  in[0].elements.Flt = vo;
  return (TRUE);
}

