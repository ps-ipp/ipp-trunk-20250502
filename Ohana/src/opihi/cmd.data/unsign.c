# include "data.h"

int unsign (int argc, char **argv) {
  
  if (argc == 1) {
    if (gfits_get_unsign_mode()) 
      gprint (GP_ERR, "mode is now UNSIGNED int \n");
    else
      gprint (GP_ERR, "mode is now SIGNED int \n");
    return (TRUE);
  }

  if (argc == 2) {
    if (!strcmp (argv[1], "1")) {
      gfits_set_unsign_mode (TRUE);
      set_int_variable ("UNSIGN", 1);
      return (TRUE);
    }
    if (!strcmp (argv[1], "0")) {
      gfits_set_unsign_mode (FALSE);
      set_int_variable ("UNSIGN", 0);
      return (TRUE);
    }
  }

  gprint (GP_ERR, "USAGE: unsign [0/1]\n");
  return (FALSE);
}
