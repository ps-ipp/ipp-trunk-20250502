# include "data.h"

int spline_list (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: spline list\n");
    return FALSE;
  }

  ListSplines();
  return TRUE;
}

int spline_create (int argc, char **argv) {

  int i, N;
  Vector *xvec, *yvec;

  double dyLower = NAN;
  double dyUpper = NAN;
  if ((N = get_argument (argc, argv, "-dyLower"))) {
    remove_argument (N, &argc, argv);
    dyLower = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dyUpper"))) {
    remove_argument (N, &argc, argv);
    dyUpper = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: spline create (name) (Xknots) (Yknots)\n");
    return FALSE;
  }

  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (xvec->Nelements != yvec->Nelements) {
    gprint (GP_ERR, "x and y vectors must be the same length\n");
    return FALSE;
  }

  if (xvec->Nelements < 3) {
    gprint (GP_ERR, "cannot make a spline with fewer than 3 knots\n");
    return FALSE;
  }

  Spline *myspline = CreateSpline (argv[1], xvec->Nelements);

  for (i = 0; i < xvec->Nelements; i++) {
    myspline->xk[i] = xvec->elements.Flt[i];
    myspline->yk[i] = yvec->elements.Flt[i];
  }    

  spline_construct_dbl (myspline->xk, myspline->yk, myspline->Nknots, myspline->y2, dyLower, dyUpper);
  return TRUE;
}

int spline_apply (int argc, char **argv) {

  int i;
  Vector *xvec, *yvec;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: spline apply (name) (Xpoints) (Ypoints)\n");
    return FALSE;
  }

  Spline *myspline = FindSpline (argv[1]);
  if (!myspline) {
    gprint (GP_ERR, "spline %s not found\n", argv[1]);
    return (FALSE);
  }

  if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (yvec, OPIHI_FLT, xvec->Nelements);

  for (i = 0; i < xvec->Nelements; i++) {
    opihi_flt value = spline_apply_dbl (myspline->xk, myspline->yk, myspline->y2, myspline->Nknots, xvec->elements.Flt[i]);
    yvec->elements.Flt[i] = value;
  }    

  return TRUE;
}

int spline_save (int argc, char **argv) {

  int N;

  int APPEND = FALSE;
  if ((N = get_argument (argc, argv, "-append"))) {
    APPEND = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: spline save (name) (filename) [-append]\n");
    return FALSE;
  }

  if (!SaveSpline(argv[2], argv[1], APPEND)) {
    gprint (GP_ERR, "failed to save spline %s\n", argv[1]);
    return (FALSE);
  }
  return TRUE;
}

int spline_print (int argc, char **argv) {

  int i;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: spline print (name)\n");
    return FALSE;
  }

  Spline *myspline = FindSpline (argv[1]);
  if (!myspline) {
    gprint (GP_ERR, "spline %s not found\n", argv[1]);
    return (FALSE);
  }

  for (i = 0; i < myspline->Nknots; i++) {
    gprint (GP_LOG, "%lf : %lf : %lf\n", myspline->xk[i], myspline->yk[i], myspline->y2[i]);
  }    

  return TRUE;
}

int spline_load (int argc, char **argv) {

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: spline load (name) (filename)\n");
    return FALSE;
  }

  if (!LoadSpline(argv[2], argv[1])) {
    gprint (GP_ERR, "failed to load spline %s\n", argv[1]);
    return (FALSE);
  }
  return TRUE;
}

int spline_delete (int argc, char **argv) {

  int N, status;

  int QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: spline delete (spline)\n");
    return FALSE;
  }

  Spline *spline = FindSpline (argv[1]);
  if (spline == NULL) {
    if (QUIET) return TRUE;
    gprint (GP_ERR, "spline %s not found\n", argv[1]);
    return FALSE;
  }

  status = DeleteSpline (spline);
  if (!status) abort ();
  return TRUE;
}

int spline_rename (int argc, char **argv) {

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: spline rename (spline) (newname)\n");
    return FALSE;
  }

  Spline *spline = FindSpline (argv[1]);
  if (spline == NULL) {
    gprint (GP_ERR, "spline %s not found\n", argv[1]);
    return FALSE;
  }

  Spline *newname = FindSpline (argv[2]);
  if (newname != NULL) {
    DeleteSpline (newname);
  }

  free (spline->name);
  spline->name = strcreate (argv[2]);
  return TRUE;
}
