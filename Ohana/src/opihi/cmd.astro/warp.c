# include "astro.h"

/*** needs mosaic astrometry ***/

static double XO, XX, XY;
static double YO, YX, YY;
int ZERO;

int map_output_to_input (int Npix, double df);
int map_input_to_output (int Npix, double df);
void set_linear_terms (Coords *in, Coords *out, int i, int j, int Npix);
void apply_terms (double *Xout, double *Yout, double Xin, double Yin);

Coords coords_in, coords_out;
Buffer *in, *out, *wt;

int warp (int argc, char **argv) {

  int Nlinear, Np, N;
  double scale_in, scale_out, df;

  ZERO = FALSE;
  if ((N = get_argument (argc, argv, "-zero"))) {
    ZERO = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: transform <from> <to> <weight> (Nlinear)\n");
    gprint (GP_ERR, "  output buffer must exist with target astrometry header\n");
    gprint (GP_ERR, "  Nlinear is the pixel scale for linear astrometric transformation\n");
    return (FALSE);
  }

  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((wt  = SelectBuffer (argv[3], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nlinear = atoi (argv[4]);

  GetCoords (&coords_in, &in[0].header);
  GetCoords (&coords_out, &out[0].header);

  /* for the moment, disable WRP / DIS */
  if (!strcmp(&coords_in.ctype[4], "-WRP") || !strcmp(&coords_out.ctype[4], "-WRP")) {
    gprint (GP_ERR, "WRP mode not implemented for astrom\n");
    return (FALSE);
  }
  
  scale_in = sqrt(fabs(coords_in.cdelt1*coords_in.cdelt2*(coords_in.pc1_1*coords_in.pc2_2 - coords_in.pc1_2*coords_in.pc2_1)));
  scale_out = sqrt(fabs(coords_out.cdelt1*coords_out.cdelt2*(coords_out.pc1_1*coords_out.pc2_2 - coords_out.pc1_2*coords_out.pc2_1)));
  
  gprint (GP_ERR, "%f - %f\n", scale_in, scale_out);

  if (scale_in > scale_out) {
    Np = MAX (1, 3*scale_out / scale_in);
    df = 1.0 / Np;
    map_output_to_input (Nlinear, df);
  } else {
    Np = MAX (1, 3*scale_in / scale_out);
    df = 1.0 / Np;
    map_input_to_output (Nlinear, df);
  }
  return (TRUE);
}

/* mode 1: input pixels >> output pixels: loop over output pixels */
/* mode 2: input pixels << output pixels: loop over input pixels */
/* mode 3: input pixels ~= output pixels: drizzle input to output */

/* loop over the input pixels, map input output image */
int map_output_to_input (int Npix, double df) {

  int i, j, Ni, No, Nx, Ny, nx, ny;
  float *Vin, *Vout, *Vwt;
  double x, y, X, Y;

  /* loop over output pixels */
  /* set up pointers for buffers */
  Vin  = (float *) in[0].matrix.buffer;
  Vout = (float *) out[0].matrix.buffer;
  Vwt  = (float *) wt[0].matrix.buffer;

  nx = in[0].header.Naxis[0];
  ny = in[0].header.Naxis[1];
  Nx = out[0].header.Naxis[0];
  Ny = out[0].header.Naxis[1];

  if (ZERO) {
    bzero (Vout, Nx*Ny*sizeof(float));
    bzero (Vwt,  Nx*Ny*sizeof(float));
  }

  for (j = 0; j < Ny; j+=Npix) {
    for (i = 0; i < Nx; i+=Npix) {
      
      /* define linear transformation in region */
      set_linear_terms (&coords_out, &coords_in, i, j, Npix);

      for (X = i; (X < i + Npix) && (X < Nx); X += df) {
	for (Y = j; (Y < j + Npix) && (Y < Ny); Y += df) {
	  
	  No = (int)X + ((int)Y)*Nx;
	  apply_terms (&x, &y, X, Y);
	  if (x < 0) continue;
	  if (x >= nx) continue;
	  if (y < 0) continue;
	  if (y >= ny) continue;
	  Ni = (int)x + ((int)y)*nx;

	  Vout[No] += Vin[Ni];
	  Vwt[No] ++;
	}
      }
    }
  }
  return (TRUE);
}

/* loop over the input pixels, map input output image */
int map_input_to_output (int Npix, double df) {

  int i, j, Ni, No, Nx, Ny, nx, ny;
  float *Vin, *Vout, *Vwt;
  double x, y, X, Y;

  /* loop over output pixels */
  /* set up pointers for buffers */
  Vin  = (float *) in[0].matrix.buffer;
  Vout = (float *) out[0].matrix.buffer;
  Vwt  = (float *) wt[0].matrix.buffer;

  Nx = in[0].header.Naxis[0];
  Ny = in[0].header.Naxis[1];
  nx = out[0].header.Naxis[0];
  ny = out[0].header.Naxis[1];

  if (ZERO) {
    bzero (Vout, nx*ny*sizeof(float));
    bzero (Vwt,  nx*ny*sizeof(float));
  }

  for (j = 0; j < Ny; j+=Npix) {
    for (i = 0; i < Nx; i+=Npix) {
      
      /* define linear transformation in region */
      set_linear_terms (&coords_in, &coords_out, i, j, Npix);

      for (X = i; (X < i + Npix) && (X < Nx); X += df) {
	for (Y = j; (Y < j + Npix) && (Y < Ny); Y += df) {
	  
	  Ni = (int)X + ((int)Y)*Nx;
	  apply_terms (&x, &y, X, Y);
	  if (x < 0) continue;
	  if (x >= nx) continue;
	  if (y < 0) continue;
	  if (y >= ny) continue;
	  No = (int)x + ((int)y)*nx;

	  Vout[No] += Vin[Ni];
	  Vwt[No] ++;
	}
      }
    }
  }
  return (TRUE);
}

/* find the linear astrometric fix between images at this location */
void set_linear_terms (Coords *in, Coords *out, int i, int j, int Npix) {

  int n;
  double x, y, x2, y2, xy, X, Y, Xx, Xy, Yx, Yy;
  double Xin, Yin, Xout, Yout;
  double Sx2, Sy2, Sxy, SXx, SXy, SYx, SYy;
  double N, r, d;

  Xin = Yin = 0;
  N = x = y = x2 = y2 = xy = X = Y = Xx = Xy = Yx = Yy = 0;

  /* define several test points, fit a line to the input,output pairs */
  for (n = 0; n < 3; n++) {

    switch (n) {
    case 0:
      Xin = i;
      Yin = j;
      break;
    case 1:
      Xin = i + Npix;
      Yin = j;
      break;
    case 2:
      Xin = i;
      Yin = j + Npix;
      break;
    }

    XY_to_RD (&r, &d, Xin, Yin, in);
    RD_to_XY (&Xout, &Yout, r, d, out);

    x  += Xin;
    y  += Yin;
    x2 += Xin*Xin;
    y2 += Yin*Yin;
    xy += Xin*Yin;
    X  += Xout;
    Y  += Yout;
    Xx += Xout*Xin;
    Xy += Xout*Yin;
    Yx += Yout*Xin;
    Yy += Yout*Yin;
    N  += 1.0;
  }

  Sx2 = x2 - x*x/N;
  Sy2 = y2 - y*y/N;
  Sxy = xy - x*y/N;
  SXx = Xx - X*x/N;
  SXy = Xy - X*y/N;
  SYx = Yx - Y*x/N;
  SYy = Yy - Y*y/N;
  
  XX = (SXx*Sy2 - SXy*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  XY = (SXy*Sx2 - SXx*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  XO = X/N - XX*x/N - XY*y/N;

  YX = (SYx*Sy2 - SYy*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  YY = (SYy*Sx2 - SYx*Sxy) / (Sx2*Sy2 - Sxy*Sxy);
  YO = Y/N - YX*x/N - YY*y/N;

}


void apply_terms (double *Xout, double *Yout, double Xin, double Yin) {
  *Xout = XO + XX*Xin + XY*Yin;
  *Yout = YO + YX*Xin + YY*Yin;
}
