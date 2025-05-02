# include "data.h"

int QUIET = FALSE;

void _threshold_set_values (double value, int Nbin, double level, int thresherr) {

  set_variable ("threshval", value);
  set_int_variable ("threshbin", Nbin);
  set_variable ("threshold", level);
  set_int_variable ("thresherr", thresherr);

  if (!QUIET) gprint (GP_LOG, "theshold %f (bin %d is %f)\n", level, Nbin, value);
  return;
}

int threshold (int argc, char **argv) {
  
  int N;
  Vector *vecx, *vecy;

  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  int SATURATE = FALSE;
  if ((N = get_argument (argc, argv, "-saturate"))) {
    SATURATE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-sat"))) {
    SATURATE = TRUE;
    remove_argument (N, &argc, argv);
  }

  int REVERSE = FALSE;
  if ((N = get_argument (argc, argv, "-r"))) {
    REVERSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  int BinMin = -1;
  int BinMax = -1;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    BinMin = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    BinMax = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  float ValMin = NAN;
  float ValMax = NAN;
  if ((N = get_argument (argc, argv, "-vrange"))) {
    remove_argument (N, &argc, argv);
    ValMin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ValMax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: threshold <x> <y> (value) [-q] [-r] [-range BinMin BinMax] \n");
    gprint (GP_ERR, "  find the x coordinate at which we pass the specified value\n");
    gprint (GP_ERR, "  by default, y must be monotonically increasing\n");
    gprint (GP_ERR, "  -q : quiet mode\n");
    gprint (GP_ERR, "  -r : reverse (find downward transition)\n");
    return (FALSE);
  }
  
  if ((vecx = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (vecx[0].Nelements != vecy[0].Nelements) return FALSE;
  int Nelements = vecx[0].Nelements;
  
  double value = atof (argv[3]);
  
  int isFltX = (vecx[0].type == OPIHI_FLT);
  int isFltY = (vecy[0].type == OPIHI_FLT);

  // BinMin = -1 -> range not provide
  if (BinMin == -1) {
    BinMin = 0;
    BinMax = Nelements - 1;
  } else {
    if ((BinMin >= vecx[0].Nelements) || (BinMin < -1*vecx[0].Nelements)) {
      gprint (GP_ERR, "vector subscript out of range\n"); 
      return FALSE;
    }
    if (BinMin < 0) BinMin += vecx[0].Nelements;
    if ((BinMax >= vecx[0].Nelements) || (BinMax < -1*vecx[0].Nelements)) {
      gprint (GP_ERR, "vector subscript out of range\n"); 
      return FALSE;
    }
    if (BinMax < 0) BinMax += vecx[0].Nelements;
  }

  // ValMin, ValMax not compatible with BinMin, BinMax
  if (isfinite(ValMin)) {
    int binTest = ohana_bisection_double (vecx[0].elements.Flt, vecx[0].Nelements, ValMin);
    BinMin = (vecx[0].elements.Flt[binTest+1] == ValMin) ? binTest : binTest + 1;
    // XXX dangerous: check that this does not run off the end, etc
  }
  if (isfinite(ValMax)) {
    int binTest = ohana_bisection_double (vecx[0].elements.Flt, vecx[0].Nelements, ValMax);
    BinMax = (vecx[0].elements.Flt[binTest+1] == ValMax) ? binTest : binTest + 1;
    // XXX dangerous: check that this does not run off the end, etc
  }

  // use bisection to find the value

  // find the last entry before start
  int Nlo = BinMin;
  int Nhi = BinMax;

  if (!REVERSE) {
    // this algorithm assumes vecy[BinMax] > threshold, vecy[BinMin] < threshold
    if (vecy[0].elements.Flt[BinMin] > value) {
      if (SATURATE) {
	_threshold_set_values (vecy[0].elements.Flt[BinMin], BinMin, vecx[0].elements.Flt[BinMin], 1);
	return TRUE;
      } else {
	gprint (GP_ERR, "ERROR: all values above threshold\n");
	_threshold_set_values (NAN, -1, NAN, 1);
	return FALSE;
      }
    }
    if (vecy[0].elements.Flt[BinMax] < value) {
      if (SATURATE) {
	_threshold_set_values (vecy[0].elements.Flt[BinMax], BinMax, vecx[0].elements.Flt[BinMax], 1);
	return TRUE;
      } else {
	gprint (GP_ERR, "ERROR: all values below threshold\n");
	_threshold_set_values (NAN, -1, NAN, 1);
	return FALSE;
      }
    }
    while (Nhi - Nlo > 10) {
      N = 0.5*(Nlo + Nhi);
      double testval = isFltY ? vecy[0].elements.Flt[N] : vecy[0].elements.Int[N];
      if (testval < value) {
	Nlo = MAX(N, 0);
      } else {
	Nhi = MIN(N, Nelements - 1);
      }
    }
    // v[Nlo] < value <= v[Nhi]
    for (N = Nlo; N <= Nhi; N++) {
      double testval = isFltY ? vecy[0].elements.Flt[N] : vecy[0].elements.Int[N];
      if (testval > value) {
	Nhi = N;
	break;
      }
    }
  } else {
    // this algorithm assumes vecy[BinMin] > threshold, vecy[BinMax] < threshold
    if (vecy[0].elements.Flt[BinMin] < value) {
      if (SATURATE) {
	_threshold_set_values (vecy[0].elements.Flt[BinMin], BinMin, vecx[0].elements.Flt[BinMin], 1);
	return TRUE;
      } else {
	gprint (GP_ERR, "ERROR: all values below threshold\n");
	_threshold_set_values (NAN, -1, NAN, 1);
	return FALSE;
      }
    }
    if (vecy[0].elements.Flt[BinMax] > value) {
      if (SATURATE) {
	_threshold_set_values (vecy[0].elements.Flt[BinMax], BinMax, vecx[0].elements.Flt[BinMax], 1);
	return TRUE;
      } else {
	gprint (GP_ERR, "ERROR: all values above threshold\n");
	_threshold_set_values (NAN, -1, NAN, 1);
	return FALSE;
      }
    }
    while (Nhi - Nlo > 10) {
      N = 0.5*(Nlo + Nhi);
      double testval = isFltY ? vecy[0].elements.Flt[N] : vecy[0].elements.Int[N];
      if (testval > value) {
	Nlo = MAX(N, 0);
      } else {
	Nhi = MIN(N, Nelements - 1);
      }
    }
    // v[Nlo] > value >= v[Nhi]
    for (N = Nlo; N <= Nhi; N++) {
      double testval = isFltY ? vecy[0].elements.Flt[N] : vecy[0].elements.Int[N];
      if (testval < value) {
	Nhi = N;
	break;
      }
    }
  }
  // v[Nhi] is transition bin
  
  double x0, x1, y0, y1, Xvalue;
  if (Nhi == 0) {
    // interpolate to value:
    y0 = isFltY ? vecy[0].elements.Flt[Nhi] : vecy[0].elements.Int[Nhi];
    y1 = isFltY ? vecy[0].elements.Flt[Nhi+1]   : vecy[0].elements.Int[Nhi+1];
    x0 = isFltX ? vecx[0].elements.Flt[Nhi] : vecy[0].elements.Int[Nhi];
    x1 = isFltX ? vecx[0].elements.Flt[Nhi+1]   : vecy[0].elements.Int[Nhi+1];
    if (y0 == y1) {
      Xvalue = 0.5*(x0 + x1);
    } else {
      Xvalue = (value - y0) * (x1 - x0) / (y1 - y0) + x0;
    }    
  } else {
    // interpolate to value:
    y0 = isFltY ? vecy[0].elements.Flt[Nhi-1] : vecy[0].elements.Int[Nhi-1];
    y1 = isFltY ? vecy[0].elements.Flt[Nhi]   : vecy[0].elements.Int[Nhi];
    x0 = isFltX ? vecx[0].elements.Flt[Nhi-1] : vecy[0].elements.Int[Nhi-1];
    x1 = isFltX ? vecx[0].elements.Flt[Nhi]   : vecy[0].elements.Int[Nhi];
    if (y0 == y1) {
      Xvalue = 0.5*(x0 + x1);
    } else {
      Xvalue = (value - y0) * (x1 - x0) / (y1 - y0) + x0;
    }
  }

  _threshold_set_values (y1, Nhi, Xvalue, 0);
  return (TRUE);
}
