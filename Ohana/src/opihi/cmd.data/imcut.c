# include "data.h"

int imcut (int argc, char **argv) {
  
  int i, Nx, Ny, xi, yi, L;
  double xs, ys, xe, ye, dX, dY;
  Vector *xvec, *yvec;
  Buffer *buf;
  float *V;

  if (argc != 8) goto usage;

  if ((buf  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto missed;
  if ((xvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) goto usage;
  if ((yvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) goto usage;
 
  xs = atof (argv[4]);
  ys = atof (argv[5]);
  xe = atof (argv[6]);
  ye = atof (argv[7]);

  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];

  if ((xs < 0) || (xs > Nx)) goto range;
  if ((ys < 0) || (ys > Ny)) goto range;
  if ((xe < 0) || (xe > Nx)) goto range;
  if ((ye < 0) || (ye > Ny)) goto range;

  dX = xe - xs;
  dY = ye - ys;
  L = hypot (dX, dY);
  dX = dX / L;
  dY = dY / L;

  ResetVector (xvec, OPIHI_FLT, L);
  ResetVector (yvec, OPIHI_FLT, L);

  V = (float *)buf[0].matrix.buffer;
  for (i = 0; i < L; i++) {
    xi = xs + i*dX - 0.5;
    yi = ys + i*dY - 0.5;
    xvec[0].elements.Flt[i] = i;
    yvec[0].elements.Flt[i] = V[xi + Nx*yi];
  }

  return (TRUE);

 usage: 
  gprint (GP_ERR, "USAGE: imcut <buffer> <X vector> <Y vector> xs ys xe ye\n");
  gprint (GP_ERR, " extract pixels along the line from (xs,ys) to (xe,ye)\n");
  gprint (GP_ERR, " (see also imvector)\n");
  return (FALSE);

 range:
  gprint (GP_ERR, "ERROR: coordinates out of range\n");
  return (FALSE);

 missed:
  gprint (GP_ERR, "ERROR: buffer not found\n");
  return (FALSE);
}

