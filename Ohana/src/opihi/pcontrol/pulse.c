# include "pantasks.h"

int pulse (int argc, char **argv) {

  int Nusec;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: pulse (microseconds)\n");
    return (FALSE);
  }

  Nusec = atoi (argv[1]);
  rl_set_keyboard_input_timeout (Nusec); 

  return (TRUE);
}
