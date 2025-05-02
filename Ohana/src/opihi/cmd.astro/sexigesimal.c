# include "astro.h"

int sexigesimal (int argc, char **argv) {
  
  int HMS, N;
  double value;
  char string[80];

  HMS = TRUE;
  if ((N = get_argument (argc, argv, "-hms"))) {
    HMS = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-hh"))) {
    HMS = FALSE;
    remove_argument (N, &argc, argv);
  }

  if ((argc != 3) && (argc != 2)) {
    gprint (GP_ERR, "USAGE: sexigesimal (from) [to]\n");
    return (FALSE);
  }

  if (HMS) {
    if (!ohana_dms_to_ddd (&value, argv[1])) {
      gprint (GP_ERR, "syntax error in input\n");
      return (FALSE);
    }
    if (argc == 3) {
      set_variable (argv[2], value);
    } else {
      gprint (GP_LOG, "%10.6f\n", value);
    }
    return (TRUE);
  } else {
    value = atof (argv[1]);
    hms_format (string, 80, value);
    if (argc == 3) {
      set_str_variable (argv[2], string);
    } else {
      gprint (GP_LOG, "%s\n", string);
    }
    return (TRUE);
  }      

  return (TRUE);

}

