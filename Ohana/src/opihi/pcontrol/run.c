# include "pcontrol.h"

int run (int argc, char **argv) {

  RunLevels level;

  if ((argc > 2) ||
      get_argument (argc, argv, "help") ||
      get_argument (argc, argv, "-h") ||
      get_argument (argc, argv, "--help")) 
  {
    if (!strcmp (argv[0], "run")) {
      gprint (GP_ERR, "USAGE: run [level]\n");
      gprint (GP_ERR, "  allowed levels:\n");
      gprint (GP_ERR, "  all (default) : manage machines, spawn jobs, harvest jobs\n");
      gprint (GP_ERR, "  reap          : manage machines, harvest jobs\n");
      gprint (GP_ERR, "  hosts         : manage machines, not jobs\n");
      gprint (GP_ERR, "  none          : all stop\n");
    } else {
      gprint (GP_ERR, "USAGE: stop (immediate processing halt)\n");
    }
    return (FALSE);
  }

  level = PCONTROL_RUN_UNKNOWN;
  if (argc == 1) {
    if (!strcasecmp (argv[0], "run")) level = PCONTROL_RUN_ALL;
    if (!strcasecmp (argv[0], "stop")) level = PCONTROL_RUN_NONE;
  } else {
    if (!strcasecmp (argv[1], "all")) level = PCONTROL_RUN_ALL;
    if (!strcasecmp (argv[1], "reap")) level = PCONTROL_RUN_REAP;
    if (!strcasecmp (argv[1], "host")) level = PCONTROL_RUN_HOSTS;
    if (!strcasecmp (argv[1], "hosts")) level = PCONTROL_RUN_HOSTS;
    if (!strcasecmp (argv[1], "none")) level = PCONTROL_RUN_NONE;
  }

  if (level == PCONTROL_RUN_UNKNOWN) {
    gprint (GP_ERR, "  unknown run level %s\n", argv[1]);
    gprint (GP_ERR, "  allowed levels:\n");
    gprint (GP_ERR, "  all (default) : manage machines, spawn jobs, harvest jobs\n");
    gprint (GP_ERR, "  reap          : manage machines, harvest jobs\n");
    gprint (GP_ERR, "  hosts         : manage machines, not jobs\n");
    gprint (GP_ERR, "  none          : all stop\n");
    return (FALSE);
  }

# ifdef THREADED
  SetRunLevel (level);
# else
  if (level == PCONTROL_RUN_NONE) {
    rl_event_hook = NULL;
  } else {
    rl_event_hook = CheckSystem;
  }
# endif

  return (TRUE);
}

/* 
   run levels:
   all (manage machines, spawn jobs, harvest jobs)
   reap (manage machines, harvest jobs)
   hosts (manage machines, not jobs)
   none (all stop)
*/
