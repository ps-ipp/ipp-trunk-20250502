# include "data.h"

// need to rename this as an image function
int imspline_construct (int argc, char **argv) {
  
  int i, j, Nx, Ny;
  float *Tx, *Ty, *Ty2, *V;
  Buffer *in, *out;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: imspline_construct <in> <out> (x/y)\n");
    return (FALSE);
  }

  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  // XXX move this to gfits_create_matrix
  free (out[0].matrix.buffer);
  if ((in[0].file[0] != '*') && (in[0].file[0] != '(')) {
    snprintf_nowarn (out[0].file, OPIHI_NAME_SIZE, "*%s", in[0].file);
  } else {
    snprintf_nowarn (out[0].file, OPIHI_NAME_SIZE, "%s", in[0].file);
  }
  out[0].bitpix = in[0].bitpix;
  out[0].unsign = in[0].unsign;
  out[0].bscale = in[0].bscale;
  out[0].bzero  = in[0].bzero;
  gfits_copy_matrix_info (&in[0].matrix, &out[0].matrix);
  gfits_copy_header (&in[0].header, &out[0].header);
  gfits_create_matrix (&out[0].header, &out[0].matrix);

  // int xdir = FALSE;
  // if (!strcmp (argv[3], "x")) xdir = TRUE; 
  /* ideally, the resulting image should carry this info (in header?) */

  Nx = in[0].matrix.Naxis[0];
  Ny = in[0].matrix.Naxis[1];

  ALLOCATE (Ty2, float, Ny);
  ALLOCATE (Ty, float, Ny);
  ALLOCATE (Tx, float, Ny);

  /** for now only perform the operation for the ydir splines */

  /* construct coordinate vector */
  for (j = 0; j < Ny; j++) { Tx[j] = j; }
  
  for (i = 0; i < Nx; i++) {
    
    /* construct temp vector with values to spline */
    V = (float *)(in[0].matrix.buffer) + i;
    for (j = 0; j < Ny; j++, V+=Nx) {
      Ty[j] = *V;
    }
  
    spline_construct_flt (Tx, Ty, Ny, Ty2, NAN, NAN);
  
    /* copy derivatives to output buffer */
    V = (float *)(out[0].matrix.buffer) + i;
    for (j = 0; j < Ny; j++, V+=Nx) {
      *V = Ty2[j];
    }
  }

  free (Tx);
  free (Ty);
  free (Ty2);
  
  return (TRUE);

}
