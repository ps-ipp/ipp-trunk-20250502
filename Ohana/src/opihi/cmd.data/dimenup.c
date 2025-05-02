# include "data.h"

int dimenup (int argc, char **argv) {
  
  int i, Nx, Ny, Npix;
  float *out;
  Vector *vec;
  Buffer *buf;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: dimenup <vector> <buffer> Nx Ny\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((buf = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  Npix = vec[0].Nelements;
  Nx = atof (argv[3]);
  Ny = atof (argv[4]);
  if (Npix != Nx * Ny) {
    gprint (GP_ERR, "dimensions don't match\n");
    return (FALSE);
  }
  ResetBuffer (buf, Nx, Ny, -32, 0.0, 1.0);

  out = (float *) buf[0].matrix.buffer;

  if (vec[0].type == OPIHI_FLT) {
    opihi_flt *in = vec[0].elements.Flt;
    for (i = 0; i < Npix; i++, in++, out++) {
      *out = *in;
    }
  } else {
    opihi_int *in = vec[0].elements.Int;
    for (i = 0; i < Npix; i++, in++, out++) {
      *out = *in;
    }
  }

  return (TRUE);
}

