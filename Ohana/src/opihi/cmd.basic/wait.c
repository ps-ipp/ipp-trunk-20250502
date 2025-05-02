# include "basic.h"

int wait_func (int argc, char **argv) {

  char buff[1024];
  int i;

  for (i = 1; i < argc; i++) {
    gprint (GP_ERR, "%s ", argv[i]);
  }
  gprint (GP_ERR, "\n");
  scan_line (stdin, buff);
  return (TRUE);
}
