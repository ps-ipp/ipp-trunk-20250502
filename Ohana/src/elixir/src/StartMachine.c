# include "elixir.h"

int StartMachine (Machine *machine, Object *object, int argc, char **argv) {

  int i, Nbyte, Nout;
  char command[1024];
  FILE *logfile;

  logfile = LogOpen (object[0].logfile);
  machine[0].object = object;

  Nbyte = Nout = 0;
  for (i = 0; i < argc; i++) {
    Nout = sprintf (&command[Nbyte], "%s ", argv[i]);
    Nbyte += Nout;
  }
    
  fprintf (logfile, "%s @ %s: %s\n", object[0].argv[0], machine[0].hostname, command);
  if (logfile != stderr) {
    fflush (logfile);
    fclose (logfile);
  }

  Nout = sprintf (&command[Nbyte], "\n echo PROCESS DONE\n");
  if (write (machine[0].wsock, command, strlen(command)) != strlen(command)) return (FALSE);

  gettimeofday (&machine[0].object[0].start, (void *) NULL);
  gettimeofday (&machine[0].quiet, (void *) NULL);

  machine[0].status = BUSY;
  return (TRUE);
}

/* machine[0].quiet times since last message received from process. 
   if process is too quiet for too long, we assume it died */
