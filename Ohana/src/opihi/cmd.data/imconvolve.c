# include "data.h"

int imconvolve (int argc, char **argv) {
  
  Buffer *in, *ker, *out;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: imconvolve (input) (kernel) (output)\n");
    return (FALSE);
  }
  
  if ((in   = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((ker  = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out  = SelectBuffer (argv[3], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  // output image will have same size as input1 (not padding the edges
  int Nx = in[0].matrix.Naxis[0];
  int Ny = in[0].matrix.Naxis[1];

  int nx = in[0].matrix.Naxis[0];
  int ny = in[0].matrix.Naxis[1];

  int djx = nx / 2;
  int djy = ny / 2;

  // ALLOCATE (temp, float, Nx*Ny);

  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);
  if (!CreateBuffer (out, Nx, Ny, -32, 1.0, 0.0)) return FALSE;

  float *inBuf  = (float *)  in[0].matrix.buffer;
  float *kerBuf = (float *) ker[0].matrix.buffer;
  float *outBuf = (float *) out[0].matrix.buffer;

  for (int iy = 0; iy < Ny; iy ++) {
    for (int ix = 0; ix < Nx; ix ++) {

      double outValue = 0.0;

      // loop over all kernel pixels
      for (int jy = 0; jy < ny; jy++) {
	int ky = iy + jy - djy;
	if (ky < 0) continue;
	if (ky >= Ny) continue;
	for (int jx = 0; jx < nx; jx++) {
	  int kx = ix + jx - djx;
	  if (kx < 0) continue;
	  if (kx >= Nx) continue;
	  outValue += inBuf[kx + Nx*ky]*kerBuf[jx + jy*ny];
	}
      }
      outBuf[ix + iy*Nx] = outValue;
    }
  }

  return (TRUE);
}

