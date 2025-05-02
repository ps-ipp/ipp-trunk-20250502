# include "astro.h"

int mksersic (int argc, char **argv) {
  
  int i, j, Nx, Ny, N, center;
  float *in;
  double Rmaj, Rmin, Theta, alpha, Io, ARatio;
  double root1, root2, R, A1, A2, A3;
  double Sx, Sy, Sxy;
  double x, y, r, f, Xo, Yo;
  Buffer *buf;

  Xo = Yo = 0;
  center = TRUE;
  if ((N = get_argument (argc, argv, "-coord"))) {
    remove_argument (N, &argc, argv);
    Xo = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Yo = atof (argv[N]);
    remove_argument (N, &argc, argv);
    center = FALSE;
  }

  Theta = 0.0;
  if ((N = get_argument (argc, argv, "-angle"))) {
    remove_argument (N, &argc, argv);
    Theta = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  ARatio = 1.0;
  if ((N = get_argument (argc, argv, "-aratio"))) {
    remove_argument (N, &argc, argv);
    ARatio = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: mksersic (buffer) (Io) (Rmaj) (alpha)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  if (center) {
    Xo = 0.5*Nx;
    Yo = 0.5*Ny;
  }
  
  Io = atof (argv[2]);

  /* shape parameters */
  Rmaj = atof (argv[3]);
  Rmin = Rmaj / ARatio;

  alpha = atof (argv[4]);

  /* given Rmaj, Rmin, Theta, find Sx, Sy, Sxy */
  root1 = SQ(1.0 / Rmaj);
  root2 = SQ(1.0 / Rmin);

  // XXX check this
  R = 0.5 * (root1 - root2);
  A1 = 0.25*(root1 + root2) - 0.5*R*cos(2*RAD_DEG*Theta);
  A2 = 0.25*(root1 + root2) + 0.5*R*cos(2*RAD_DEG*Theta);
  A3 = -R*sin(2*RAD_DEG*Theta);

  Sx = 0.5/A1;
  Sy = 0.5/A2;
  Sxy = A3;

  /* f = exp (-r^alpha), r = (x^2 / 2Sx) + (y^2 / 2Sy) + Sxy*x*y */
  in = (float *) buf[0].matrix.buffer;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, in++) {
      x = i + 0.5 - Xo;
      y = j + 0.5 - Yo;
      r = pow((0.5*x*x/Sx + 0.5*y*y/Sy + x*y*Sxy), alpha);
      f = Io*exp (-r);
      *in += f;
    }
  }

  return (TRUE);
}
