# include "data.h"
double get_threshold (double *vector, int Nvector, double value);

int imstats (int argc, char **argv) {
  
  int N;
  float *V;
  int sx, sy, nx, ny;

  double IgnoreValue = 0;
  int Ignore = FALSE;
  if ((N = get_argument (argc, argv, "-ignore"))) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int Quiet = FALSE;
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

  Buffer *buf;
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

  if ((sx < 0) || (sy < 0) || 
      (sx+nx > buf[0].matrix.Naxis[0]) || 
      (sy+ny > buf[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }

  // INITTIME;

  // first get completely unclipped values min, max, total, Npix
  double max = -1e32;
  double min = +1e32;
  double total = 0.0;
  int Npix = 0;
  for (int j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (int i = 0; i < nx; i++, V++) {
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      if (isnan(*V)) continue;
      if (isinf(*V)) continue;
      Npix += 1.0;
      max = MAX (max, *V);
      min = MIN (min, *V);
      total += *V;
    }
  }

  // MARKTIME ("get min,max : %f,%f : %f sec\n", min, max, dtime);

  double median = 0.5*(max + min);
  double mean = median;
  double sigma = NAN;
  double sigma_clip = NAN;
  if (max == min) goto escape;
  
  // pass 1: find the inner quartiles:
  // bin[i] : (min + delta*i) to (min + delta*(i + 1.0)), center is min + delta*(i + 0.5)
  int Nbin_v0 = 1000;
  double range_v0 = (float) Nbin_v0 / (max - min);
  int *hist = NULL;
  ALLOCATE_ZERO (hist, int, Nbin_v0 + 2); // 2 extra bins at the max
  for (int j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (int i = 0; i < nx; i++, V++) {
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      if (isnan(*V)) continue;
      if (isinf(*V)) continue;
      int bin = MIN (MAX (0, (*V - min) * range_v0), Nbin_v0);
      hist[bin] ++;
    }
  }

  // MARKTIME ("pass 1 histogram : %f sec\n", dtime);

  // generate a cumulative histogram and renormalize
  int sumHist = 0.0;
  double *cumu = NULL;
  ALLOCATE_ZERO (cumu, double, Nbin_v0 + 2); // 2 extra bins at the max
  for (int i = 0; i < Nbin_v0 + 2; i++) {
    sumHist += hist[i];
    cumu[i] = sumHist;
  }
  for (int i = 0; i < Nbin_v0 + 2; i++) {
    cumu[i] = cumu[i] / (float) sumHist;
  }

  // MARKTIME ("cumulative histogram : %f sec\n", dtime);

  // find the 15.866% and 84.135% bins (get_threshold interpolates relative to the bin center)
  double SloBin = get_threshold (cumu, Nbin_v0 + 2, 0.15866);
  double ShiBin = get_threshold (cumu, Nbin_v0 + 2, 0.84135);

  free (hist);
  free (cumu);

  if (isnan(SloBin) || isnan(ShiBin)) goto escape;

  double sigma_v0  = 0.5*(ShiBin - SloBin) / range_v0; // min cancels between Shi and Slo
  double median_v0 = 0.5*(ShiBin + SloBin) / range_v0 + min;

  // MARKTIME ("first pass median & sigma: %f +/- %f : %f sec\n", median_v0, sigma_v0, dtime);

  // re-measure with only +/- 5 sigma range

  // pass 2: limit to range median +/- 5 sigma
  // bin[i] : (min + delta*i) to (min + delta*(i + 1.0)), center is min + delta*(i + 0.5)
  int Nbin_v1 = 1000;
  double range_v1 = (float) Nbin_v1 / (10.0*sigma_v0);
  double min_s5 = median_v0 - 5.0*sigma_v0;
  double max_s5 = median_v0 + 5.0*sigma_v0;
  double S1 = 0.0;
  double S2 = 0.0;
  int Nclip = 0;
  ALLOCATE_ZERO (hist, int, Nbin_v1 + 2); // 2 extra bins at the max
  for (int j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (int i = 0; i < nx; i++, V++) {
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      if (isnan(*V)) continue;
      if (isinf(*V)) continue;
      if (*V < min_s5) continue;
      if (*V > max_s5) continue;
      int bin = MIN (MAX (0, (*V - min_s5) * range_v1), Nbin_v1);
      hist[bin] ++;
      S1 += *V;
      S2 += (*V) * (*V);
      Nclip ++;
    }
  }
  mean = S1 / (double) Nclip;
  sigma_clip = sqrt (S2 / (double) Nclip - mean*mean);

  // MARKTIME ("pass 2 histogram : %f sec\n", dtime);

  // generate a cumulative histogram and renormalize
  sumHist = 0.0;
  ALLOCATE_ZERO (cumu, double, Nbin_v1 + 2); // 2 extra bins at the max
  for (int i = 0; i < Nbin_v1 + 2; i++) {
    sumHist += hist[i];
    cumu[i] = sumHist;
  }
  for (int i = 0; i < Nbin_v1 + 2; i++) {
    cumu[i] = cumu[i] / (float) sumHist;
  }

  // MARKTIME ("cumulative histogram : %f sec\n", dtime);

  // find the 15.866% and 84.135% bins
  SloBin  = get_threshold (cumu, Nbin_v1 + 2, 0.15866);
  ShiBin  = get_threshold (cumu, Nbin_v1 + 2, 0.84135);
  double SmedBin = get_threshold (cumu, Nbin_v1 + 2, 0.50000);

  free (hist);
  free (cumu);

  if (isnan(SloBin) || isnan(ShiBin) || isnan(SmedBin)) goto escape;

  sigma  = 0.5*(ShiBin - SloBin) / range_v1;
  median = SmedBin / range_v1 + min_s5;

  // MARKTIME ("pass 2 median & sigma: %f +/- %f : %f sec\n", median, sigma, dtime);

escape:

  if (!Quiet) {
    gprint (GP_LOG, "  mean    stdev    min     max   median   Npix   Total\n");
    gprint (GP_LOG, "%7.4g %7.4g %7.4g %7.4g %7.4g %7d %7.4g\n", mean, sigma, min, max, median, Npix, total);
  }

  set_variable ("MIN",    min);
  set_variable ("MAX",    max);
  set_variable ("MEDIAN", median);
  set_variable ("MEAN",   mean);
  set_variable ("TOTAL",  total);
  set_variable ("NPIX",   Npix);
  set_variable ("SIGMA",  sigma);
  set_variable ("NCLIP",  Nclip);
  set_variable ("SIGMA_CLIP",  sigma_clip);

  return (TRUE);
}

double get_threshold (double *vector, int Nvector, double value) {

  int N;

  // find the last entry before threshold
  int Nlo = 0;
  int Nhi = Nvector - 1;

  if (value < vector[Nlo]) return NAN;
  if (value > vector[Nhi]) return NAN;

  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (vector[N] < value) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N, Nvector - 1);
    }
  }

  // v[Nlo] < value <= v[Nhi]
  for (N = Nlo; N <= Nhi; N++) {
    if (vector[N] > value) {
      Nhi = N;
      break;
    }
  }
  // v[Nhi] is the transition bin
  
  double x0, x1, y0, y1, Xvalue;

  // interpolate to value:
  y0 = vector[Nhi-1];
  y1 = vector[Nhi];
  x0 = Nhi - 1 + 0.5;
  x1 = Nhi + 0.5;
  if (y0 == y1) {
    Xvalue = 0.5*(x0 + x1);
  } else {
    Xvalue = (value - y0) * (x1 - x0) / (y1 - y0) + x0;
  }
  return Xvalue;
}
