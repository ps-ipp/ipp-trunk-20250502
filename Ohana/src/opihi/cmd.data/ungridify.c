# include "data.h"

// XXX adapt this to accept an optional region to limit the areas, otherwise use the whole image
int ungridify (int argc, char **argv) {

  int i, j, n;
  int Nx, Ny, NX, NY;
  int Xmin, Xmax, Ymin, Ymax;
  Buffer *bf;
  float *v;
  Vector *vx, *vy, *vz;
  opihi_flt *x, *y, *z;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: ungridify buffer Xmin Xmax Ymin Ymax x y z\n");
    gprint (GP_ERR, "  convert a portion of an image to a collection a triplet of vectors\n");
    return (FALSE);
  }
  
  if ((bf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Xmin = atof (argv[2]);
  Xmax = atof (argv[3]);
  Ymin = atof (argv[4]);
  Ymax = atof (argv[5]);
  Nx = Xmax - Xmin;
  Ny = Ymax - Ymin;
  
  NX = bf[0].matrix.Naxis[0];
  NY = bf[0].matrix.Naxis[1];

  if ((vx = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vy = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vz = SelectVector (argv[8], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (vx, OPIHI_FLT, Nx*Ny);
  ResetVector (vy, OPIHI_FLT, Nx*Ny);
  ResetVector (vz, OPIHI_FLT, Nx*Ny);

  x = vx[0].elements.Flt;
  y = vy[0].elements.Flt;
  z = vz[0].elements.Flt;
  n = 0;
  v = (float *)bf[0].matrix.buffer;
  for (j = Ymin; j < Ymax; j++) {
    for (i = Xmin; i < Xmax; i++, x++, y++, z++, n++) {
      vx[0].elements.Flt[n] = i;
      vy[0].elements.Flt[n] = j;
      if (i < 0) continue;
      if (i >= NX) continue;
      if (j < 0) continue;
      if (j >= NY) continue;
      vz[0].elements.Flt[n] = v[i+j*NX];
    }
  }
  if (n != Nx*Ny) {
    gprint (GP_ERR, "error in ungrid: %d vs %d (%d x %d)\n", n, Nx*Ny, Nx, Ny);
    gprint (GP_ERR, "(this is probably a programming error in ungridify)\n");
  }
  vx[0].Nelements = vy[0].Nelements = vz[0].Nelements = n;
  return (TRUE);
}
