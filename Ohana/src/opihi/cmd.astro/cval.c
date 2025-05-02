# include "astro.h"

int cval (int argc, char **argv) {
  
  int i, j, Nx;
  int sx, sy, nx, ny, xo, yo, dx, dy;
  float *V, cval, val, sn, sky;
  Buffer *buf;

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: cval <buffer> x y dx dy sky\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  xo = atof (argv[2]);
  yo = atof (argv[3]);
  dx = atof (argv[4]);
  dy = atof (argv[5]);
  sky = atof (argv[6]);

  sx = xo - dx;
  sy = yo - dy;
  nx = 2*dx + 1;
  ny = 2*dy + 1;
  if ((sx < 0) || (sy < 0) || 
      (sx+nx > buf[0].matrix.Naxis[0]) || 
      (sy+ny > buf[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }

  V = (float *)buf[0].matrix.buffer;
  Nx = buf[0].matrix.Naxis[0];
  val = V[xo + yo*Nx];

  sn = 0;
  cval = 0;
  for (j = sy; j < sy + ny; j++) {
    for (i = sx; i < sx + nx; i++) {
      cval += (val - V[i + j*Nx]) / sqrt(V[i + j*Nx]);
      sn += SQ (V[i + j*Nx] - sky) / V[i + j*Nx];
    }
  }

  gprint (GP_ERR, "cval: %f  sn: %f\n", cval, sqrt(sn));

  return (TRUE);
}

