# include "data.h"

int grow (int argc, char **argv) {
  
  int i, j, nx, ny, npix, nsum, Nx, Ny;
  float *input, *output;
  Buffer *in;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: grow (input) (npix)\n");
    return (FALSE);
  }
  
  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  npix = atoi (argv[2]);

  Nx = in[0].matrix.Naxis[0];
  Ny = in[0].matrix.Naxis[1];
  ALLOCATE (output, float, Nx*Ny);
  memset (output, 0, Nx*Ny*sizeof(float));

  input = (float *) in[0].matrix.buffer;

  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++) {
      nsum = 0;
      for (ny = -1; ny <= +1; ny++) {
	if (j + ny < 0) continue;
	if (j + ny >= Ny) continue;
	for (nx = -1; nx <= +1; nx++) {
	  if (i + nx < 0) continue;
	  if (i + nx >= Nx) continue;
	  if (!nx && !ny) continue;
	  if (input[i + nx + Nx*(j + ny)] == 0) continue;
	  nsum ++;
	}
      }
      if (nsum >= npix) {
	output[i + j*Nx] = 1;
      } else {
	output[i + j*Nx] = input[i + j*Nx];
      }
    }
  }
  
  free (in[0].matrix.buffer);
  in[0].matrix.buffer = (char *) output;
  return (TRUE);
}

