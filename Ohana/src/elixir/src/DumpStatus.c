# include "elixir.h"

int DumpStatus (char *filename) {

  FILE *f;
  int i, state, total, mode;
  Process *global, **process;
  int Nprocess, Nobject, Dynamic;

  process = GetProcessInfo (&global, &Nprocess, &Nobject);
  Dynamic = GetDynamicState ();

  if (filename == (char *) NULL) {
    if (system ("tput clear") == -1) {
      fprintf (stderr, "\n\n\n");
    }
    f = stderr;
  } else {
    /* lock file */
    f = fsetlockfile (filename, 0.1, LCK_XCLD, &state);
    if (f == NULL) return (2);
    fseeko (f, 0, SEEK_END);
  }  
  
  fprintf (f, "processes status:\n");
  fprintf (f, "            name   pending  success  failure  total\n");
  fprintf (f, "----------------------------------------------------------------\n");
  total = global[0].pending[0].Nobject + global[0].success[0].Nobject + global[0].failure[0].Nobject;
  fprintf (f, "%16s    %6d   %6d   %6d   %4d\n", "global", 
	   global[0].pending[0].Nobject, global[0].success[0].Nobject, global[0].failure[0].Nobject, total); 
  for (i = 0; i < Nprocess; i++) {
    fprintf (f, "%16s    %6d\n", process[i][0].name, process[i][0].pending[0].Nobject); 
  }
  fprintf (f, "\n");

  DumpMachineStatus (f);

  fprintf (f, "\n");
  fprintf (f, "objects loaded so far: %d\n\n", Nobject);
  if (Dynamic) {
    fprintf (f, "accepting input from FIFO\n");
  } else {
    fprintf (f, "NOT accepting input from FIFO\n");
  }    
  
  if (f != stderr) {
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    chmod (filename, mode);
    fclearlockfile (filename, f, LCK_XCLD, &state);
  }
  return (TRUE);
}

/*

processes status:
            name  pending  success  failure  total
----------------------------------------------------------------
          global        1        0        4      5
           mkdir        1
         flatten        0
          dophot        0
         imclean        0
          gastro        0
         addstar        0
          rmfile        5

machine status:
          name  process  status
----------------------------------------------------------------
         kiawe  rmfile   1
         kiawe  addstar  9
          milo  addstar  1

objects loaded so far: 13

accepting input from FIFO


*/
