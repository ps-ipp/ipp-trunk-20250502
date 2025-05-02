# include "lightcurve.h"

int get_argument (argc, argv, arg)
int    argc;
char **argv;
char  *arg;
{

  int i;

  for (i = 0; i < argc; i++) {
    if (!strcmp(argv[i], arg))
      return (i);
  }
  
  return ((int)NULL);
}


