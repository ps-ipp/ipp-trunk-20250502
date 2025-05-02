# include "pcontrol.h"

static int VERBOSE = FALSE;

int verbose (int argc, char **argv) {

  if (argc == 1) {
    if (VERBOSE) {
      gprint (GP_ERR, "verbose mode ON\n");
    } else {
      gprint (GP_ERR, "verbose mode OFF\n");
    }
    return (TRUE);
  }

  if (argc == 2) {
    if (!strcasecmp (argv[1], "ON")) {
      VERBOSE = TRUE;
      return (TRUE);
    }
    if (!strcasecmp (argv[1], "OFF")) {
      VERBOSE = FALSE;
      return (TRUE);
    }
    if (!strcasecmp (argv[1], "TOGGLE")) {
      VERBOSE = ~VERBOSE;
      return (TRUE);
    }
  }

  gprint (GP_ERR, "USAGE: verbose (on/off/toggle)\n");
  return (FALSE);
}

int VerboseMode () {
  return (VERBOSE);
}
