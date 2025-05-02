# include "dvolens.h"

// dvolens has the following modes:
// * -update-objects

DvoLensMode initialize (int argc, char **argv) {

  dvolens_help (argc, argv);
  ConfigInit (&argc, argv);
  DvoLensMode mode = args (argc, argv);
  if (!mode) exit (2);

  return mode;
}

void initialize_client (int argc, char **argv) {

  // XXX need to determine which globals can affect relphot_client in either mode and pass appropriately

  dvolens_client_help (argc, argv);
  ConfigInit (&argc, argv);
  args_client (argc, argv);
}
