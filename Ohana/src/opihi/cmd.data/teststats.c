# include "data.h"

int teststats (int argc, char **argv) {
  
  int i, N;
  double max, min, sum, var, dvar, mean, stdev;
  float *X, IgnoreValue;
  int Ignore, Quiet;

  int *Nval, bin, Nmode, Nmed;
  double dx, mode, median;
  Vector *vec;

  Ignore = FALSE;
  if (N = get_argument (argc, argv, "-ignore")) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  Quiet = FALSE;
  if (N = get_argument (argc, argv, "-q")) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if (N = get_argument (argc, argv, "-quiet")) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: teststats (vector)\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  Mean = Stdev = 0; 
  /* first pass: measure the sample statistics */
  for (j = 0; j < 3; j++) {
    /* calculate mean, npix */
    X = vec[0].elements;
    max = min = *X;
    sum = N = 0;
    for (i = 0; i < vec[0].Nelements; i++, X++) {
      if (!finite (*X)) continue;
      if (Ignore && (*X == IgnoreValue)) continue;
      if ((j > 0) && (fabs (*X - Mean) > 3*stdev)) continue;
      sum += *X;
      N++;
    }      
    mean = sum / N;
    /* calculate stdev */
    X = vec[0].elements;
    var = 0;
    for (i = 0; i < vec[0].Nelements; i++, X++) {
      if (!finite (*X)) continue;
      if (Ignore && (*X == IgnoreValue)) continue;
      if ((j > 0) && (fabs (*X - mean) > 3*stdev)) continue;
      dvar = (*X - mean);
      var += dvar*dvar;
    }      
    stdev = sqrt (var / N);
    Mean = mean;
  }
  mean = Mean;
  set_variable ("MEAN_C",     mean);
  set_variable ("SIGMA_C",    stdev);

  /* construct histogram with resolution of 0.1*stdev and range -1000*dx : 1000*dx centered on mean */
  dx = 0.1*stdev;
  min = mean - 1000*dx;
  max = mean + 1000*dx;
  NVAL = 1 + (int)((max - min) / dx);
  ALLOCATE (Nval, int, NVAL);
  bzero (Nval, NVAL*sizeof(int));
  X = vec[0].elements;
  for (i = 0; i < vec[0].Nelements; i++, X++) {
    if (!finite (*X)) continue;
    if (Ignore && (*X == IgnoreValue)) continue;
    bin = MAX (0, MIN (NVAL, (*X - min) / dx));
    Nval[bin] ++;
  }      

  /* find mode */
  Nmode = 0;
  mode = Nval[Nmode];
  for (i = 0; i < NVAL; i++) {
    if (mode < Nval[i]) {
      Nmode = i;
      mode = Nval[Nmode];
    }
  }
  mode = Nmode * dx + min;

  

  if (!Quiet) {
    gprint (GP_ERR, "mean: %f, stdev: %f, min: %f, max: %f, median: %f, mode: %f, Npts: %d\n", 
	     mean, stdev, min, max, median, mode, N);
  }

  set_variable ("MIN",      min);
  set_variable ("MAX",      max);
  set_variable ("MEDIAN",   median);
  set_variable ("MODE",     mode);
  set_variable ("TOTAL",    sum);
  set_variable ("MEAN",     mean);
  set_variable ("SIGMA",    stdev);

  set_int_variable ("NPIX", N);

  free (Nval);
  return (TRUE);
}

