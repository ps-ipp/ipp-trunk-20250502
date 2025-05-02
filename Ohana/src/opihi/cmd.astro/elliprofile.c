# include "astro.h"

int elliprofile (int argc, char **argv) {
  
  int i, ix, iy, Nx, Ny;
  float *in;
  double Rmaj, Rmin, phi;
  double root1, root2, R, A1, A2, A3;
  double Sx, Sy, Sxy;
  double x, y, Xo, Yo;
  Buffer *buf;
  Vector *rvec, *fvec;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: elliprofile (buffer) (rvec) (fvec) (Xo) (Yo) (Rmaj) (Rmin) (phi)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];

  if ((rvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((fvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (rvec, OPIHI_FLT, Nx*Ny);
  ResetVector (fvec, OPIHI_FLT, Nx*Ny);

  Xo = atof(argv[4]);
  Yo = atof(argv[5]);

  /* shape parameters */
  Rmaj = atof (argv[6]);
  Rmin = atof (argv[7]);
  phi  = atof (argv[8]);

  /* given Rmaj, Rmin, phi, find Sx, Sy, Sxy */
  root1 = SQ(1.0 / Rmaj);
  root2 = SQ(1.0 / Rmin);

  // XXX check this
  R = 0.5 * (root1 - root2);
  A1 = 0.25*(root1 + root2) - 0.5*R*cos(2*RAD_DEG*phi);
  A2 = 0.25*(root1 + root2) + 0.5*R*cos(2*RAD_DEG*phi);
  A3 = -R*sin(2*RAD_DEG*phi);

  Sx = 0.5/A1;
  Sy = 0.5/A2;
  Sxy = A3;

  /* f = exp (-r^alpha), r^2 = (x^2 / 2Sx) + (y^2 / 2Sy) + Sxy*x*y */
  in = (float *) buf[0].matrix.buffer;
  for (i = iy = 0; iy < Ny; iy++) {
      for (ix = 0; ix < Nx; ix++, in++, i++) {
      x = ix + 0.5 - Xo;
      y = iy + 0.5 - Yo;
      rvec[0].elements.Flt[i] = sqrt(0.5*x*x/Sx + 0.5*y*y/Sy + x*y*Sxy);
      fvec[0].elements.Flt[i] = *in;
    }
  }
  rvec[0].Nelements = fvec[0].Nelements = i;
  return (TRUE);
}
