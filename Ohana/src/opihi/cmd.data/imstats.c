# include "data.h"

int imstats (int argc, char **argv) {
  
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

  max = -1e32;
  min = +1e32;
  for (j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (i = 0; i < nx; i++, V++) {
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      if (isnan(*V)) continue;
      if (isinf(*V)) continue;
      N1 += *V;
      N2 += (*V)*(*V);
      Npix += 1.0;
      max = MAX (max, *V);
      min = MIN (min, *V);
    }
  }
  N1 = N1 / Npix;

/* calculate mode, median */
  median = mode = 0.5*(max + min);
  if ((max - min) != 0) {
    range = 0xffff / (max - min);
    ALLOCATE (hist, int, 0x10000);
    bzero (hist, 0x10000*sizeof(int));
    for (j = sy; j < sy + ny; j++) {
      V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
      for (i = 0; i < nx; i++, V++) {
	if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
	if (isnan(*V)) continue;
	if (isinf(*V)) continue;
	bin = MIN (MAX (0, (*V - min) * range), 0xffff);
	hist[bin] ++;
      }
    }
    Nhist = 0;
    for (i = 0; (i < 0xffff) && (Nhist < 0.5*Npix); i++) 
      Nhist += hist[i];
    median = i / range + min;
    Nmode = hist[0];
    Imode = 0;
    for (i = 1; i < 0x10000; i++) {
      if (hist[i] > Nmode) {
	Nmode = hist[i];
	Imode = i;
      }
    }
    mode = Imode / range + min;
    free (hist);
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

