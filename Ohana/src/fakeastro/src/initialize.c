# include "fakeastro.h"

void usage (void);
void usage_client (void);

void initialize (int argc, char **argv) {

  if (argc == 1) usage();
  if (get_argument (argc, argv, "-h")) usage();
  if (get_argument (argc, argv, "--h")) usage();
  if (get_argument (argc, argv, "-help")) usage();
  if (get_argument (argc, argv, "--help")) usage();

  args (&argc, argv);
  ConfigInit (&argc, argv);
  if (argc != 1) usage ();

  // XXX add to config?
  if (!InitGalaxyModel (GALAXY_MODEL)) {
    fprintf (stderr, "failed to init galaxy model %s\n", GALAXY_MODEL);
    exit (2);
  }
}

void initialize_client (int argc, char **argv) {

  args_client (&argc, argv);
  ConfigInit (&argc, argv);
  if (argc != 1) usage_client ();

  // XXX add to config?
  if (!InitGalaxyModel (GALAXY_MODEL)) {
    fprintf (stderr, "failed to init galaxy model %s\n", GALAXY_MODEL);
    exit (2);
  }
}

