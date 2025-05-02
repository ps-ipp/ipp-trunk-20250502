# include "data.h"

int vlist (int argc, char **argv) {
  
  int i, N;
  Vector *vec;
  
  int APPEND = FALSE;
  if ((N = get_argument (argc, argv, "-append"))) {
    APPEND = TRUE;
    remove_argument (N, &argc, argv);
  }

  int INT = FALSE;
  if ((N = get_argument (argc, argv, "-int"))) {
    if (APPEND) {
      gprint (GP_ERR, " ERROR : cannot mix -append and -int\n");
      return (FALSE);
    }
    INT = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc < 3) {
    gprint (GP_ERR, "USAGE: vlist [-append] [-int] vector value value ...\n");
    gprint (GP_ERR, "    -int : resulting vector is integer type (values are truncated to integer)\n");
    gprint (GP_ERR, " -append : append value to end of vector (otherwise deleted)\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  // for append, type comes from input vector
  if (APPEND) {
    INT = (vec[0].type == OPIHI_INT);
  }

  int Nitem = argc - 2;
  int Noffset = APPEND ? vec[0].Nelements : 0;
  vec[0].Nelements = APPEND ? vec[0].Nelements + Nitem : Nitem;

  if (INT) {
    vec[0].type = OPIHI_INT;
    REALLOCATE (vec[0].elements.Int, opihi_int, vec[0].Nelements);
    for (i = 0; i < Nitem; i++) {
      vec[0].elements.Int[i + Noffset] = atoi (argv[i+2]);
    }
  } else {
    vec[0].type = OPIHI_FLT;
    REALLOCATE (vec[0].elements.Flt, opihi_flt, vec[0].Nelements);
    for (i = 0; i < Nitem; i++) {
      vec[0].elements.Flt[i + Noffset] = atof (argv[i+2]);
    }
  }

  return (TRUE);
}

// vlist name 0 1 2 3
// argc = 6
// Nitem = 4
// Noffset = 0
// Int[0,1,2,3] = argv[2,3,4,5]
