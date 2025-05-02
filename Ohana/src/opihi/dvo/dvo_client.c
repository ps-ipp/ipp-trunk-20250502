# include "dvoshell.h"

int input PROTO((int, char **));

/* program-dependent initialization */
void program_init (int *argc, char **argv) {
  
  int N;

  /* load the commands used by this implementation */
  InitBasic ();
  InitData ();
  InitAstro ();
  InitDVO ();

  {
    char *helpdir;
    char *modules;
    static char *datadir = "@DATADIR@";
    ALLOCATE (helpdir, char, strlen(datadir) + strlen("/help") + 2);
    sprintf (helpdir, "%s/help", datadir);
    set_str_variable ("HELPDIR", helpdir);
    free (helpdir);
    ALLOCATE (modules, char, strlen(datadir) + strlen("/modules") + 2);
    sprintf (modules, "%s/modules", datadir);
    set_str_variable ("MODULES:0", modules);
    set_int_variable ("MODULES:n", 1);
    free (modules);
  }

  set_int_variable ("DVO_CLIENT", TRUE);
  set_int_variable ("SCRIPT", FALSE);

  // dvo_client should have 2 standard arguments: -hostID and -hostdir
  HOST_ID = 0;
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);;
    remove_argument (N, argc, argv);
  }
  if (!HOST_ID) {
    fprintf (stderr, "ERROR: dvo_client requires a -hostID value\n");
    exit (3);
  }

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);;
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) {
    fprintf (stderr, "ERROR: dvo_client requires a -hostdir value\n");
    exit (3);
  }

  RESULT_FILE = NULL;
  if ((N = get_argument (*argc, argv, "-result"))) {
    remove_argument (N, argc, argv);
    RESULT_FILE = strcreate (argv[N]);;
    remove_argument (N, argc, argv);
  }

  // parse -skyregion option (used by most commands)
  set_skyregion (0.0, 360.0, -90.0, +90.0);
  if ((N = get_argument (*argc, argv, "-skyregion"))) {
    if (N + 4 >= *argc) {
      gprint (GP_ERR, "USAGE: -skyregion (RA) (RA) (DEC) (DEC)\n");
      exit (1);
    }
    remove_argument (N, argc, argv);
    set_skyregion (atof(argv[N]), atof(argv[N+1]), atof(argv[N+2]), atof(argv[N+3]));
    remove_argument (N, argc, argv);
    remove_argument (N, argc, argv);
    remove_argument (N, argc, argv);
    remove_argument (N, argc, argv);
  }    

  // parse -time option
  if ((N = get_argument (*argc, argv, "-time"))) {
    if (N + 2 >= *argc) {
      gprint (GP_ERR, "USAGE: -time (TimeRef) (TimeFormat)\n");
      exit (1);
    }
    remove_argument (N, argc, argv);
    set_str_variable ("TIMEREF", argv[N]);
    remove_argument (N, argc, argv);
    set_str_variable ("TIMEFORMAT", argv[N]);
    remove_argument (N, argc, argv);
  }

  // these are set in 'startup.c' for readline-based programs
  set_variable ("PID", getpid());
  set_str_variable ("KAPA", "kapa");
  set_int_variable ("UNSIGN", 0);
  gfits_set_unsign_mode (FALSE);

  return;
}

// dvo_client should be called like a dvo command;
int main (int argc, char **argv) {
  
  // parse out whatever might be needed up front
  general_init (&argc, argv);
  program_init (&argc, argv);

  if (argc < 2) {
    fprintf (stderr, "USAGE: dvo_client (command) (options)\n");
    exit (3);
  }

  int status = input (2, &argv[0]);
  if (!status) {
    fprintf (stderr, "WARNING: exit status 2 %d (%s)\n", HOST_ID, HOSTDIR);
    exit (2);
  }
  exit (0);

# if (0)
  // identiy the comm
  Command *cmd = MatchCommand (argv[1], TRUE, TRUE);
  if (cmd == NULL) {
    fprintf (stderr, "unknown command %s\n", argv[1]);
    exit (1);
  }

  // argv[0] is dvo_client; we want to pass in argv[1].. as argv[0]
  int status = (*cmd[0].func) (argc - 1, argv + 1);
  if (!status) exit (2);
  exit (0);
# endif
}

/* standard welcome message */
void welcome () {
  gprint (GP_ERR, "starting dvo_client...\n");
}

/* add program-dependent exit functions here */
void cleanup () {
  QuitKapa ();
  ConfigFree ();

  FreeBasic ();
  FreeData ();
  FreeAstro ();
  FreeDVO ();
  return;
}

