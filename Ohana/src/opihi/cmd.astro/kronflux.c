# include "astro.h"
int kronflux_stats (Matrix *matrix, int X, int Y, int radius, int maxradius, int border, float alpha);

int VERBOSE = TRUE;

// manual calculation of the Kron parameters  
int kronflux (int argc, char **argv) {

  int x, y, N, radius, border, maxradius;
  float alpha;
  Buffer *buf;

  radius = 5;
  if ((N = get_argument (argc, argv, "-radius"))) {
    remove_argument (N, &argc, argv);
    radius  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  maxradius = 100;
  if ((N = get_argument (argc, argv, "-maxradius"))) {
    remove_argument (N, &argc, argv);
    maxradius  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  border = 5;
  if ((N = get_argument (argc, argv, "-border"))) {
    remove_argument (N, &argc, argv);
    border  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  alpha = 2.5;
  if ((N = get_argument (argc, argv, "-alpha"))) {
    remove_argument (N, &argc, argv);
    alpha  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = FALSE;
  }
  
  if (argc != 4) {
    gprint (GP_ERR, "USAGE: kronflux (buffer) x y [-radius N] [-border N] [-alpha F] [-q]\n");
    gprint (GP_ERR, " -radius is the radius of the initial window (default 5)\n");
    gprint (GP_ERR, " -border is the size of the sky annulus (default 5)\n");
    gprint (GP_ERR, " -alpha  is the scale factor for kron radius vs 1st radial moment (default 2.5)\n");
    return (FALSE);
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  x = atof (argv[2]);
  y = atof (argv[3]);

  kronflux_stats (&buf[0].matrix, x, y, radius, maxradius, border, alpha);

  return TRUE;
}

// I'd like to be able to skip the sky measurement
// if Nborder > 0, calculate local sky as the median of the pixels in the region between radius and border
int kronflux_stats (Matrix *matrix, int X, int Y, int radius, int maxradius, int border, float alpha) {

  int i, j;

  // double gain;
  // char *string = get_variable ("GAIN");
  // if (string == (char *) NULL) {
  //   if (VERBOSE) gprint (GP_ERR, "assuming a value of 1.0\n");
  //   gain = 1.0;
  // } else {
  //   gain = atof (string);
  // }

  float *data = (float *) matrix->buffer;
  int Nx = matrix->Naxis[0];
  int Ny = matrix->Naxis[1];
  
  double sky = 0.0;
  double dsky2 = 0.0;
  if (border > 0) {
    int radius2 = radius + border;

    // allocate enough space for all pixels in the 2*radius2 + 1 square
    int NRING = SQ(2*radius2 + 1);
    double *ring = NULL;
    ALLOCATE (ring, double, NRING);
    bzero (ring, sizeof(double)*NRING);

    int Nring = 0;  
    for (j = -radius2; j <= radius2; j++) {
      int iy = j + Y;
      if (iy < 0) continue;
      if (iy >= Ny) continue;
      for (i = -radius2; i <= radius2; i++) {
	if (hypot(i, j) <= radius) continue;
	int ix = i + X;
	if (ix < 0) continue;
	if (ix >= Nx) continue;
	ring[Nring] = data[ix + iy*Nx];
	Nring ++;
      }
    }
    dsort (ring, Nring);

    int Npts = 0;
    for (i = 0.25*Nring; i < 0.75*Nring; i++, Npts += 1.0) {
      sky += ring[i];
      dsky2 += ring[i]*ring[i];
    }
    sky = sky / Npts;
    dsky2 = dsky2 / Npts - sky*sky;
    free (ring);
  }

  // find the 1st radial moment
  float flux1 = 0.0;
  float Srf = 0.0;
  for (j = -radius; j <= radius; j++) {
    int iy = j + Y;
    if (iy < 0) continue;
    if (iy >= Ny) continue;
    for (i = -radius; i <= radius; i++) {
      float r = hypot(i, j);
      if (r > radius) continue;
      int ix = i + X;
      if (ix < 0) continue;
      if (ix >= Nx) continue;
      float value = data[ix + iy*Nx] - sky;
	
      flux1 += value;
      Srf += value * r;
    }
  }

  float r1 = Srf / flux1;
  float rK = r1 * alpha;

  rK = MIN(MAX(rK, 5.0), maxradius);

  // measure the flux within A*r1
  float fluxK = 0.0;
  for (j = -rK; j <= rK; j++) {
    int iy = j + Y;
    if (iy < 0) continue;
    if (iy >= Ny) continue;
    for (i = -rK; i <= rK; i++) {
      float r = hypot(i, j);
      if (r > rK) continue;
      int ix = i + X;
      if (ix < 0) continue;
      if (ix >= Nx) continue;
      float value = data[ix + iy*Nx] - sky;
	
      fluxK += value;
    }
  }

  float magK = -2.5*log10(fluxK);
  set_variable ("KronFlux", fluxK);
  set_variable ("KronRadius", rK);
  set_variable ("KronMag", magK);
  set_variable ("KronF1", flux1);
  set_variable ("KronR1", r1);
  
  if (VERBOSE) gprint (GP_LOG, "%f %f %f %f %f\n", magK, fluxK, rK, flux1, r1);

  return TRUE;
}

