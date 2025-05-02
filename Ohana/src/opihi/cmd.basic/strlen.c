# include "basic.h"

int strlen_func (int argc, char **argv) {

  /* returns length of the given string */ 
  int len;

  if ((argc != 2) && (argc != 3)) {
    gprint (GP_ERR, "USAGE: strlen (string) [var]\n");
    return (FALSE);
  }

  len = strlen (argv[1]);

  if (argc == 3) {
    set_int_variable (argv[2], len);
  } else {
    gprint (GP_ERR, "%d\n", len);
  }
  return (TRUE);

}
