# include "astro.h"

int hermitian2d (int argc, char **argv) {
  
  int i, j, N, Nx, Ny, Xorder, Yorder, Poly;
  float *in;
  double Xo, Yo, sigma;
  double x, y, xf, yf, value, norm, weight;
  double nf, mf;
  Buffer *buf;

  // optionally return the Hermitian Polynomial or the Hermitian Function
  Poly = FALSE; 
  if ((N = get_argument (argc, argv, "-poly"))) {
    Poly = TRUE;
    remove_argument (N, &argc, argv);
  }

  Xo = Yo = NAN;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    Xo = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Yo = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) goto usage;

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  
  if (isnan(Xo)) Xo = Nx / 2;
  if (isnan(Yo)) Yo = Ny / 2;

  /* gaussian parameters */
  sigma = atof (argv[2]);
  Xorder = atof (argv[3]);
  Yorder = atof (argv[4]);
  if (Xorder < 0) goto usage;
  if (Yorder < 0) goto usage;
  if (Xorder > 10) goto usage;
  if (Yorder > 10) goto usage;

  nf = exp(lgamma(Xorder + 1));
  mf = exp(lgamma(Yorder + 1));
  norm = 1.0 / sqrt(nf*mf*2*M_PI) / sigma;

  in = (float *) buf[0].matrix.buffer;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, in++) {

      x = (i - Xo) / sigma;
      y = (j - Yo) / sigma;

      // not sure what a 2D hermitian looks like
      // is it H_i(x) * H_j(y)?

      xf = hermitian_polynomial (x, Xorder);
      yf = hermitian_polynomial (y, Yorder);
      value = xf*yf;
      if (!Poly) {
	weight = norm*exp(-0.25*(x*x + y*y));
	value *= weight;
      }

      *in += value;
    }
  }

  return (TRUE);

usage:
  gprint (GP_ERR, "USAGE: hermitian2d (buffer) (sigma) Xorder Yorder\n");
  gprint (GP_ERR, "  Note : only orders up to 10 are supported\n");
  return (FALSE);
}
