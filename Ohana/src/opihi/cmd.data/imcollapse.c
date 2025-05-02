# include "data.h"

enum {CALC_MEDIAN, CALC_MEAN, CALC_IRLS, CALC_WTMEAN, CALC_INNER_FRACTION};

// take an image and collapse to a histogram along x or y 
int imcollapse (int argc, char **argv) {
  
  int N;
  Buffer *buf;
  Vector *vecX, *vecY;

  int mode = CALC_MEDIAN;
  if ((N = get_argument (argc, argv, "-mean"))) {
    mode = CALC_MEAN;
    remove_argument (N, &argc, argv);
  }

  int sx = 0, sy = 0;
  int nx = 0, ny = 0;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    sx = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    sy = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    nx = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    ny = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: imcollapse <buffer> <x> <y> <X|Y> [-region sx sy nx ny]\n");
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
  if ((nx < 0) || (ny < 0)) {
    gprint (GP_ERR, "invalid region\n");
    return (FALSE);
  }

  if ((vecX = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecY = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  int direction = 0;
  if (!strcasecmp(argv[4], "x")) direction = 1;
  if (!strcasecmp(argv[4], "y")) direction = 2;
  if (!direction) { 
    gprint (GP_ERR, "invalid direction %s\n", argv[4]);
    return (FALSE);
  }

  // 'direction' is the direction of collapse 
  int Nbins = (direction == 1) ? ny : nx;
  ResetVector (vecX, OPIHI_FLT, Nbins);
  ResetVector (vecY, OPIHI_FLT, Nbins);
  bzero (vecX->elements.Flt, vecX->Nelements*sizeof(opihi_flt));
  bzero (vecY->elements.Flt, vecY->Nelements*sizeof(opihi_flt));
  opihi_flt *xout = vecX->elements.Flt;
  opihi_flt *yout = vecY->elements.Flt;
  
  float *V = (float *)buf[0].matrix.buffer;

  // temp storage for extracted values
  int Nvals = (direction == 1) ? nx : ny;
  ALLOCATE_PTR (values, opihi_flt, Nvals);

  if (direction == 1) {
    for (int iy = sy; iy < sy + ny; iy++) {
      xout[iy - sy] = iy;
      // accumulate the values for this output bin
      int n = 0;
      for (int ix = sx; ix < sx + nx; ix++) {
	double val = V[iy*Nx + ix];
	if (!isfinite(val)) continue;
	values[n] = val;
	n++;
      }
      if (n == 0) {
	yout[iy - sy] = NAN;
	continue;
      }
      // calculate the statistic (mean or median)
      if (mode == CALC_MEDIAN) {
	dsort (values, n);
	int midpt = 0.5*n;
	yout[iy - sy] = (n % 2) ? values[midpt] : 0.5*(values[midpt] + values[midpt-1]);
      }
      if (mode == CALC_MEAN) {
	opihi_flt sum = 0.0;
	for (int i = 0; i < n; i++) {	
	  sum += values[i];
	}
	yout[iy - sy] = sum / (float) n;
      }
    }
  }  
  return (TRUE);
}

