# include "astro.h"

int jdtolst (int argc, char **argv) {

  int N;
  Vector *jdvec, *outvec;

  int VERBOSE = FALSE;
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  char *OutputName = NULL;
  if ((N = get_argument (argc, argv, "-output"))) {
    remove_argument (N, &argc, argv);
    OutputName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (argc != 2) goto syntax;

  char *long_str = get_variable ("LONGITUDE");
  if (!long_str) {
    gprint (GP_ERR, "please define $LONGITUDE (decimal hours, west positive)\n");
    FREE (OutputName);
    return FALSE;
  }
  double longitude = atof (long_str);
  if (VERBOSE) gprint (GP_ERR, "using longitude of %f (decimal hours, west positive)\n", longitude);
  free (long_str);

  if (ISNUM(argv[1][0])) {

    char *endptr;
    double jd = strtod (argv[1], &endptr);
    if (endptr == argv[1]) {
      gprint (GP_ERR, "error interpretting %s as a number\n", argv[1]);
      FREE (OutputName);
      return FALSE;
    }

    double lst = ohana_lst (jd, longitude);

    if (OutputName) {
      set_variable (OutputName, lst);
      FREE (OutputName);
      return (TRUE);
    }
    
    gprint (GP_ERR, "lst: %f\n", lst);
    return (TRUE);
  }

  if (!OutputName) {
    gprint (GP_ERR, " if (JD) is a vector, -output must be supplied\n");
    return FALSE;
  }

  if ((jdvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) {
    FREE (OutputName);
    return FALSE;
  }

  if (jdvec->type != OPIHI_FLT) {
    gprint (GP_ERR, " (JD) must be of type FLOAT\n");
    FREE (OutputName);
    return FALSE;
  }

  if ((outvec = SelectVector (OutputName, ANYVECTOR, TRUE)) == NULL) {
    FREE (OutputName);
    return FALSE;
  }

  ResetVector (outvec, OPIHI_FLT, jdvec[0].Nelements);

  for (int i = 0; i < jdvec[0].Nelements; i++) {
    double lst = ohana_lst (jdvec->elements.Flt[i], longitude);
    outvec->elements.Flt[i] = lst;
  }
  FREE (OutputName);
  return TRUE;

 syntax:
  gprint (GP_ERR, "USAGE: jdtolst (JD) [-output name]\n");
  gprint (GP_ERR, "  (JD) may be a number or a vector.\n");
  gprint (GP_ERR, "  if (JD) is a vector, (output) must be supplied and is used as the name of the output vector \n");
  gprint (GP_ERR, "  $LONGITUDE must be set (decimal hours, west positive)\n");
  return (FALSE);
}

# define MJD_OFFSET 2400000.5

int mjdtolst (int argc, char **argv) {

  int N;
  Vector *mjdvec, *outvec;

  int VERBOSE = FALSE;
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  char *OutputName = NULL;
  if ((N = get_argument (argc, argv, "-output"))) {
    remove_argument (N, &argc, argv);
    OutputName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (argc != 2) goto syntax;

  char *long_str = get_variable ("LONGITUDE");
  if (!long_str) {
    gprint (GP_ERR, "please define $LONGITUDE (decimal hours, west positive)\n");
    FREE (OutputName);
    return FALSE;
  }
  double longitude = atof (long_str);
  if (VERBOSE) gprint (GP_ERR, "using longitude of %f (decimal hours, west positive)\n", longitude);
  free (long_str);

  if (ISNUM(argv[1][0])) {

    char *endptr;
    double mjd = strtod (argv[1], &endptr);
    if (endptr == argv[1]) {
      gprint (GP_ERR, "error interpretting %s as a number\n", argv[1]);
      FREE (OutputName);
      return FALSE;
    }

    double lst = ohana_lst (mjd + MJD_OFFSET, longitude);

    if (OutputName) {
      set_variable (OutputName, lst);
      FREE (OutputName);
      return (TRUE);
    }
    
    gprint (GP_ERR, "lst: %f\n", lst);
    return (TRUE);
  }

  if (!OutputName) {
    gprint (GP_ERR, " if (MJD) is a vector, -output must be supplied\n");
    return FALSE;
  }

  if ((mjdvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) {
    FREE (OutputName);
    return FALSE;
  }

  if (mjdvec->type != OPIHI_FLT) {
    gprint (GP_ERR, " (MJD) must be of type FLOAT\n");
    FREE (OutputName);
    return FALSE;
  }

  if ((outvec = SelectVector (OutputName, ANYVECTOR, TRUE)) == NULL) {
    FREE (OutputName);
    return FALSE;
  }

  ResetVector (outvec, OPIHI_FLT, mjdvec[0].Nelements);

  for (int i = 0; i < mjdvec[0].Nelements; i++) {
    double lst = ohana_lst (mjdvec->elements.Flt[i] + MJD_OFFSET, longitude);
    outvec->elements.Flt[i] = lst;
  }
  FREE (OutputName);
  return TRUE;

 syntax:
  gprint (GP_ERR, "USAGE: mjdtolst (MJD) [-output name]\n");
  gprint (GP_ERR, "  (MJD) may be a number or a vector.\n");
  gprint (GP_ERR, "  if (MJD) is a vector, (output) must be supplied and is used as the name of the output vector \n");
  gprint (GP_ERR, "  $LONGITUDE must be set (decimal hours, west positive)\n");
  return (FALSE);
}
