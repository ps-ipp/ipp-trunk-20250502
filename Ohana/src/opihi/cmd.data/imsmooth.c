# include "data.h"

int imsmooth (int argc, char **argv) {
  
  int i, j, n, N, Nx, Ny, Ns, Ngauss;
  float *vi, *vo, *gauss, *gaussnorm;
  float g, s, sigma, Nsigma;
  Buffer *in;
  float *temp;

  Nsigma = 3;
  if ((N = get_argument (argc, argv, "-Nsigma"))) {
    remove_argument (N, &argc, argv);
    Nsigma = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: imsmooth (input) sigma\n");
    return (FALSE);
  }
  
  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  sigma = atof (argv[2]);

  Nx = in[0].matrix.Naxis[0];
  Ny = in[0].matrix.Naxis[1];
  ALLOCATE (temp, float, Nx*Ny);

  /* build a 1D gaussian */
  Ns = (int) (Nsigma*sigma + 0.5);
  Ngauss = 2*Ns + 1;
  ALLOCATE (gaussnorm, float, Ngauss);
  gauss = &gaussnorm[Ns];
  for (i = -Ns; i < Ns + 1; i++) {
    gauss[i] = exp ((i*i)/(-2*sigma*sigma));
  }

  /* smooth in X direction */
  for (j = 0; j < Ny; j++) {
    vi = (float *) in[0].matrix.buffer + j*Nx;
    vo = &temp[j*Nx];
    for (i = 0; i < Nx; i++) {
      g = s = 0;
      for (n = -Ns; n < Ns + 1; n++) {
	if (i+n < 0) continue;
	if (i+n >= Nx) continue;
	s += gauss[n]*vi[i+n];
	g += gauss[n];
      }
      vo[i] = s / g;
    }
  }

  /* smooth in Y direction */
  for (i = 0; i < Nx; i++) {
    vi = &temp[i];
    vo = (float *)in[0].matrix.buffer + i;
    for (j = 0; j < Ny; j++) {
      g = s = 0;
      for (n = -Ns; n < Ns + 1; n++) {
	if (j+n < 0) continue;
	if (j+n >= Ny) continue;
	s += gauss[n]*vi[(n+j)*Nx];
	g += gauss[n];
      }
      vo[j*Nx] = s / g;
    }
  }

  free (temp);
  free (gaussnorm);
  return (TRUE);
}

