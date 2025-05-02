# include "astro.h"

int polar (int argc, char **argv) {
  
  double Lo, dL, Do, dD, Mo, dM, No, dN;
  double xo, yo, Xo, Yo;
  double x, y, r, t;
  float *Vin, *Vout, *Vmask;
  int i, j, nx, ny, Nx, Ny;
  int X, Y;
  Buffer *in, *out, *mask;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: polar in out\n");
    return (FALSE);
  }

  if ((in   = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out  = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((mask = SelectBuffer (argv[3], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  Nx = out[0].matrix.Naxis[0];
  Ny = out[0].matrix.Naxis[1];
  nx = mask[0].matrix.Naxis[0];
  ny = mask[0].matrix.Naxis[1];
  if ((Nx != nx) && (Ny != ny)) {
    gprint (GP_ERR, "output and mask must have same dimensions\n");
    return (FALSE);
  }
  nx = in[0].matrix.Naxis[0];
  ny = in[0].matrix.Naxis[1];

  /* we expect the output image to have units of longitude and distance */
  gfits_scan (&in[0].header, "CRVAL1", "%lf", 1, &Lo);
  gfits_scan (&in[0].header, "CDELT1", "%lf", 1, &dL);
  gfits_scan (&in[0].header, "CRPIX1", "%lf", 1, &xo);
  gfits_scan (&in[0].header, "CRVAL2", "%lf", 1, &Do);
  gfits_scan (&in[0].header, "CDELT2", "%lf", 1, &dD);
  gfits_scan (&in[0].header, "CRPIX2", "%lf", 1, &yo);

  /* we expect the input image to have units of distance X and Y */
  gfits_scan (&out[0].header, "CRVAL1", "%lf", 1, &Mo);
  gfits_scan (&out[0].header, "CDELT1", "%lf", 1, &dM);
  gfits_scan (&out[0].header, "CRPIX1", "%lf", 1, &Xo);
  gfits_scan (&out[0].header, "CRVAL2", "%lf", 1, &No);
  gfits_scan (&out[0].header, "CDELT2", "%lf", 1, &dN);
  gfits_scan (&out[0].header, "CRPIX2", "%lf", 1, &Yo);

  Vin  = (float *)in[0].matrix.buffer;
  Vout = (float *)out[0].matrix.buffer;
  Vmask = (float *)mask[0].matrix.buffer;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, Vout++, Vmask++) {
      x = (i - Xo) * dM + Mo;
      y = (j - Yo) * dN + No;
      r = hypot(x, y);
      t = ohana_normalize_angle(DEG_RAD*atan2 (y, x));
      X = (t - Lo) / dL + xo;
      Y = (r - Do) / dD + yo;
      if ((X >= 0) && (X < nx) && (Y >= 0) && (Y < ny)) {
	*Vout += Vin[Y*nx + X];
	*Vmask += 1;
      }
    }
  }

 return (TRUE);

}
