# include "data.h"

int bisection (int argc, char **argv) {
  
  int N;
  Vector *vec;

  int QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: bisection <vector> (threshold)\n");
    gprint (GP_ERR, "  return the vector index for which the vector is last below the threshold via bisection\n");
    gprint (GP_ERR, "  vector must be sorted\n");
    return (FALSE);
  }
  
  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  double threshold = atof (argv[2]);
  
  int isFlt = (vec[0].type == OPIHI_FLT);

  if (isFlt) {
    N = ohana_bisection_double (vec[0].elements.Flt, vec[0].Nelements, threshold);
  } else {
    // N = ohana_bisection_int (vec[0].elements.Flt, vec[0].Nelements);
  }

  set_variable ("bisecbin", N);

  if (!QUIET) gprint (GP_LOG, "bin %d for theshold %f\n", N, threshold);

  return (TRUE);
}
