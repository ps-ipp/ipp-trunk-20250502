# include "basic.h"

int macro (int argc, char **argv) {

  int status;
  CommandF *cmd;

  if ((argc != 2) && (argc != 3) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: macro (cmd)\n");
    gprint (GP_ERR, "  (cmd) can be one of:\n");
    gprint (GP_ERR, "    (name)         -- create macro (name)\n");
    gprint (GP_ERR, "    create (name)  -- create macro (name)\n");
    gprint (GP_ERR, "    delete (name)  -- delete macro (name)\n");
    gprint (GP_ERR, "    list   (name)  -- list macro (name)\n");
    gprint (GP_ERR, "    edit   (name)  -- edit macro (name) <not working yet!> *\n");
    gprint (GP_ERR, "    read   (name)  -- read macro(s) from file (name) <not working yet!> *\n");
    gprint (GP_ERR, "    write  (name)  -- write macro (name) to a file <not working yet!> *\n");
    return (FALSE);
  }

  cmd = find_macro_command (argv[1]);
  if (cmd != NULL) {
    status = (*cmd) (argc - 1, argv + 1);
  } else {
    /* sub-command was not found, pass argv[1..N] to macro_create */
    status = macro_create (argc, argv);
  }

  return (status);
}

/* macro is called with the command "macro".  
   the command line word "macro" is meant to be followed the one of several 
   possible options:
   
   macro create
   macro delete
   macro list
   macro edit
   macro read
   macro write

*/
