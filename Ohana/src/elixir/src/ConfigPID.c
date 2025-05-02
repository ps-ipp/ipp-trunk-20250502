# include "elixir.h"

static char *PIDMaster = (char *) NULL;

void ConfigPID (char *PIDFile) {

  pid_t pid;
  char *username, machine[256];
  FILE *f;

  f = fopen (PIDFile, "r");
  if (f == (FILE *) NULL) { 

    pid = getpid ();
    username = getenv ("USER");
    if (username == (char *) NULL) {
      fprintf (stderr, "error getting username\n");
      exit (2);
    }
    bzero (machine, 256);
    if (gethostname (machine, 256)) {
      fprintf (stderr, "error getting hostname\n");
      exit (2);
    }

    f = fopen (PIDFile, "w");
    if (f == (FILE *) NULL) { 
      fprintf (stderr, "can't write to PID file %s\n", PIDFile);
      exit (2);
    }

    fprintf (f, "PID:     %d\n", pid);
    fprintf (f, "USER:    %-s\n", username);
    fprintf (f, "MACHINE: %-s\n", machine);
    fclose (f);

    PIDMaster = PIDFile;
    return; 
  }

  ALLOCATE (username, char, 256);
  if (fscanf (f, "%*s %d", &pid) != 1) fprintf (stderr, "error reading pid\n");
  if (fscanf (f, "%*s %s", username) != 1) fprintf (stderr, "error reading pid\n");
  if (fscanf (f, "%*s %s", machine) != 1) fprintf (stderr, "error reading pid\n");
  fclose (f);

  fprintf (stderr, "elixir is apparently running:\n\n");
  fprintf (stderr, "  machine: %s\n", machine);
  fprintf (stderr, "  user: %s\n", username);
  fprintf (stderr, "  PID: %d\n", pid);
  fprintf (stderr, "  remove %s if elixir has died unexpectedly\n", PIDFile);
  Shutdown (1);
}

void RemovePID () {
  
  if (PIDMaster == (char *) NULL) 
    return;
  
  if (unlink (PIDMaster)) {
    fprintf (stderr, "error deleting PID File %s\n", PIDMaster);
  }   
}
