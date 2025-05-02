# include "astro.h"

double Interpolate (const double x, const double y, float *in, int Nx, int Ny);

int galprofiles (int argc, char **argv) {
  
  int i, Nx, Ny, N, Nsec, NELEMENTS;
  float *in;
  double theta, dtheta, x, y, Xo, Yo, r, Rmax, value;
  Buffer *buf;
  Vector **fvec, **rvec, *tvec;
  char name[128];

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: galprofiles (buffer) (Xo) (Yo) Nsec\n");
    gprint (GP_ERR, "  generates Nsec profiles around the circle; the first is centered on 0.0 degrees\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  Rmax = 0.5*hypot(Nx, Ny);
  
  Xo = atof(argv[2]);
  Yo = atof(argv[3]);
  Nsec = atof(argv[4]);

  dtheta = 360.0 / Nsec;
  // sector i ranges from (i-0.5)*dtheta to (i+0.5)*dtheta
  // i = theta / dtheta + 0.5;

  ALLOCATE (fvec, Vector *, Nsec);
  ALLOCATE (rvec, Vector *, Nsec);
  if ((tvec = SelectVector ("theta", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  ResetVector (tvec, OPIHI_FLT, Nsec);

  // dereference pointer
  in = (float *) buf[0].matrix.buffer;

  for (i = 0; i < Nsec; i++) {

    // generate the output vector names & vectors
    sprintf (name, "flux_%d", i);
    if ((fvec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) return (FALSE);
    sprintf (name, "rad_%d", i);
    if ((rvec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) return (FALSE);

    // allocate initial data space
    N = 0;
    NELEMENTS = 1000;
    ResetVector (fvec[i], OPIHI_FLT, NELEMENTS);
    ResetVector (rvec[i], OPIHI_FLT, NELEMENTS);

    // angle for this profile
    tvec[0].elements.Flt[i] = i*dtheta;
    theta = RAD_DEG*i*dtheta;

    // start at Xo,Yo and find the x,y locations for r_i, theta where r_i increments by 1 pixel
    for (r = 0; r < Rmax; r++) {

      // XXX Xo,Yo are referenced to pixels with bounds i+0.0, i+1.0
      x = r * cos (theta) + Xo;
      y = r * sin (theta) + Yo;

      // value is NAN if we run off the image
      value = Interpolate(x, y, in, Nx, Ny);
      if (isnan(value)) break;

      rvec[i][0].elements.Flt[N] = r;
      fvec[i][0].elements.Flt[N] = value;
      N++;

      if (N >= NELEMENTS) {
	NELEMENTS += 100;
	REALLOCATE (rvec[N][0].elements.Flt, opihi_flt, NELEMENTS);
	REALLOCATE (fvec[N][0].elements.Flt, opihi_flt, NELEMENTS);
      }
    }
    fvec[i][0].Nelements = N;
    rvec[i][0].Nelements = N;
  }
  return (TRUE);
}

// fast & simple API to interpolate to a subpixel position using bilinear interpolation
// x,y in parent image coordinates (pixel centers at 0.5, 0.5)
// stolen from psLib
double Interpolate (const double x, const double y, float *in, int Nx, int Ny) {

  // allow extrapolation a small distance beyond the edge of valid pixels, but no
  // further (this allows the nXskip,nYskip boundary areas to be used as well)
  float nXedge = 0.125*Nx;
  float nYedge = 0.125*Ny;

  if ((x < -nXedge) || (x > Nx + nXedge) || (y < -nYedge) || (y > Ny + nYedge)) {
    return NAN;
  }

  // limiting cases: Nx == 1 and/or Ny == 1

  // if we have a single pixel, there is no spatial information
  if ((Nx == 1) && (Ny == 1)) {
    return in[0];
  }

  // handle edge cases with extrapolation
  const int ix = x - 0.5; // index of reference pixel
  const int iy = y - 0.5; // index of reference pixel

  // do numCols,Rows first so we are never < 0
  const int Xs = MAX (MIN (ix, Nx - 2), 0);
  const int Ys = MAX (MIN (iy, Ny - 2), 0);

  const int Xe = Xs + 1;
  const int Ye = Ys + 1;

  // dx,dy range from 0.0 to 1.0 for interpolated pixels, and -0.5 to 1.5 for extrapolation
  const double dx = x - 0.5 - Xs;
  const double dy = y - 0.5 - Ys;

  const double rx = 1.0 - dx;
  const double ry = 1.0 - dy;

  // if Nx == 1, we have no x-dir spatial information
  if (Nx == 1) {
    double V0 = in[Ys*Nx + Xs];
    double V1 = in[Ye*Nx + Xs];

    const double value = V0*ry + V1*dy;
    return value;
  }

  // if Ny == 1, we have no y-dir spatial information
  if (Ny == 1) {
    double V0 = in[Ys*Nx + Xs];
    double V1 = in[Ys*Nx + Xe];

    const double value = V0*rx + V1*dx;
    return value;
  }

  // Vxy
  double V00 = in[Ys*Nx + Xs];
  double V01 = in[Ye*Nx + Xs];
  double V10 = in[Ys*Nx + Xe];
  double V11 = in[Ye*Nx + Xe];

  double value;

  // corners
  if ((dx < 0.0) && (dy < 0.0)) {
    return V00;
  }
  if ((dx > 1.0) && (dy < 0.0)) {
    return V10;
  }
  if ((dx < 0.0) && (dy > 1.0)) {
    return V01;
  }
  if ((dx > 1.0) && (dy > 1.0)) {
    return V11;
  }

  // sides
  if (dx < 0.0) {
    value = V00*ry + V01*dy;
    return value;
  }
  if (dy < 0.0) {
    value = V00*rx + V10*dx;
    return value;
  }
  if (dx > 1.0) {
    value = V10*ry + V11*dy;
    return value;
  }
  if (dy > 1.0) {
    value = V01*rx + V11*dx;
    return value;
  }

  // bilinear interpolation
  value = V00*rx*ry + V10*dx*ry + V01*rx*dy + V11*dx*dy;
  return value;
}
