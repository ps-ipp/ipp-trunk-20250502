# include "data.h"

double interpolateValue (int *hist, int Nbins, float min, float binsize, int bin, float value);
double sampleMedian (Matrix *matrix, int Nsubset);
Stats *robustMedian (Matrix *matrix, Region *region, float minValue, float maxValue, float sigma);

typedef struct {
  int sx;
  int sy;
  int nx;
  int ny;
} Region;

typedef struct {
  float median;
  float mean;
  float mode;
  float sigma;
} Stats;

int stats (int argc, char **argv) {
  
  int i, j, Nmode, Imode;
  double Npix, N1, N2, max, min, range, median, mode, IgnoreValue;
  float *V;
  int sx, sy, nx, ny, *hist, Nhist, bin;
  int Ignore, Quiet, N;
  Buffer *buf;

  IgnoreValue = 0;
  Ignore = FALSE;
  if ((N = get_argument (argc, argv, "-ignore"))) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((argc != 2) && (argc != 6)) {
    gprint (GP_ERR, "USAGE: stats <buffer> sx sy nx ny\n");
    gprint (GP_ERR, "OR:    stats <buffer>\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if (argc == 6) {
    sx = strcmp (argv[2], "-") ? atof (argv[2]) : 0;
    sy = strcmp (argv[3], "-") ? atof (argv[3]) : 0;
    nx = strcmp (argv[4], "-") ? atof (argv[4]) : buf[0].matrix.Naxis[0];
    ny = strcmp (argv[5], "-") ? atof (argv[5]) : buf[0].matrix.Naxis[1];
  } else {
    sx = 0;
    sy = 0;
    nx = buf[0].matrix.Naxis[0];
    ny = buf[0].matrix.Naxis[1];
  }

  Npix = N1 = N2 = 0;
  if ((sx < 0) || (sy < 0) || 
      (sx+nx > buf[0].matrix.Naxis[0]) || 
      (sy+ny > buf[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }

  max = min = *((float *)(buf[0].matrix.buffer) + sy*buf[0].matrix.Naxis[0] + sx);
  for (j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (i = 0; i < nx; i++, V++) {
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      N1 += *V;
      N2 += (*V)*(*V);
      Npix += 1.0;
      max = MAX (max, *V);
      min = MIN (min, *V);
    }
  }
  N1 = N1 / Npix;

  sigma = sqrt (N2/Npix - N1*N1);

  if (ROBUST) {
    stats = robustMedian (matrix, min, max, sigma);
    if (stats->sigma 
  } 
  
  if (!Quiet) {
    gprint (GP_LOG, "  mean    stdev    min     max   median   Npix   Total\n");
    gprint (GP_LOG, "%7.4g %7.4g %7.4g %7.4g %7.4g %7.0f %7.4g\n", N1, sqrt (N2/Npix - N1*N1), 
	    min, max, median, Npix, Npix*N1);
  }

  set_variable ("MIN",    min);
  set_variable ("MAX",    max);
  set_variable ("MEDIAN", median);
  set_variable ("MEAN",   N1);
  set_variable ("MODE",   mode);
  set_variable ("TOTAL",  N1*Npix);
  set_variable ("NPIX",   Npix);
  set_variable ("SIGMA",  sqrt (N2/Npix - N1*N1));

  return (TRUE);
}

double sampleMedian (Matrix *matrix, int Nsubset) {

  int i, Nsample, Npix;
  long A, B;
  float *values, *buffer, median;

  /* Generate a vector of Nsample elements */
  Npix = matrix[0].Naxis[0]*matrix[0].Naxis[1]
    Nsample = Nsubset;
  if (Nsubset == 0) {
    Nsample = Npix;
  }

  ALLOCATE (values, float, Nsample);
    
  // srand48() is called by startup.c
 
  *buffer = (float *) matrix[0].buffer;

  for (i = 0; i < Nsample; i++) {
    if (Nsubset) {
      j = MIN(Npix - 1, MAX(0, Npix*drand48()));
    } else {
      j = i;
    }
    values[i] = buffer[j];
  }
    
  fsort (values, Nsample);

  if (Nsample == 0) return 0.0;

  // these are all covered by the logic below
  // if (Nsample == 1) return values[0];
  // if (Nsample == 2) return (0.5*(values[0] + values[1]));
  // if (Nsample == 3) return (values[1]);

  if (Nsample % 2) {
    median = 0.5*(values[(int)(0.5*Nsample)] + values[(int)(0.5*Nsample) - 1]);
  } else {
    median = values[(int)(0.5*Nsample)];
  }
  return (median);
}

Stats *robustMedian (Matrix *matrix, Region *region, float minValue, float maxValue, float sigma) {

  Stats *stats;

  ALLOCATE (stats, Stats, 1);

  // no data range:
  if (maxValue - minValue < FLT_EPSILON) {
    stats->median = minValue;
    stats->mean   = minValue;
    stats->mode   = minValue;
    stats->sigma  = 0.0;
    return stats;
  }

  // generate a histogram ranging from min to max with step size of sigma / 4
  // no more than 0x1000 bins, no less than 0x10

  Nbins = 4.0 * (maxValue - minValue) / sigma;
  Nbins = MIN (0x1000, MAX (0x10, Nbins));

  ALLOCATE (hist, int, Nbins);
  memset (hist, 0, Nbins*sizeof(int));
  
  delta = Nbins / (max - min);
  binsize = 1.0 / delta;

  Ntotal = 0;
  for (j = region->sy; j < region->sy + region->ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + region->sx; 
    for (i = 0; i < region->nx; i++, V++) {
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      bin = MIN (MAX (0, (*V - min) * delta), Nbins - 1);
      hist[bin] ++;
      Ntotal ++;
    }
  }

  // generate the cumulative histogram & find mode
  ALLOCATE (cumu, int, Nbins);
  memset (cumu, 0, Nbins*sizeof(int));

  Nhist = 0;
  Imode = -1;
  Vmode = 0;
  for (i = 0; i < Nbins; i++) {
    Nhist += hist[i];
    cumu[i] = Nhist;
    if (hist[i] > Vmode) {
      Imode = i;
      Vmode = hist[i];
    }
  }

  Ioo = ibracket (cumu, Nbins, 0.500000*Nhist, 1);
  Im1 = ibracket (cumu, Nbins, 0.308538*Nhist, 1);
  Ip1 = ibracket (cumu, Nbins, 0.691462*Nhist, 1);
  Im2 = ibracket (cumu, Nbins, 0.022481*Nhist, 1);
  Ip2 = ibracket (cumu, Nbins, 0.977519*Nhist, 1);

  Voo = interpolateValue (cumu, Nbins, min, delta, Ioo, 0.500000*Nhist);
  Vm1 = interpolateValue (cumu, Nbins, min, delta, Im1, 0.308538*Nhist);
  Vp1 = interpolateValue (cumu, Nbins, min, delta, Ip1, 0.691462*Nhist);
  Vm2 = interpolateValue (cumu, Nbins, min, delta, Im2, 0.022481*Nhist);
  Vp2 = interpolateValue (cumu, Nbins, min, delta, Ip2, 0.977519*Nhist);

  sigma1 = (Vp1 - Vm1);
  sigma2 = (Vp2 - Vm2) / 4.0;

  Vs = Vn = 0;
  for (i = Im1; i < Ip1 + 1; i++) {
    Vn += hist[i];
    Vs += hist[i] * (i * binsize + min);
  }

  stats->median = Voo;
  stats->mean = Vs / Vn;
  stats->mode = Imode * delta + min;
  stats->sigma = MIN (sigma1, sigma2);

  free (hist);
  free (cumu);

  return (stats);
}

// what is the fractional bin position for the given value
double interpolateValue (int *hist, int Nbins, float min, float binsize, int bin, float value) {

  V0 = bin * binsize + min;
  V1 = (bin + 1) * binsize + min;

  // fbin = (value - V0) / (V1 - V0) + bin;
  fbin = (value - V0) / binsize + bin;

  return fbin;
}
