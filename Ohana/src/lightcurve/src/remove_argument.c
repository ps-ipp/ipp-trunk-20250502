# include "lightcurve.h"

int remove_argument (N, argc, argv)
int    N;
int   *argc;
char **argv;
{

  int i;

  if ((N != (int)NULL) && (N != 0)) {
    (*argc)--;
    for (i = N; i < *argc; i++) {
      argv[i] = argv[i+1];
    }
  }

  return (N);
    
}

