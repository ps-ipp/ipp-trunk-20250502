# include "basic.h"

int exec_usleep (int argc, char **argv) {

  int i;

  if (argc < 2) {
    gprint (GP_ERR, "usage: usleep N\n");
    return (FALSE);
  }

  i = atof (argv[1]);
  usleep (i);
  return (TRUE);
}
