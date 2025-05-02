# include "data.h"

int integrate (int argc, char **argv) {
  
  int N;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  int TRAPEZOID = TRUE;
  if ((N = get_argument (argc, argv, "-trapezoid"))) {
    TRAPEZOID = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-trap"))) {
    TRAPEZOID = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-t"))) {
    TRAPEZOID = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-rectangular"))) {
    TRAPEZOID = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-rect"))) {
    TRAPEZOID = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-r"))) {
    TRAPEZOID = FALSE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: integrate <x> <y> start end [-v] [-r] [-t]\n");
    gprint (GP_ERR, " -v : verbose mode - report $sum and $range\n");
    gprint (GP_ERR, " -r : use rectangular integration\n");
    gprint (GP_ERR, " -t : use trapezoidal integration (default)\n");
    return (FALSE);
  }

  Vector *vecx = NULL, *vecy = NULL;
  if ((vecx = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  double start = atof (argv[3]);
  double end   = atof (argv[4]);

  int isFloat = vecy[0].type == OPIHI_FLT;
  opihi_flt *Yf = vecy[0].elements.Flt;
  opihi_int *Yi = vecy[0].elements.Int;

  double value = 0;
  double range = 0;

  if (TRAPEZOID) {
    if (vecx[0].type == OPIHI_FLT) {
      opihi_flt *X = vecx[0].elements.Flt;
      for (int i = 0; i < vecx[0].Nelements-1; i++, X++, Yi++, Yf++) {
	if ((*X >= start) && (*X <= end)) {
	  double V0 = (isFloat) ? Yf[0] : Yi[0];
	  double V1 = (isFloat) ? Yf[1] : Yi[1];
	  value += (V1 + V0) * (X[1] - X[0]) / 2;
	  range += (X[1] - X[0]);
	}
      }      
    } else {
      opihi_int *X = vecx[0].elements.Int;
      for (int i = 0; i < vecx[0].Nelements-1; i++, X++, Yi++, Yf++) {
	if ((*X >= start) && (*X <= end)) {
	  double V0 = (isFloat) ? Yf[0] : Yi[0];
	  double V1 = (isFloat) ? Yf[1] : Yi[1];
	  value += (V1 + V0) * (X[1] - X[0]) / 2;
	  range += (X[1] - X[0]);
	}
      }      
    }
  } else {
    if (vecx[0].type == OPIHI_FLT) {
      opihi_flt *X = vecx[0].elements.Flt;
      for (int i = 0; i < vecx[0].Nelements-1; i++, X++, Yi++, Yf++) {
	if ((*X >= start) && (*X <= end)) {
	  double dvalue = (isFloat) ? *Yf : *Yi;
	  value += dvalue * (X[1] - X[0]);
	  range += (X[1] - X[0]);
	}
      }      
    } else {
      opihi_int *X = vecx[0].elements.Int;
      for (int i = 0; i < vecx[0].Nelements-1; i++, X++, Yi++, Yf++) {
	if ((*X >= start) && (*X <= end)) {
	  double dvalue = (isFloat) ? *Yf : *Yi;
	  value += dvalue * (X[1] - X[0]);
	  range += (X[1] - X[0]);
	}
      }      
    }
  }

  set_variable ("sum", value); 
  set_variable ("range", range);
  if (VERBOSE) gprint (GP_LOG, "sum: %f\n", value);
  return (TRUE);
}

