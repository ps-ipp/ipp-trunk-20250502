# include "data.h"

int vstats (int argc, char **argv) {
  
  int i, iter, N, Nbin, Niter;
  double max, min, pmin, pmax, sum, var, dvar, mean, stdev;
  float IgnoreValue, Nsigma;
  int Ignore, Quiet;

  int *Nval, bin, Nmode, Nmed, Nused;
  double dx, mode, median, threshold;
  Vector *vec;
  char *mask = NULL;

  IgnoreValue = 0;
  Ignore = FALSE;
  if ((N = get_argument (argc, argv, "-ignore"))) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  Niter = 1;
  Nsigma = 3.0;

  if ((N = get_argument (argc, argv, "-sigma-clip"))) {
    remove_argument (N, &argc, argv);
    Nsigma = atof(argv[N]);
    Niter = 3;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-iter"))) {
    remove_argument (N, &argc, argv);
    Niter = atoi(argv[N]);
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

  int valid = (argc == 2);
  valid |= (argc > 3) && !strcmp (argv[2], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: vstat (vector) [-ignore value] [-q] [-quiet] [-iter Niter] [-sigma-clip Nsigma] [where (logical expression)]\n");
    gprint (GP_ERR, "  default is 1 iteration without sigma clipping or 3 with sigma clipping\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  /* we need two passes, one for max, min, mean, sum, one for median, stdev, etc */

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
  if (argc > 3) {
    int size;
    char *out = dvomath (argc - 3, &argv[3], &size, 1);
    if (out == NULL) {
      print_error ();
      return FALSE;
    }
    if ((tvec = SelectVector (out, OLDVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, " invalid logic result\n");
      DeleteNamedVector (out);
      free (out);
      return (FALSE);
    }
    if (tvec->Nelements != vec->Nelements) {
      gprint (GP_ERR, "logical vector does not match in length\n");
      return FALSE;
    }
  }

  // set a good / bad mask (mask == 1 means 'bad')
  ALLOCATE (mask, char, vec[0].Nelements);
  if (vec[0].type == OPIHI_FLT) {
    opihi_flt *X = vec[0].elements.Flt;
    for (i = 0; i < vec[0].Nelements; i++, X++) {
      mask[i] = 0; // do not mask unless we have a reason below
      if (tvec) {
	// note the logical vector is 0 == bad (ignore) while mask[i] has the opposite sense (0 is good)
	mask[i] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0);
      }
      if (!finite (*X)) { mask[i] = 1; }
      if (Ignore && (*X == IgnoreValue)) { mask[i] = 1; }
    }      
  } else {
    opihi_int *X = vec[0].elements.Int;
    for (i = 0; i < vec[0].Nelements; i++, X++) {
      mask[i] = 0; // do not mask unless we have a reason below
      if (tvec) {
	// note the logical vector is 0 == bad (ignore) while mask[i] has the opposite sense (0 is good)
	mask[i] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0);
      }
      if (!finite (*X)) { mask[i] = 1; }
      if (Ignore && (*X == IgnoreValue)) { mask[i] = 1; }
    }      
  }

  /* calculate max, min, mean, sum, npix */
  max = -HUGE_VAL;
  min = HUGE_VAL;
  sum = N = 0;
  if (vec[0].type == OPIHI_FLT) {
    opihi_flt *X = vec[0].elements.Flt;
    for (i = 0; i < vec[0].Nelements; i++, X++) {
      if (mask[i]) continue;
      max = MAX (*X, max);
      min = MIN (*X, min);
      sum += *X;
      N++;
    }      
  } else {
    opihi_int *X = vec[0].elements.Int;
    for (i = 0; i < vec[0].Nelements; i++, X++) {
      if (mask[i]) continue;
      max = MAX (*X, max);
      min = MIN (*X, min);
      sum += *X;
      N++;
    }      
  }
  mean = sum / N;

  // we do Niter passes; on each pass, we exclude entries > Nsigma from the median
  pmin = min;
  pmax = max;

  stdev = mode = median = Nused = 0.0;

  for (iter = 0; iter < Niter; iter ++) {

    // reduce Nbin after the first iteration?
    Nbin = 1000;

    /* calculate median and mode with resolution of (max - min) / 1000 */ 
    dx = (pmax - pmin) / Nbin;
    if (dx == 0) {
      median = mode = min;
      stdev = 0.0;
      goto skip;
    }
    // we can hit inf if pmax and pmin are close to the max range
    if (isinf(dx)) dx = DBL_MAX / Nbin;

    ALLOCATE (Nval, int, Nbin + 2);
    bzero (Nval, (Nbin + 2)*sizeof(int));
    var = 0;
    if (vec[0].type == OPIHI_FLT) {
      opihi_flt *X = vec[0].elements.Flt;
      for (i = 0; i < vec[0].Nelements; i++, X++) {
	if (mask[i]) continue;
	bin = MAX (0, MIN (Nbin, (*X - pmin) / dx));
	Nval[bin] ++;
	dvar = (*X - mean);
	var += dvar*dvar;
      }      
    } else {
      opihi_int *X = vec[0].elements.Int;
      for (i = 0; i < vec[0].Nelements; i++, X++) {
	if (mask[i]) continue;
	bin = MAX (0, MIN (Nbin, (*X - pmin) / dx));
	Nval[bin] ++;
	dvar = (*X - mean);
	var += dvar*dvar;
      }      
    }
    stdev = sqrt (var / (N - 1));

    Nmode = 0;
    mode = Nval[Nmode];
    median = 0;
    Nmed = -1;
    for (i = 0; i < Nbin + 1; i++) {
      if (Nmed == -1) {
	median += Nval[i];
	if (median >= N / 2.0) {
	  Nmed = i;
	  median = i * dx + pmin;
	}
      }
      if (mode < Nval[i]) {
	Nmode = i;
	mode = Nval[Nmode];
      }
    }
    mode = Nmode * dx + pmin;
    free (Nval);

    threshold = Nsigma * stdev;

    // we are going to do another pass: mark the entries to skip
    pmin = min;
    pmax = max;
    if (iter < Niter - 1) {
      if (vec[0].type == OPIHI_FLT) {
	opihi_flt *X = vec[0].elements.Flt;
	for (i = 0; i < vec[0].Nelements; i++, X++) {
	  if (mask[i]) continue;
	  if (fabs(*X - median) > threshold) {
	    mask[i] = 1;
	  } else {
	    pmin = MIN (*X, pmin);
	    pmax = MAX (*X, pmax);
	  }      
	}
      } else {
	opihi_int *X = vec[0].elements.Int;
	for (i = 0; i < vec[0].Nelements; i++, X++) {
	  if (mask[i]) continue;
	  if (fabs(*X - median) > threshold) {
	    mask[i] = 1;
	  } else {
	    pmin = MIN (*X, pmin);
	    pmax = MAX (*X, pmax);
	  }
	}      
      }
    }
    // gprint (GP_ERR, "iter %d, mean: %g, stdev: %g, min: %g, max: %g, median: %g, mode: %g, Npts: %d\n", 
    // iter, mean, stdev, min, max, median, mode, N);
  }

  Nused = 0;
  for (i = 0; i < vec[0].Nelements; i++) {
    if (mask[i]) continue;
    Nused ++;
  }

skip:
  FREE(mask);
  if (mask) mask = NULL;

  if (!Quiet) {
    gprint (GP_ERR, "mean: %g, stdev: %g, min: %g, max: %g, median: %g, mode: %g, Npts: %d\n", 
	    mean, stdev, min, max, median, mode, N);
  }

  set_variable ("PMIN",      pmin);
  set_variable ("PMAX",      pmax);
  set_variable ("MIN",      min);
  set_variable ("MAX",      max);
  set_variable ("MEDIAN",   median);
  set_variable ("MEAN",     mean);
  set_variable ("MODE",     mode);
  set_variable ("TOTAL",    sum);
  set_int_variable ("NPIX", N);
  set_int_variable ("NPTS", N);
  set_int_variable ("NUSED", Nused);
  set_variable ("SIGMA",    stdev);
  return (TRUE);
}

