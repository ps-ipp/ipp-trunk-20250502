# include "data.h"

int imsmooth_generic (int argc, char **argv) {
  
  int i, j, n;
  float *vi, *vo;
  float g, s;
  Buffer *in;
  Vector *vec;
  float *temp;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: imsmooth (input) vector\n");
    return (FALSE);
  }
  
  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (vec[0].Nelements % 2 == 0) {
    gprint (GP_ERR, "vector must have an odd number of elements\n");
    return FALSE;
  }

  int Nx = in[0].matrix.Naxis[0];
  int Ny = in[0].matrix.Naxis[1];
  ALLOCATE (temp, float, Nx*Ny);

  /* build a 1D gaussian */
  int Ns = 0.5*(vec[0].Nelements - 1);
  opihi_flt *smvec = &vec[0].elements.Flt[Ns];

  /* smooth in X direction */
  for (j = 0; j < Ny; j++) {
    vi = (float *) in[0].matrix.buffer + j*Nx;
    vo = &temp[j*Nx];
    for (i = 0; i < Nx; i++) {
      g = s = 0;
      for (n = -Ns; n < Ns + 1; n++) {
	if (i+n < 0) continue;
	if (i+n >= Nx) continue;
	s += smvec[n]*vi[i+n];
	g += smvec[n];
      }
      vo[i] = s / g;
    }
  }

  /* smooth in Y direction */
  for (i = 0; i < Nx; i++) {
    vi = &temp[i];
    vo = (float *)in[0].matrix.buffer + i;
    for (j = 0; j < Ny; j++) {
      g = s = 0;
      for (n = -Ns; n < Ns + 1; n++) {
	if (j+n < 0) continue;
	if (j+n >= Ny) continue;
	s += smvec[n]*vi[(n+j)*Nx];
	g += smvec[n];
      }
      vo[j*Nx] = s / g;
    }
  }

  free (temp);
  return (TRUE);
}

