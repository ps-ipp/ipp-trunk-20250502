# include "basic.h"

int exec_sleep (int argc, char **argv) {

  int i;

  if (argc < 2) {
    gprint (GP_ERR, "usage: sleep N\n");
    return (FALSE);
  }

  i = atof (argv[1]);
  sleep (i);
  return (TRUE);
}
