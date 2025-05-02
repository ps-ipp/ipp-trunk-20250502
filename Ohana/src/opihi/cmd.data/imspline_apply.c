# include "data.h"

// need to rename this as an image function
int imspline_apply (int argc, char **argv) {
  
  int i, j, I, J;
  int nx, ny, Nx, Ny;
  float rx, ry, x, y;
  float *Tx1, *Tx2, *Txc, *Ty1, *Ty2, *Tyc, *V, *V1, *V2;
  Buffer *out, *y1, *y2;

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: imspline_apply <Y> <Y2> <out> (x/y) Nx Ny\n");
    return (FALSE);
  }

  if ((y1  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((y2  = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[3], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  // xdir = FALSE;
  // if (!strcmp (argv[4], "x")) xdir = TRUE; 

  nx = atoi (argv[5]);
  ny = atoi (argv[6]);

  Nx = y1[0].matrix.Naxis[0];
  Ny = y1[0].matrix.Naxis[1];

  rx = Nx / (float) nx;
  ry = Ny / (float) ny;

  /* create an output matrix buffer with desired nx, ny */
  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);

  out[0].bitpix = y1[0].bitpix;
  out[0].unsign = y1[0].unsign;
  out[0].bscale = y1[0].bscale;
  out[0].bzero  = y1[0].bzero;
  gfits_copy_header (&y1[0].header, &out[0].header);
  gfits_modify (&out[0].header, "NAXIS1", "%d", 1, nx);
  gfits_modify (&out[0].header, "NAXIS2", "%d", 1, ny);

  out[0].header.Naxis[0] = nx;
  out[0].header.Naxis[1] = ny;
  gfits_create_matrix (&out[0].header, &out[0].matrix);
  if ((y1[0].file[0] != '*') && (y1[0].file[0] != '(')) {
    snprintf_nowarn (out[0].file, OPIHI_NAME_SIZE, "*%s", y1[0].file);
  } else {
    snprintf_nowarn (out[0].file, OPIHI_NAME_SIZE, "%s", y1[0].file);
  }

  ALLOCATE (Ty2, float, Ny);
  ALLOCATE (Ty1, float, Ny);
  ALLOCATE (Tyc, float, Ny);
  for (i = 0; i < Ny; i++) { Tyc[i] = i; }

  ALLOCATE (Tx1, float, Nx);
  ALLOCATE (Tx2, float, Nx);
  ALLOCATE (Txc, float, Nx);
  for (i = 0; i < Nx; i++) { Txc[i] = i; }

  V = (float *)(out[0].matrix.buffer);

  for (J = 0; J < ny; J++) {
    y = J * ry;

    /* construct spline for each element in this row */
    for (i = 0; i < Nx; i++) {
      V1 = (float *)(y1[0].matrix.buffer) + i;
      V2 = (float *)(y2[0].matrix.buffer) + i;
      for (j = 0; j < Ny; j++, V1+=Nx, V2+=Nx) {
	Ty1[j] = *V1;
	Ty2[j] = *V2;
      }
      Tx1[i] = spline_apply_flt (Tyc, Ty1, Ty2, Ny, y);
    }
    spline_construct_flt (Txc, Tx1, Nx, Tx2, NAN, NAN);

    /* apply x-dir spline to new image */
    for (I = 0; I < nx; I++, V++) {
      x = I * rx;
      *V = spline_apply_flt (Txc, Tx1, Tx2, Nx, x);
    }
  }

  free (Ty1);
  free (Ty2);
  free (Tyc);
  
  free (Tx1);
  free (Tx2);
  free (Txc);
  
  return (TRUE);

}
