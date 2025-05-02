# include "elixir.h"

static int Dynamic;
static int Reload;

static int      Nobjects;
static int      Nprocess;
static Process **process;
static Process   global;

int main (int argc, char **argv) {
  
  int i, Nnew, state;
  char *config;
  int nloop;
  char **targv;

  /* save complete arglist incase we reload */
  ALLOCATE (targv, char *, argc + 1);
  for (i = 0; i < argc; i++) {
    targv[i] = strcreate (argv[i]);
  }
  targv[i] = (char *) NULL;

  config = ConfigInit (&argc, argv);

  process = DefineProcesses (&global, &Nprocess, config);

  /* PID filename stored in global.argv[7] */
  ConfigPID (global.argv[7]);

  InitMsgFile (global.argv[0]); /* success */
  InitMsgFile (global.argv[1]); /* failure */
  InitMsgFile (global.argv[5]); /* message */
  InitMsgFile (global.argv[6]); /* end stat */
  InitMachines (config);
  free (config);

  SetMessageFile (global.argv[5]);

  InitProcessTimers (process, Nprocess);

  Nobjects = 0;
  Reload = FALSE;
  Dynamic = TRUE;
  nloop = 0;

  /* any remaining entry is a file to load in static mode */
  if (argc == 2) {
    Dynamic = FALSE;
    Nobjects = LoadPending (&global, argv[1], &state, &Dynamic);
    if (state) {
      fprintf (stderr, "error with input file %s (state %d)\n", argv[1], state);
      Shutdown (1);
    }
  }

  while (CheckEndingState (&global, Nobjects, Dynamic)) {

    if (Dynamic) { 
      Nnew = LoadPending (&global, global.argv[4], &state, &Dynamic);
      Nobjects += Nnew;
    }

    for (i = 0; i < Nprocess; i++) {
      CheckProcess (process[i]);
    }

    for (i = Nprocess - 1; i >= 0; i--) {
      CheckProcess (process[i]);
    }

    CheckMessages ();
    /* CheckMessages (&global, process, Nprocess, &Dynamic, Nobjects); */
    RestartMachines ();

    usleep (1000000);
    if (nloop > 3000) {
      fprintf (stderr, ".");
      nloop = 0;
    }
    nloop ++;
  }

  DumpProcessTimes (global.argv[6]);
 
  /* DumpStatus (global.argv[6], &global, process, Nprocess, Dynamic, Nobjects);  */
  DumpStatus (global.argv[6]);

  WriteMsg (global.argv[6], "DONE");

  if (Reload) Restart (targv);

  Shutdown (0);
  exit (0);
}

void SIG_RELOAD (int sig) {
  fprintf (stderr, "got signal RELOAD\n");
  SetExitTimer ();
  Dynamic = FALSE;
  Reload = TRUE;
}

void ElixirStop () {
  Dynamic = FALSE;
}

int GetDynamicState () {
  return (Dynamic);
}

Process **GetProcessInfo (Process **gb, int *np, int *no) {
  *np = Nprocess;
  *no = Nobjects;
  *gb = &global;
  return (process);
}

/* create 'reload' mode:
   0 - Reload = TRUE;
   1 - Dynamic = FALSE;
   2 - start timer (in CheckEndingState.c)
   3 - if timer expires, set exit state
   4 - if (Reload) exec (argc, arg)
*/
