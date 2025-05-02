# include "basic.h"

int date (int argc, char **argv) {
  
  int N, SECONDS;
  double REFTIME;
  struct timeval now;
  char *tstring = NULL;
  char *varName = NULL;

  SECONDS = FALSE;
  if ((N = get_argument (argc, argv, "-seconds"))) {
    remove_argument (N, &argc, argv);
    SECONDS = TRUE;
  } else {
    ALLOCATE (tstring, char, 32);
  }

  REFTIME = 0.0;
  if ((N = get_argument (argc, argv, "-reftime"))) {
    remove_argument (N, &argc, argv);
    REFTIME = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } 

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: date [-var variable] [-seconds] [-reftime seconds]\n");
    return (FALSE);
  }

  gettimeofday (&now, NULL);
  if (SECONDS) {
    double nowSec = now.tv_sec + 1e-6*now.tv_usec;
    if (varName) {
      set_variable (varName, nowSec - REFTIME);
    } else {
      gprint (GP_ERR, "%.12g\n", nowSec - REFTIME);
    }
  } else {
    ctime_r (&now.tv_sec, tstring);
    N = strlen (tstring) - 1;
    tstring[N] = 0;

    if (varName) {
      set_str_variable (varName, tstring);
    } else {
      gprint (GP_ERR, "%s\n", tstring);
    }
    free (tstring);
  }
  return (TRUE);
}
