# include "data.h"

int vprint (int argc, char **argv) {

  Vector **vec;
  int i, j, N;

  int START_VALUE = 0;
  if ((N = get_argument (argc, argv, "-s"))) {
    remove_argument (N, &argc, argv);
    START_VALUE = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int END_VALUE = -1;
  if ((N = get_argument (argc, argv, "-e"))) {
    remove_argument (N, &argc, argv);
    END_VALUE = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: vprint (vector)[,vector,...] [-s start] [-e end]\n");
    return (FALSE);
  }

  int Nvec = argc - 1;
  ALLOCATE (vec, Vector *, Nvec);
  for (i = 1; i < argc; i++) {
    vec[i-1] = SelectVector (argv[i], OLDVECTOR, FALSE);
    if (!vec[i-1]) {
      gprint (GP_ERR, "unknown vector %s\n", argv[i]);
      free (vec);
      return FALSE;
    }
  }

  int MaxLen = 0;
  for (i = 0; i < Nvec; i++) {
    MaxLen = MAX (MaxLen, vec[i][0].Nelements);
  }

  // start and end may be 0 - N (truncated to N) or may be negative, in which case it refers to 
  // distance from the end (just like vector[-5])
  START_VALUE = (START_VALUE < 0) ? MaxLen + START_VALUE + 1 : MIN (START_VALUE, MaxLen);
  START_VALUE = MAX (0, START_VALUE);

  END_VALUE = (END_VALUE < 0) ? MaxLen + END_VALUE + 1 : MIN (END_VALUE, MaxLen);
  END_VALUE = MAX (0, END_VALUE);

  for (j = START_VALUE; j < END_VALUE; j++) {
    for (i = 0; i < Nvec; i++) {
      if (j >= vec[i][0].Nelements) {
	gprint (GP_LOG, "NaN ");
      } else {
	if (vec[i][0].type == OPIHI_FLT) {
	  gprint (GP_LOG, "%f ", vec[i][0].elements.Flt[j]);
	} 
	if (vec[i][0].type == OPIHI_INT) {
	  gprint (GP_LOG, OPIHI_INT_FMT" ", vec[i][0].elements.Int[j]);
	} 
	if (vec[i][0].type == OPIHI_STR) {
	  gprint (GP_LOG, "%s ", vec[i][0].elements.Str[j]);
	} 
      }
    }
    gprint (GP_LOG, "\n");
  }
  free (vec);

  return (TRUE);
}

