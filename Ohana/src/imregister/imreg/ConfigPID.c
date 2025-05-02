# include "imregister.h"
# include "imreg.h"

static char *PIDMaster = (char *) NULL;

int ConfigPID (char *pidfile) {

  pid_t pid;
  char *username, machine[256];
  FILE *f;

  ALLOCATE (username, char, 256);
  if (!LoadPID (pidfile, &pid, username, machine)) {
    /* no PID file, make new one */

    pid = getpid ();

    free (username);
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

    f = fopen (pidfile, "w");
    if (f == (FILE *) NULL) { 
      fprintf (stderr, "can't write to PID file %s\n", pidfile);
      exit (2);
    }

    fprintf (f, "PID:     %d\n", pid);
    fprintf (f, "USER:    %-s\n", username);
    fprintf (f, "MACHINE: %-s\n", machine);
    fclose (f);

    PIDMaster = pidfile;
    return (TRUE); 
  }

  /* PID file exists, warn & exit */
  fprintf (stderr, "elixir is apparently running:\n\n");
  fprintf (stderr, "  machine: %s\n", machine);
  fprintf (stderr, "  user: %s\n", username);
  fprintf (stderr, "  PID: %d\n", pid);
  fprintf (stderr, "  remove %s if elixir has died unexpectedly\n", pidfile);
  Shutdown (1);
  return (FALSE); 
}

void RemovePID () {
  
  if (PIDMaster == (char *) NULL) 
    return;
  
  if (unlink (PIDMaster)) {
    fprintf (stderr, "error deleting PID File %s\n", PIDMaster);
  }   
}

int LoadPID (char *file, pid_t *pid, char *username, char *machine) {

  int t1, t2, t3;
  FILE *f;

  f = fopen (file, "r");
  if (f == (FILE *) NULL) { 
    return (FALSE);
  }

  t1 = fscanf (f, "%*s %d", pid);
  t2 = fscanf (f, "%*s %s", username);
  t3 = fscanf (f, "%*s %s", machine);
  if ((t1 != 1) || (t2 != 2) || (t3 != 1)) {
    fprintf (stderr, "error reading pid info\n");
  }
  fclose (f);

  return (TRUE);
}

int Shutdown (int status) {

  RemovePID ();
  exit (status);
}
