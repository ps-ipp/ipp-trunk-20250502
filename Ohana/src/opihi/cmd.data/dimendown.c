# include "data.h"

enum {VALUE, XCOORD, YCOORD};

int dimendown (int argc, char **argv) {
  
  int i, Nx, Ny, Npix, N, mode;
  float *in;
  opihi_flt *out;
  Vector *vec;
  Buffer *buf;

  mode = VALUE;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    mode = XCOORD;
  }
  if ((N = get_argument (argc, argv, "-y"))) {
    remove_argument (N, &argc, argv);
    mode = YCOORD;
  }

  if (argc != 3) goto usage;

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];
  Npix = Nx * Ny;

  ResetVector (vec, OPIHI_FLT, Npix);

  in = (float *) buf[0].matrix.buffer;
  out = vec[0].elements.Flt;

  switch (mode) {
    case VALUE:
      for (i = 0; i < Npix; i++, in++, out++) {
	*out = *in;
      }
      break;

    case XCOORD:
      for (i = 0; i < Npix; i++, out++) {
	*out = i % Nx;
      }
      break;

    case YCOORD:
      for (i = 0; i < Npix; i++, out++) {
	*out = i / Nx;
      }
      break;
  }
      
  return (TRUE);

 usage:
    gprint (GP_ERR, "USAGE: dimendown <buffer> <vector>\n");
    gprint (GP_ERR, "  -x : fill vector with buffer x-coords\n");
    gprint (GP_ERR, "  -y : fill vector with buffer y-coords\n");
    return (FALSE);
}
