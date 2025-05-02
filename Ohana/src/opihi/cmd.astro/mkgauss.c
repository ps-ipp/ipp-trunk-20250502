# include "astro.h"

int mkgauss (int argc, char **argv) {
  
  int i, j, Nx, Ny, N;
  float *in;
  double Sig_x, Sig_y, Theta;
  double root1, root2, R, A1, A2, A3;
  double Sx, Sy, Sxy;
  double x, y, r, f, Xo, Yo;
  Buffer *buf;

  int Normalize = FALSE;
  if ((N = get_argument (argc, argv, "-norm"))) {
    Normalize = TRUE;
    remove_argument (N, &argc, argv);
  }    

  // this should be Nx/2, Ny/2 if not set
  Xo = Yo = NAN;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    Xo = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Yo = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((argc < 3) || (argc > 5)) {
    gprint (GP_ERR, "USAGE: mkgauss (buffer) (sigma) [[sy/sx] angle]\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  
  if (isnan(Xo)) Xo = Nx / 2;
  if (isnan(Yo)) Yo = Ny / 2;

  /* gaussian parameters */
  Sig_x = atof (argv[2]);
  Sig_y = Sig_x;
  Theta = 0.0;
  if (argc > 3) {
    Sig_y = Sig_x*atof (argv[3]);
    if (argc == 5) {
      Theta = atof (argv[4]);
    }
  }

  /* given Sig_x, Sig_y, Theta, find Sx, Sy, Sxy */
  root1 = SQ(1.0 / Sig_y);
  root2 = SQ(1.0 / Sig_x);

  R = 0.5 * (root1 - root2);
  A1 = 0.25*(root1 + root2) - 0.5*R*cos(2*RAD_DEG*Theta);
  A2 = 0.25*(root1 + root2) + 0.5*R*cos(2*RAD_DEG*Theta);
  A3 = -R*sin(2*RAD_DEG*Theta);

  Sx = 0.5/A1;
  Sy = 0.5/A2;
  Sxy = A3;

  /* f = exp (-r), r = (x^2 / 2Sx) + (y^2 / 2Sy) + Sxy*x*y */

  double Io = Normalize ? 1.0 / (2.0 * M_PI * Sig_x * Sig_y) : 1.0;

  in = (float *) buf[0].matrix.buffer;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, in++) {

      x = i - Xo;
      y = j - Yo;
      r = 0.5*x*x/Sx + 0.5*y*y/Sy + x*y*Sxy;
      f = Io * exp (-r);
      *in += f;
    }
  }

  return (TRUE);
}
