# include "dvolens.h"

int main (int argc, char **argv) {

  // get configuration info, args
  SetSignals ();
  DvoLensMode mode = initialize (argc, argv);
  if (!mode) exit (2);

  switch (mode) {
    case MODE_UPDATE_OBJECTS:
      update_objects ();
      FREE (UserCatalog);
      FREE (CATDIR);
      free_images();
      FreeWarpGroups();
      ohana_memcheck (VERBOSE);
      ohana_memdump (VERBOSE);
      fprintf (stderr, "done with dvolens\n");
      exit (0);

    default:
      fprintf (stderr, "ERROR: no valid dvolens mode chosen\n");
      exit (2);
  }
  fprintf (stderr, "IMPOSSIBLE: skipped out of switch?\n");
  exit (1);
}
