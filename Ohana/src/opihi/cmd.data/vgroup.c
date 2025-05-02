# include "data.h"

enum {USE_MEDIAN, USE_COUNT, USE_SUM, USE_MEAN};

int vgroup (int argc, char **argv) {
  
  int i, j, N;
  Vector *xin, *yin, *xout, *yout;
  opihi_flt xmin, xmax, sum;
  double *values;

  // Normalize = FALSE;
  // if ((N = get_argument (argc, argv, "-norm"))) {
  //   remove_argument (N, &argc, argv);
  //   Normalize = TRUE;
  // }
  // 
  // Ignore = FALSE;
  // IgnoreValue = 0.0;
  // if ((N = get_argument (argc, argv, "-ignore"))) {
  //   Ignore = TRUE;
  //   remove_argument (N, &argc, argv);
  //   IgnoreValue = atof (argv[N]);
  //   remove_argument (N, &argc, argv);
  // }

  int mode = USE_MEDIAN;
  if ((N = get_argument (argc, argv, "-sum"))) {
    remove_argument (N, &argc, argv);
    mode = USE_SUM;
  }
  if ((N = get_argument (argc, argv, "-mean"))) {
    remove_argument (N, &argc, argv);
    mode = USE_MEAN;
  }

  float binsize = NAN;
  if ((N = get_argument (argc, argv, "-binsize"))) {
    remove_argument (N, &argc, argv);
    binsize = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: vgroup <xin> <yin> <xout> <yout>\n");
    gprint (GP_ERR, " group x,y values in bins defined by <xout>\n");
    gprint (GP_ERR, " by default, yout has the median of the associated input values\n");
    gprint (GP_ERR, " use -sum to add values in the bin\n");
    gprint (GP_ERR, " use <yin> = histogram count matching values\n");
    gprint (GP_ERR, " use -binsize to specify a fixed bin width (<xout> will define the bin center)\n");
    return (FALSE);
  }

  if ((xin  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((xout = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yout = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  yin = NULL;

  // this should conflict with the -sum option...
  if (!strcmp(argv[2], "histogram")) {
    mode = USE_COUNT;
  } else {
    if ((yin  = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  }

  // re-binning creates a float vector
  ResetVector (yout, OPIHI_FLT, xout[0].Nelements);

  // storage vector
  ALLOCATE (values, double, xin[0].Nelements);

  for (i = 0; i < xout[0].Nelements - 1; i++) {
    if (isnan(binsize)) {
      xmin = xout[0].elements.Flt[i];
      xmax = xout[0].elements.Flt[i+1];
    } else {
      xmin = xout[0].elements.Flt[i] - 0.5*binsize;
      xmax = xout[0].elements.Flt[i] + 0.5*binsize;
    }

    N = 0;
    for (j = 0; j < xin[0].Nelements; j++) {
      if (xin[0].elements.Flt[j] < xmin) continue;
      if (xin[0].elements.Flt[j] > xmax) continue;
      if (yin) {
	values[N] = yin[0].elements.Flt[j];
      }
      N++;
    }
    
    sum = NAN;
    switch (mode) {
      case USE_MEDIAN:
	dsort (values, N);
	if (N > 1) {
	  sum = (N % 2) ? values[(int)(0.5*N)] : 0.5*(values[N/2] + values[N/2 + 1]);
	} else {
	  sum = values[0];
	}
	break;

      case USE_SUM:
	sum = 0.0;
	for (j = 0; j < N; j++) {
	  sum += values[j];
	}
	break;
	
      case USE_COUNT:
	sum = N;
	break;

      case USE_MEAN:
	sum = 0.0;
	for (j = 0; j < N; j++) {
	  sum += values[j];
	}
	sum /= N;
	break;
    }
    yout[0].elements.Flt[i] = sum;
  }
  free (values);
  return (TRUE);
}
