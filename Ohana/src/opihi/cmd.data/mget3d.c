# include "data.h"

int mget3d (int argc, char **argv) {
  
  int i;
  Buffer *buf;
  Vector *vec;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: mget <buffer> <vector> x y\n");
    return (FALSE);
  }

  int x = atoi(argv[3]);
  int y = atoi(argv[4]);

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if (buf[0].matrix.Naxes < 3) {
    gprint (GP_ERR, "buffer is not 3D\n");
    return FALSE;
  }

  int Nx = buf[0].matrix.Naxis[0];
  int Ny = buf[0].matrix.Naxis[1];
  int Nz = buf[0].matrix.Naxis[2];

  int invalid = FALSE;
  invalid = invalid || (x < 0);
  invalid = invalid || (x >= Nx);
  invalid = invalid || (y < 0);
  invalid = invalid || (y >= Ny);
  if (invalid) {
    gprint (GP_ERR, "selection (%d,%d) out of range\n", x, y);
    return (FALSE);
  }

  if ((vec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (vec, OPIHI_FLT, Nz);
  float *in  = (float *) buf[0].matrix.buffer + x + y*Nx;
  opihi_flt *out = vec[0].elements.Flt;
  for (i = 0; i < Nz; i++, in += Nx*Ny, out++) {
    *out = *in;
  }
  return (TRUE);
}
