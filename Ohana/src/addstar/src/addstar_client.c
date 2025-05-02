# include "addstar.h"

// addstar_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// there is one mode:

// addstar_client -resort : update the table indicies

int main (int argc, char **argv) {

  AddstarClientOptions options;

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, etc) 
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_parallel_client (argc, argv, options);

  // load the current sky table (layout of all SkyRegions) 
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, -1, VERBOSE);
  if (!sky) {
      fprintf (stderr, "ERROR running loading sky table from %s\n", CATDIR);
      exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  if (options.mode == ADDSTAR_MODE_RESORT) {
    resort_catalogs (&options, sky);
    exit (0);
  }

  fprintf (stderr, "unknown mode for addstar_client\n");
  exit (1);
}
