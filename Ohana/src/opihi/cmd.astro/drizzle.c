# include "astro.h"

/*** needs mosaic astrometry ***/

static double XO, XX, XY;
static double YO, YX, YY;
static int ZERO;

static int ROTATE;
static double rot_phi, rot_alpha, rot_delta;
static double rot_cdp, rot_sdp;

int map_output_to_input (int Npix, double df);
int map_input_to_output (int Npix, double df);
int set_linear_terms (Coords *in, Coords *out, int i, int j, int Npix);
void apply_terms (double *Xout, double *Yout, double Xin, double Yin);

Coords coords_in, coords_out;
Buffer *in, *out, *wt, *mask;

int drizzle (int argc, char **argv) {

  int Nlinear, Np, N;
  double scale_in, scale_out, df;

  ZERO = FALSE;
  if ((N = get_argument (argc, argv, "-zero"))) {
    ZERO = TRUE;
    remove_argument (N, &argc, argv);
  }

  ROTATE = FALSE;
  if ((N = get_argument (argc, argv, "-roll"))) {
    /* -roll phi alpha_pole delta_pole */
    /* XXX need to clarify the meaning of phi, alpha, delta */
    ROTATE = TRUE;
    remove_argument (N, &argc, argv);
    rot_phi   = atof (argv[N]);
    remove_argument (N, &argc, argv);
    rot_alpha = atof (argv[N]);
    remove_argument (N, &argc, argv);
    rot_delta = atof (argv[N]);
    remove_argument (N, &argc, argv);

    rot_cdp = cos(RAD_DEG*rot_delta);
    rot_sdp = sin(RAD_DEG*rot_delta);
  }

  mask = NULL;
  if ((N = get_argument (argc, argv, "-mask"))) {
    remove_argument (N, &argc, argv);
    if ((mask = SelectBuffer (argv[N], OLDBUFFER, TRUE)) == NULL) return (FALSE);
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
  float *Vin, *Vout, *Vwt, *Vmk;
  double x, y, X, Y;

  /* loop over output pixels */
  /* set up pointers for buffers */
  Vin  = (float *) in[0].matrix.buffer;
  Vout = (float *) out[0].matrix.buffer;
  Vwt  = (float *) wt[0].matrix.buffer;
  Vmk  = NULL;
  Vmk = (mask == NULL) ? NULL : (float *) mask[0].matrix.buffer;

  nx = in[0].header.Naxis[0];
  ny = in[0].header.Naxis[1];
  Nx = out[0].header.Naxis[0];
  Ny = out[0].header.Naxis[1];

  if (ZERO) {
    bzero (Vout, Nx*Ny*sizeof(float));
    bzero (Vwt,  Nx*Ny*sizeof(float));
  }

  gprint (GP_ERR, "mapping output to input\n");

  for (j = 0; j < Ny; j+=Npix) {
    for (i = 0; i < Nx; i+=Npix) {
      
      /* define linear transformation in region */
      if (!set_linear_terms (&coords_out, &coords_in, i, j, Npix)) continue;

      for (X = i; (X < i + Npix) && (X < Nx); X += df) {
	for (Y = j; (Y < j + Npix) && (Y < Ny); Y += df) {
	  
	  No = (int)X + ((int)Y)*Nx;
	  apply_terms (&x, &y, X, Y);
	  if (x < 0) continue;
	  if (x >= nx) continue;
	  if (y < 0) continue;
	  if (y >= ny) continue;
	  Ni = (int)x + ((int)y)*nx;

	  if (Vmk && Vmk[Ni]) continue;
	  if (!isfinite(Vin[Ni])) continue;

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
  float *Vin, *Vout, *Vwt, *Vmk;
  double x, y, X, Y;

  /* loop over output pixels */
  /* set up pointers for buffers */
  Vin  = (float *) in[0].matrix.buffer;
  Vout = (float *) out[0].matrix.buffer;
  Vwt  = (float *) wt[0].matrix.buffer;
  Vmk  = NULL;
  Vmk = (mask == NULL) ? NULL : (float *) mask[0].matrix.buffer;

  Nx = in[0].header.Naxis[0];
  Ny = in[0].header.Naxis[1];
  nx = out[0].header.Naxis[0];
  ny = out[0].header.Naxis[1];

  if (ZERO) {
    bzero (Vout, nx*ny*sizeof(float));
    bzero (Vwt,  nx*ny*sizeof(float));
  }

  gprint (GP_ERR, "mapping input to output\n");

  for (j = 0; j < Ny; j+=Npix) {
    for (i = 0; i < Nx; i+=Npix) {
      
      /* define linear transformation in region */
      if (!set_linear_terms (&coords_in, &coords_out, i, j, Npix)) continue;

      for (X = i; (X < i + Npix) && (X < Nx); X += df) {
	for (Y = j; (Y < j + Npix) && (Y < Ny); Y += df) {
	  
	  Ni = (int)X + ((int)Y)*Nx;
	  apply_terms (&x, &y, X, Y);
	  if (x < 0) continue;
	  if (x >= nx) continue;
	  if (y < 0) continue;
	  if (y >= ny) continue;
	  No = (int)x + ((int)y)*nx;

	  if (Vmk && Vmk[Ni]) continue;
	  if (!isfinite(Vin[Ni])) continue;
	  Vout[No] += Vin[Ni];
	  Vwt[No] ++;
	}
      }
    }
  }
  return (TRUE);
}

int rotate_coords (double *phi, double *theta, double alpha, double delta) {

  double sda, cda, cd, sd, sth;
  double x, y;
  
  sda = sin(RAD_DEG*(alpha - rot_alpha));
  cda = cos(RAD_DEG*(alpha - rot_alpha));
  sd = sin(RAD_DEG*delta);
  cd = cos(RAD_DEG*delta);
  
  sth = -cd*sda*rot_cdp + sd*rot_sdp;
  y   = +cd*sda*rot_sdp + sd*rot_cdp;
  x   = +cd*cda;

  *theta = DEG_RAD*asin(sth);
  *phi   = DEG_RAD*atan2(y,x) + rot_phi;
  
  *phi = ohana_normalize_angle(*phi);
  return (TRUE);
}

/* find the linear astrometric fix between images at this location */
int set_linear_terms (Coords *in, Coords *out, int i, int j, int Npix) {

  int n;
  double x, y, x2, y2, xy, X, Y, Xx, Xy, Yx, Yy;
  double Xin, Yin, Xout, Yout;
  double Sx2, Sy2, Sxy, SXx, SXy, SYx, SYy;
  double N, r, d, phi, theta;

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

    if (!XY_to_RD (&r, &d, Xin, Yin, in)) return (FALSE);
    if (ROTATE) { 
      rotate_coords (&phi, &theta, r, d);
      r = phi;
      d = theta;
    }
    if (!RD_to_XY (&Xout, &Yout, r, d, out)) return (FALSE);

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
  return (TRUE);
}


void apply_terms (double *Xout, double *Yout, double Xin, double Yin) {
  *Xout = XO + XX*Xin + XY*Yin;
  *Yout = YO + YX*Xin + YY*Yin;
}
