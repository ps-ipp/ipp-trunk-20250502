# include "data.h"

int create (int argc, char **argv) {
  
  int i, N;
  opihi_flt start, end, delta;
  Vector *vec;
  
  // create a vector of empty strings
  if ((N = get_argument (argc, argv, "-str"))) {
    remove_argument (N, &argc, argv);

    char *stringValue = NULL;
    if ((N = get_argument (argc, argv, "-value"))) {
      remove_argument (N, &argc, argv);
      stringValue = strcreate (argv[N]);
      remove_argument (N, &argc, argv);
    }

    if (argc != 3) {
      gprint (GP_ERR, "USAGE: create vector Nelements -value word\n");
      return (FALSE);
    }

    if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    int Nelements = atoi (argv[2]);

    // a newly reset vector has NULL-valued pointers
    ResetVector (vec, OPIHI_STR, Nelements);
    for (int i = 0; i < Nelements; i++) {
      vec[0].elements.Str[i] = stringValue ? strcreate (stringValue) : strcreate ("");
    }
    return TRUE;
  }

  int INT = FALSE;
  if ((N = get_argument (argc, argv, "-int"))) {
    INT = TRUE;
    remove_argument (N, &argc, argv);
  }

  int EMPTY = FALSE;
  if ((N = get_argument (argc, argv, "-empty"))) {
    EMPTY = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (!EMPTY && (argc != 5) && (argc != 4)) goto usage;
  if (EMPTY && (argc != 2)) goto usage;

  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if (EMPTY) {
    ResetVector (vec, (INT ? OPIHI_INT : OPIHI_FLT), 0);
    return TRUE;
  }

  delta = 1;
  start = atof (argv[2]);
  end   = atof (argv[3]);
  if (argc == 5) delta = atof (argv[4]);

  if (!isfinite(start) || !isfinite(end) || (start == end) || (delta == 0)) {
    gprint (GP_ERR, "error in value: %f to %f, %f\n", start, end, delta);
    return (FALSE);
  }
  delta = fabs (delta);
  if (end - start < 0) {
    delta = -1.0 * delta;
  }

  if (INT && (delta != (int)delta)) {
    gprint (GP_ERR, "integer vector requested but fractional step-size specified\n");
    return (FALSE);
  }

  // make a 1gig hard limit (test as below to avoid numerical limits of the division)
  // 0x4000.0000 bytes is a gigabyte, divide by 8 for bytes/element
  if (0x8000000 * delta < end - start) { 
    gprint (GP_ERR, "ERROR: attempt to create >1gigabyte vector\n");
    return (FALSE);
  }
  vec[0].Nelements = MAX(0, (end - start) / delta);

  if (INT) {
    vec[0].type = OPIHI_INT;
    REALLOCATE (vec[0].elements.Int, opihi_int, vec[0].Nelements);
    for (i = 0; i < vec[0].Nelements; i++) {
      vec[0].elements.Int[i] = start + i*delta;
    }
  } else {
    vec[0].type = OPIHI_FLT;
    REALLOCATE (vec[0].elements.Flt, opihi_flt, vec[0].Nelements);
    for (i = 0; i < vec[0].Nelements; i++) {
      vec[0].elements.Flt[i] = start + i*delta;
    }
  }

  return (TRUE);

 usage:
  gprint (GP_ERR, "USAGE: create vector start end [delta] [-int]\n");
  gprint (GP_ERR, " -int : resulting vector is integer type (delta must be integer)\n");
  gprint (GP_ERR, " -str : resulting vector is string type (only give number of elements)\n");
  gprint (GP_ERR, "OR: create vector -empty (creates vector of length zero)\n");
  return (FALSE);
}
