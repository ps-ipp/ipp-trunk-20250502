# include "data.h"

// take an image and accumulate a histogram in some direction
int xsection (int argc, char **argv) {
  
  int i, j, N, Nbins;
  int sx, sy, nx, ny, bin;
  int *vecN;
  float *V, delta;
  Vector *vec1, *vec2;
  Buffer *buf;

 // int Quiet = FALSE;
 // if ((N = get_argument (argc, argv, "-q"))) {
 //   Quiet = TRUE;
 //   remove_argument (N, &argc, argv);
 // }
 // if ((N = get_argument (argc, argv, "-quiet"))) {
 //   Quiet = TRUE;
 //   remove_argument (N, &argc, argv);
 // }

  delta = 1.0;
  if ((N = get_argument (argc, argv, "-delta"))) {
    remove_argument (N, &argc, argv);
    delta = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  sx = sy = 0.0;
  nx = ny = 0.0;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    sx = atof (argv[N]);
    remove_argument (N, &argc, argv);
    sy = atof (argv[N]);
    remove_argument (N, &argc, argv);
    nx = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ny = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: xsection <buffer> <x> <y> (angle) [-region sx sy nx ny] [-delta binsize]\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  
  int Nx = buf[0].matrix.Naxis[0];
  int Ny = buf[0].matrix.Naxis[1];

  /* if either range is set to zero, use the rest of the chip */
  if (nx == 0) nx = Nx - sx;
  if (ny == 0) ny = Ny - sy;

  if ((sx < 0) || (sy < 0) || (sx+nx > Nx) || (sy+ny > Ny)) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }


  // center coords are center of the region
  float Xo = sx + 0.5*nx;
  float Yo = sy + 0.5*ny;

  float angle = atof(argv[4]);
  float secant = MAX(2.0*fabs(cos(angle*RAD_DEG)/Nx), 2.0*fabs(sin(angle*RAD_DEG)/Ny));
  float range = 1.1 / secant;

  Nbins = 2*range / delta;

  if ((vec1 = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vec2 = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (vec1, OPIHI_FLT, Nbins + 1);
  ResetVector (vec2, OPIHI_FLT, Nbins + 1);
  bzero (vec1[0].elements.Flt, vec1[0].Nelements*sizeof(opihi_flt));
  bzero (vec2[0].elements.Flt, vec2[0].Nelements*sizeof(opihi_flt));
  
  V = (float *)buf[0].matrix.buffer;

  float dx = cos(angle*RAD_DEG);
  float dy = sin(angle*RAD_DEG);

  ALLOCATE (vecN, int, Nbins + 1);
  memset (vecN, 0, (Nbins + 1)*sizeof(int));

  for (j = sy; j < sy + ny; j++) {
    float dY = (j - Yo);
    for (i = sx; i < sx + nx; i++) {
      float value = V[i*Nx + j];
      if (!isfinite(value)) continue;
      float dX = (i - Xo);
      float L = (dX * dx) + (dY * dy);
      bin = MAX (MIN (Nbins, (L / delta) + 0.5*Nbins), 0);
      vec2[0].elements.Flt[bin] += value;
      vecN[bin] ++;
    }
  }
  for (i = 0; i < Nbins + 1; i++) {
    vec1[0].elements.Flt[i] = (i - 0.5*Nbins)*delta;
    vec2[0].elements.Flt[i] /= (float) vecN[i];
  }
  
  return (TRUE);
}

