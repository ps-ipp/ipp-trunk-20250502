# include "pantasks.h"

int server_load (int argc, char **argv) {

  char *input;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: server load (file)\n");
    return (FALSE);
  }

  // the allocated input string is eventually freed 
  // by the InputQueue system
  input = strcreate (argv[1]);
  AddNewInput (input);

  return (TRUE);
}
