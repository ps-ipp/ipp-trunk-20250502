# include "data.h"

int minterp (int argc, char **argv) {
  
  int i, j, status, nx, ny, Nx, Ny, N, Extrapolate;
  int ic, jc, dx, dy, Npix;
  char temp[1024];
  double scale, scale2, dX, dY;
  float *V00, *V01, *V10, *V11, *Vout, dV1, dV2, dV3;
  float *buf, I, J, x, y, xs, xe, ys, ye;
  Buffer *in, *out;

  /* choose the appropriate graphing window */
  Extrapolate = FALSE;
  if ((N = get_argument (argc, argv, "-extrapolate"))) {
    remove_argument (N, &argc, argv);
    Extrapolate = TRUE;
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: minterpolate <from> <to> scale [-extrapolate]\n");
    return (FALSE);
  }

  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);

  scale  = atof (argv[3]);
  scale2 = scale*scale;
  Nx = in[0].header.Naxis[0];
  Ny = in[0].header.Naxis[1];
  nx = Nx * scale;
  ny = Ny * scale;

  /* create new matrix */
  out[0].bitpix = in[0].bitpix;
  out[0].unsign = in[0].unsign;
  out[0].bscale = in[0].bscale;
  out[0].bzero  = in[0].bzero;
  gfits_copy_header (&in[0].header, &out[0].header);

  gfits_modify (&out[0].header, "NAXIS1", "%d", 1, nx);
  gfits_modify (&out[0].header, "NAXIS2", "%d", 1, ny);
  out[0].header.Naxis[0] = nx;
  out[0].header.Naxis[1] = ny;
  gfits_create_matrix (&out[0].header, &out[0].matrix);

  /* fix astrometric terms */
  status =  gfits_scan (&out[0].header, "CDELT1", "%lf", 1, &dX);
  status &= gfits_scan (&out[0].header, "CDELT2", "%lf", 1, &dY);
  dX /= scale;
  dY /= scale;
  if (status) {
    gfits_modify (&out[0].header, "CDELT1", "%lf", 1, dX);
    gfits_modify (&out[0].header, "CDELT2", "%lf", 1, dY);
  }
  status =  gfits_scan (&out[0].header, "CRPIX1", "%lf", 1, &dX);
  status &= gfits_scan (&out[0].header, "CRPIX2", "%lf", 1, &dY);
  dX *= scale;
  dY *= scale;
  if (status) {
    gfits_modify (&out[0].header, "CRPIX1", "%lf", 1, dX);
    gfits_modify (&out[0].header, "CRPIX2", "%lf", 1, dY);
  }

  /* adjust filename */
  temp[0] = 0;
  if ((in[0].file[0] != '*') && (in[0].file[0] != '(')) {
    strcpy (temp, "*");
  }
  strcat (temp, in[0].file);
  strcpy (out[0].file, temp);

  dX = dY = scale;

  buf = (float *)in[0].matrix.buffer;
  Npix = 0;

  if (Extrapolate) {
    for (j = 0; j < Ny - 1; j++) {
      for (i = 0; i < Nx - 1; i++) {
	V00 = buf + i + j*Nx;
	V10 = V00 + 1;
	V01 = V00 + Nx;
	V11 = V01 + 1;
	dV1 = (*V11 + *V00 - *V01 - *V10) / scale2;
	dV2 = (*V01 - *V00) / scale;
	dV3 = (*V10 - *V00) / scale;

	x = (i + 0.5) * scale;
	y = (j + 0.5) * scale;

	xs = ys = 0;
	xe = ye = scale;

	if (i == 0)      { xs = -0.5*scale; }
	if (i == Nx - 2) { xe =  1.5*scale; }

	if (j == 0)      { ys = -0.5*scale; }
	if (j == Ny - 2) { ye =  1.5*scale; }

	for (J = ys; J < ye; J += 1.0) {
	  dx = (x + xs);
	  dy = (y + J);
	  Vout = (float *)(out[0].matrix.buffer) + dy*nx + dx;
	  for (I = xs; I < xe; I += 1.0, Vout++) {
	    *Vout = dV1 * (I*J) + dV2 * J + dV3 * I + *V00;
	    Npix ++;
	  }
	}
      }
    }
  } else {
     for (j = -1; j < Ny; j++) {
      for (i = -1; i < Nx; i++) {
	ic = MIN (MAX (i, 0), Nx-1);  /* we never actually reach Nx, Ny */
	jc = MIN (MAX (j, 0), Ny-1);
	V00 = buf + ic + jc*Nx;
	V10 = V00 + 1;
	V01 = V00 + Nx;
	V11 = V01 + 1;

	if ((i == -1) || (i == Nx - 1)) { V10 = V00; } else { V10 = V00 + 1; }
	if ((j == -1) || (j == Ny - 1)) { V01 = V00; } else { V01 = V00 + Nx; }
	if ((i == -1) || (i == Nx - 1)) { V11 = V01; } else { V11 = V01 + 1; }

	dV1 = (*V11 + *V00 - *V01 - *V10) / scale2;
	dV2 = (*V01 - *V00) / scale;
	dV3 = (*V10 - *V00) / scale;

	x = (i + 0.5) * scale;
	y = (j + 0.5) * scale;

	xs = ys = 0;
	xe = ye = scale;

	if (i == -1)     { xs = 0.5*scale; }
	if (i == Nx - 1) { xe = 0.5*scale; }

	if (j == -1)     { ys = 0.5*scale; }
	if (j == Ny - 1) { ye = 0.5*scale; }

	for (J = ys; J < ye; J += 1.0) {
	  dx = (x + xs);
	  dy = (y + J);
	  Vout = (float *)(out[0].matrix.buffer) + dy*nx + dx;
	  for (I = xs; I < xe; I += 1.0, Vout++) {
	    *Vout = dV1 * (I*J) + dV2 * J + dV3 * I + *V00;
	    Npix ++;
	  }
	}
      }
    }
  }
  return (TRUE);
}
