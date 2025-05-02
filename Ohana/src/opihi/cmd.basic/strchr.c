# include "basic.h"

int getchr_func (int argc, char **argv) {

  /* returns position of the first given character */ 
  char *c;
  int pos;

  if ((argc != 3) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: strchr (string) (char) [var]\n");
    return (FALSE);
  }

  c = strchr (argv[1], argv[2][0]);

  if (c == (char *) NULL) {
    pos = -1;
  } else {
    pos = c - argv[1];
  }

  if (argc == 5) {
    set_variable (argv[3], pos);
  } else {
    gprint (GP_ERR, "%d\n", pos);
  }
  return (TRUE);

}
