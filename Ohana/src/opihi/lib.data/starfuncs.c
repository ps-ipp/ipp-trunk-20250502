# include "data.h"

double get_aperture_stats (Matrix *matrix, int X, int Y, int Npix, int Nborder, double max, int VERBOSE) {

  double *ring;
  double x, y, x2, y2, xy, I, sky, FWHMx, FWHMy, value, mag, Sxy;
  int i, j, n, Radius, Nring, Nmax;
  double Npts, gain, dsky2, dmag, peak, offset;
  char *string;
  
  string = get_variable ("GAIN");
  if (string == (char *) NULL) {
    gprint (GP_ERR, "assuming a value of 1.0\n");
    gain = 1.0;
  } else {
    gain = atof (string);
  }
  Nborder = MAX (1, Nborder);
  Nborder = MIN (1000, Nborder);
  
  Radius = (int)(0.5*Npix);
  Npix = 2 * Radius + 1;
  Nring = 4*Nborder*(Nborder + Npix);
  ALLOCATE (ring, double, Nring);
  bzero (ring, sizeof(double)*Nring);

  n = 0;  
  for (j = 0; j < Nborder; j++) {
    for (i = X - Radius - Nborder; i < X + Radius + Nborder + 1; i++, n+=2) {
      ring[n]   = gfits_get_matrix_value (matrix, i, (int)(Y - Radius - j));
      ring[n+1] = gfits_get_matrix_value (matrix, i, (int)(Y + Radius + j));
    }
    for (i = Y - Radius; i < Y + Radius + 1; i++, n+=2) {
      ring[n]   = gfits_get_matrix_value (matrix, (int)(X - Radius - j), i);
      ring[n+1] = gfits_get_matrix_value (matrix, (int)(X + Radius + j), i);
    }
  }
  dsort (ring, Nring);
  for (Npts = sky = dsky2 = 0, i = 0.25*Nring; i < 0.75*Nring; i++, Npts += 1.0) {
    sky += ring[i];
    dsky2 += ring[i]*ring[i];
  }
  sky = sky / Npts;
  dsky2 = dsky2 / Npts - sky*sky;
  free (ring);

  float dx, dy;

  peak = 0;
  Npts = Nmax = 0;
  x = y = x2 = y2 = xy = I = 0;
  for (i = X - Radius; i < X + Radius + 1; i++) {
    for (j = Y - Radius; j < Y + Radius + 1; j++) {
      if (hypot((i-X), (j-Y)) > Radius) continue;
      value = gfits_get_matrix_value (matrix, i, j);
      offset = value - sky;
      dx = i - X;
      dy = j - Y;
      x  += dx*offset;
      y  += dy*offset;
      x2 += dx*dx*offset;
      y2 += dy*dy*offset;
      xy += dx*dy*offset;
      I  += offset;
      Npts ++;
      if (value > max) {
	Nmax ++;
      }
      if (value > peak) peak = value;
    }
  }

  x = x / I;
  y = y / I;
  double Mxx = x2 / I - x*x;
  double Myy = y2 / I - y*y;
  FWHMx = 2.355*sqrt (fabs(Mxx));
  FWHMy = 2.355*sqrt (fabs(Myy));

  // fprintf (stderr, "Mxx, Myy: %f, %f\n", x2/I - x*x, y2/I - y*y);
  Sxy   = xy / I - x*y;
  mag = -2.5*log10(I);
  dmag = sqrt (fabs(1.0 / (gain*I) + Npts*dsky2 / (I*I)));
  x = x + X;
  y = y + Y;
  
  set_variable ("Xg", x);
  set_variable ("Yg", y);
  set_variable ("SXg", FWHMx);
  set_variable ("SYg", FWHMy);
  set_variable ("MXXg", Mxx);
  set_variable ("MYYg", Myy);
  set_variable ("SXYg", Sxy);
  set_variable ("Sg", sky);
  set_variable ("dSg", sqrt (fabs (dsky2)));
  set_variable ("Zg", mag);
  set_variable ("dZg", dmag);
  set_variable ("Zcg", I);
  set_variable ("Zpk", peak);
  set_int_variable ("Nsat", Nmax);
  set_int_variable ("Npts", Npts);
  
  if (VERBOSE) gprint (GP_LOG, "%f %f %f %f %f %f %f %f\n", x, y, FWHMx, FWHMy, sky, I, mag, dmag);

  return (mag);

}

double get_box_stats (Matrix *matrix, int X, int Y, int dX, int dY, int Nborder, double max, int VERBOSE) {

  double *ring;
  double x, y, x2, y2, xy, I, sky, FWHMx, FWHMy, value, mag, Sxy;
  int i, j, n, Nring, Nmax;
  double Npts, gain, dsky2, dmag, peak, offset;
  char *string;
  
  string = get_variable ("GAIN");
  if (string == (char *) NULL) {
    gprint (GP_ERR, "assuming a value of 1.0\n");
    gain = 1.0;
  } else {
    gain = atof (string);
  }
  Nborder = MAX (1, Nborder);
  Nborder = MIN (1000, Nborder);
  
  int dX2 = (int)(0.5*dX);
  int dY2 = (int)(0.5*dY);
  dX = 2 * dX2 + 1;
  dY = 2 * dY2 + 1;

  Nring = 2*Nborder*(dX + 2*Nborder) + 2*Nborder*(dY + 2*Nborder);
  ALLOCATE (ring, double, Nring);
  bzero (ring, sizeof(double)*Nring);

  // get the pixels in the border regions:
  // XXX gfits_get_matrix_value returns 0 for out-of-bounds pixels, but should return NAN
  // and they should be skipped
  n = 0;  
  for (j = 0; j < Nborder; j++) {
    for (i = X - dX2 - Nborder; i < X + dX2 + Nborder + 1; i++) {
      value = gfits_get_matrix_value (matrix, i, (int)(Y - dY2 - j));
      if (isfinite(value)) { ring[n] = value; n++; }
      value = gfits_get_matrix_value (matrix, i, (int)(Y + dY2 + j));
      if (isfinite(value)) { ring[n] = value; n++; }
    }
    for (i = Y - dY2; i < Y + dY2 + 1; i++) {
      value = gfits_get_matrix_value (matrix, (int)(X - dX2 - j), i);
      if (isfinite(value)) { ring[n] = value; n++; }
      value = gfits_get_matrix_value (matrix, (int)(X + dX2 + j), i);
      if (isfinite(value)) { ring[n] = value; n++; }
    }
  }
  Nring = n;
  dsort (ring, Nring);
  for (Npts = sky = dsky2 = 0, i = 0.25*Nring; i < 0.75*Nring; i++, Npts += 1.0) {
    sky += ring[i];
    dsky2 += ring[i]*ring[i];
  }
  sky = sky / Npts;
  dsky2 = dsky2 / Npts - sky*sky;
  free (ring);

  float dx, dy;

  peak = 0;
  Npts = Nmax = 0;
  x = y = x2 = y2 = xy = I = 0;
  for (i = X - dX2; i < X + dX2 + 1; i++) {
    for (j = Y - dY2; j < Y + dY2 + 1; j++) {
      value = gfits_get_matrix_value (matrix, i, j);
      if (!isfinite(value)) continue;
      offset = value - sky;
      dx = i - X;
      dy = j - Y;
      x  += dx*offset;
      y  += dy*offset;
      x2 += dx*dx*offset;
      y2 += dy*dy*offset;
      xy += dx*dy*offset;
      I  += offset;
      Npts ++;
      if (value > max) {
	Nmax ++;
      }
      if (value > peak) peak = value;
    }
  }

  x = x / I;
  y = y / I;
  FWHMx = 2.355*sqrt (fabs(x2 / I - x*x));
  FWHMy = 2.355*sqrt (fabs(y2 / I - y*y));
  Sxy   = xy / I - x*y;
  mag = -2.5*log10(I);

  // flux_error = sqrt( I + Npts*dsky2 )
  // dmag = 1.086 * flux_error / flux
  dmag = 1.086 * sqrt (fabs(I + Npts*dsky2)) / (gain * I);
  x = x + X;
  y = y + Y;
  
  set_variable ("Xg", x);
  set_variable ("Yg", y);
  set_variable ("SXg", FWHMx);
  set_variable ("SYg", FWHMy);
  set_variable ("SXYg", Sxy);
  set_variable ("Sg", sky);
  set_variable ("dSg", sqrt (fabs (dsky2)));
  set_variable ("Zg", mag);
  set_variable ("dZg", dmag);
  set_variable ("Zcg", I);
  set_variable ("Zpk", peak);
  set_int_variable ("Nsat", Nmax);
  set_int_variable ("Npts", Npts);
  
  if (VERBOSE) gprint (GP_LOG, "%f %f %f %f %f %f %f %f\n", x, y, FWHMx, FWHMy, sky, I, mag, dmag);

  return (mag);

}

static double Raper  =  5;
static double Rinner = 10;
static double Router = 15;
static double *sky = NULL;

int set_rough_radii (double Ra, double Ri, double Ro) {

  Raper = Ra;
  Rinner = Ri;
  Router = Ro;
  if (sky == NULL) {
    ALLOCATE (sky, double, SQ(2*Router + 1));
  } else {
    REALLOCATE (sky, double, SQ(2*Router + 1));
  }
  return (TRUE);
}

/* use a circular aperture */
int get_rough_star (float *data, int Nx, int Ny, int x, int y,
		    opihi_flt *xc, opihi_flt *yc, 
		    opihi_flt *sx, opihi_flt *sy, opihi_flt *sxy,
		    opihi_flt *zs, opihi_flt *zp, opihi_flt *sk) {

  double Ro2, rad2;
  int i, j, Npts, Nsky;
  int Xs, Xe, Ys, Ye, off, Xc, Yc;
  double peak, fsky, value;
  double Sx, Sy, Sx2, Sy2, Sxy, Sum;
  
  /* define circular boundaries */
  Ro2 = SQ(Router);

  /* measure the sky level */
  /* boundaries for the outer sky region */
  Xs = MAX (x - Router, 0);
  Xe = MIN (x + Router + 1, Nx);
  Ys = MAX (y - Router, 0);
  Ye = MIN (y + Router + 1, Ny);

/* this sample uses a circular aperture */
# if (0)
  double Ri2 = SQ(Rinner);
  Nsky = 0;  
  for (j = Ys; j < Ye; j++) {
    off = j*Nx;
    for (i = Xs; i < Xe; i++) { 
      rad2 = SQ(i - x) + SQ(j - y);
      if (rad2 > Ro2) continue;
      if (rad2 < Ri2) continue;
      sky[Nsky] = data[i+off];
      Nsky ++;
    }
  }
  dsort (sky, Nsky);
  for (Npts = fsky = 0, i = 0.25*Nsky; i < 0.75*Nsky; i++, Npts += 1.0) {
    fsky += sky[i];
  }
  fsky = fsky / Npts;
# else

/* this sample uses a square outer annulus, without loop if tests */
  Nsky = 0;  
  Xs = MAX (x - Router, 0);
  Xe = MIN (x - Rinner + 1, Nx);
  Ys = MAX (y - Rinner, 0);
  Ye = MIN (y + Rinner + 1, Ny);
  for (j = Ys; j < Ye; j++) {
    off = j*Nx;
    for (i = Xs; i < Xe; i++) { 
      if (!isfinite(data[i+off])) continue;
      sky[Nsky] = data[i+off];
      Nsky ++;
    }
  }
  Xs = MAX (x + Rinner, 0);
  Xe = MIN (x + Router + 1, Nx);
  for (j = Ys; j < Ye; j++) {
    off = j*Nx;
    for (i = Xs; i < Xe; i++) { 
      if (!isfinite(data[i+off])) continue;
      sky[Nsky] = data[i+off];
      Nsky ++;
    }
  }
  Xs = MAX (x - Rinner, 0);
  Xe = MIN (x - Rinner + 1, Nx);
  Ys = MAX (y - Router, 0);
  Ye = MIN (y - Rinner + 1, Ny);
  for (j = Ys; j < Ye; j++) {
    off = j*Nx;
    for (i = Xs; i < Xe; i++) { 
      if (!isfinite(data[i+off])) continue;
      sky[Nsky] = data[i+off];
      Nsky ++;
    }
  }
  Ys = MAX (y + Rinner, 0);
  Ye = MIN (y + Router + 1, Ny);
  for (j = Ys; j < Ye; j++) {
    off = j*Nx;
    for (i = Xs; i < Xe; i++) { 
      if (!isfinite(data[i+off])) continue;
      sky[Nsky] = data[i+off];
      Nsky ++;
    }
  }
  dsort (sky, Nsky);
  for (Npts = fsky = 0, i = 0.25*Nsky; i < 0.75*Nsky; i++, Npts += 1.0) {
    fsky += sky[i];
  }
  fsky = fsky / Npts;
# endif

  /* boundaries for the star region */
  Xs = MAX (x - Raper, 0);
  Xe = MIN (x + Raper + 1, Nx);
  Ys = MAX (y - Raper, 0);
  Ye = MIN (y + Raper + 1, Ny);

  /** note that this will fail on negative flux objects */
  peak = Npts = 0;
  Sx = Sy = Sx2 = Sy2 = Sxy = Sum = 0;
  for (j = Ys; j < Ye; j++) {
    off = j*Nx;
    Yc = j - y;
    for (i = Xs; i < Xe; i++) {
      Xc = i - x;
      rad2 = SQ(Xc) + SQ(Yc);
      if (rad2 > Ro2) continue;
      // if (!isfinite(data[i+off])) continue;
      value = data[i+off] - fsky;
      Sx  += Xc*value;
      Sy  += Yc*value;
      Sx2 += Xc*Xc*value;
      Sy2 += Yc*Yc*value;
      Sxy += Xc*Yc*value;
      Sum += value;
      Npts ++;
      if (value > peak) peak = value;
    }
  }

  *xc = Sx / Sum;
  *yc = Sy / Sum;
  *sx = sqrt (fabs (Sx2 / Sum - SQ(*xc)));
  *sy = sqrt (fabs (Sy2 / Sum - SQ(*yc)));
  *sxy = Sxy / Sum;
  *xc += x;
  *yc += y;
  *zs = Sum;
  *zp = peak;
  *sk = fsky;
  /* note sigma is rough: round-off errors can introduce errors */
  /* using values relative to x,y should minimize this effect */

  return (Npts);
}

