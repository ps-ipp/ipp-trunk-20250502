# include "dimm.h"

int demux (int argc, char **argv) {
  
  int i, j, k, N;
  float *out, *in, *inptr, **outptr;
  int nx, ny, Nx, Ny, NX, NY, Nmux, Nbuf;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: demux <buffer> nx ny\n");
    return (FALSE);
  }

  if (!SelectBuffer (&Nbuf, argv[1], OLDBUFFER)) return (FALSE);

  nx = atof (argv[2]);
  ny = atof (argv[3]);
  Nmux = nx*ny;

  Nx = buffers[Nbuf].matrix.Naxis[0];
  Ny = buffers[Nbuf].matrix.Naxis[1];

  NX = Nx / nx;
  NY = Ny / ny;

  inptr = in = (float *) buffers[Nbuf].matrix.buffer;  /* don't lose reference */

  ALLOCATE (out, float, Nx*Ny);
  ALLOCATE (outptr, float *, Nmux);
  
  for (N = i = 0; i < nx; i++) {
    for (j = 0; j < ny; j++, N++) {
      outptr[N] = &out[i*NX + j*NX*NY*nx];
    }
  }

  for (N = j = 0; j < NY; j++) {
    for (i = 0; i < NX; i++) {
      for (k = 0; k < Nmux; k++) {
	*outptr[k] = *inptr;
	outptr[k] ++;
	inptr ++;
      }
    }
    for (k = 0; k < Nmux; k++) outptr[k] += NX;
  }

  /*
  for (X = Y = x = y = i = 0; i < NX*NY; i++) {
    out[X + Y*NX + x*NX*(Y+1) + y*NX*NY*nx] = *in;
    X++;
    if (X == NX) {
      X = 0;
      Y++;
    }
    if (Y == NY) {
      Y = 0;
      x++;
    }
    if (x == nx) {
      x = 0;
      y++;
    }
  }
  */

  free (in);
  buffers[Nbuf].matrix.buffer = (char *) out;

  return (TRUE);
}

