# include "data.h"

int imresample (int argc, char **argv) {
  
  Buffer *buf, *out;

  if (argc != 8) goto usage;

  if ((buf  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto missed;
  if ((out  = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) goto usage;
 
  double xs = atof (argv[3]);
  double ys = atof (argv[4]);
  double xe = atof (argv[5]);
  double ye = atof (argv[6]);
  int    Wo = atoi (argv[7]);

  int Nx = buf[0].matrix.Naxis[0];
  int Ny = buf[0].matrix.Naxis[1];

  // allow window to fall off the edge?
  if ((xs < 0) || (xs > Nx)) goto range;
  if ((ys < 0) || (ys > Ny)) goto range;
  if ((xe < 0) || (xe > Nx)) goto range;
  if ((ye < 0) || (ye > Ny)) goto range;

  // relationship between a pixel in the output window (x,y) and the input image (X,Y)
  // (0,0) corner of the output window is:
  // Xo,Yo = xs + dY * Wo/2, ys - dX * Wo/2
  // output window has size Lo, Wo
  // a pixel (x,y) in the output window has coords in the input image of:
  // (X,Y) = (Xo + x*dX - y*dY, Yo + x*dY + y*dX)

  double dX = xe - xs;
  double dY = ye - ys;
  double Lo = hypot (dX, dY);
  dX = dX / Lo;
  dY = dY / Lo;
  double Xo = xs + dY * Wo/2.0;
  double Yo = ys - dX * Wo/2.0;

  // dimensions of output window:
  int nx = Lo;
  int ny = Wo;

  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);
  if (!CreateBuffer (out, nx, ny, -32, 1.0, 0.0)) return FALSE;

  float *Vi = (float *)buf[0].matrix.buffer;
  float *Vo = (float *)out[0].matrix.buffer;
  for (int ix = 0; ix < nx; ix++) {
    for (int iy = 0; iy < ny; iy++) {
      int xi = Xo + ix*dX - iy*dY;
      int yi = Yo + ix*dY + iy*dX;
      if (xi < 0) continue;
      if (yi < 0) continue;
      if (xi >= Nx) continue;
      if (yi >= Ny) continue;
      Vo[ix + nx*iy] = Vi[xi + Nx*yi];
    }
  }
  return (TRUE);

 usage: 
  gprint (GP_ERR, "USAGE: imresample <buffer> <output> xs ys xe ye width\n");
  gprint (GP_ERR, "  resample image in new window along line (xs,ys) to (xe,ye) of given width\n");
  return (FALSE);

 range:
  gprint (GP_ERR, "ERROR: coordinates out of range\n");
  return (FALSE);

 missed:
  gprint (GP_ERR, "ERROR: buffer not found\n");
  return (FALSE);
}

