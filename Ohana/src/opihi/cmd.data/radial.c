# include "data.h"

int circstats (int argc, char **argv) {
  
  int i, j;
  double Npix, S1, S2, max, min, Sum, Mean, Stdev, IgnoreValue; 
  double xc, yc, radius, R2, r;
  float *V;
  int xs, ys, xe, ye;
  int Ignore, Quiet, N, Nx, Ny;
  Buffer *buf;

  Ignore = FALSE;
  IgnoreValue = 0;
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

  if (argc != 5) goto usage;
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto missed;

  xc = atof (argv[2]);
  yc = atof (argv[3]);
  radius = atof (argv[4]);
  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];

  if (xc < 0) goto range;
  if (yc < 0) goto range;
  if (xc >= Nx) goto range;
  if (yc >= Ny) goto range;

  xs = MAX (0, xc - radius);
  ys = MAX (0, yc - radius);
  xe = MIN (Nx, xc + radius + 1);
  ye = MAX (Ny, yc + radius + 1);
  R2 = radius*radius;

  S1 = S2 = Npix = 0;
  min = max = *(float *)(buf[0].matrix.buffer) + (int)(yc)*buf[0].matrix.Naxis[0] + (int)(xc); 
  for (j = ys; j < ye; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + xs; 
    for (i = xs; i < xe; i++, V++) {
      r = SQ(i - xc) + SQ(j - yc);
      if (r > R2) continue;
      if (Ignore && (fabs (*V - IgnoreValue) < 1e-8)) continue;
      S1 += *V;
      S2 += (*V)*(*V);
      Npix += 1.0;
      max = MAX (max, *V);
      min = MIN (min, *V);
    }
  }
  Mean = S1 / Npix;
  Sum  = Mean * M_PI * R2;
  Stdev = sqrt (S2/Npix - Mean*Mean);

  if (!Quiet) {
    gprint (GP_LOG, "     mean     stdev       min       max    Npix     Total\n");
    gprint (GP_LOG, "%9.4g %9.4g %9.4g %9.4g %7.0f %9.4g\n", Mean, Stdev, min, max, Npix, Sum);
  }

  set_variable ("MIN",    min);
  set_variable ("MAX",    max);
  set_variable ("MEAN",   Mean);
  set_variable ("SUM",    Sum);
  set_variable ("NPIX",   Npix);
  set_variable ("SIGMA",  Stdev);

  return (TRUE);

 usage: 
  gprint (GP_ERR, "USAGE: circstats <buffer> x y radius\n");
  return (FALSE);

 range:
  gprint (GP_ERR, "ERROR: coordinates out of range\n");
  return (FALSE);

 missed:
  gprint (GP_ERR, "ERROR: buffer not found\n");
  return (FALSE);
}

