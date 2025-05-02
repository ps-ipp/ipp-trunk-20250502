# include "photdbc.h"

int main (int argc, char **argv) {

  SetSignals ();
  initialize_client (argc, argv);

  // load the current sky table (layout of all SkyRegions) 
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  if (!sky) {
      fprintf (stderr, "ERROR loading sky table from %s\n", CATDIR);
      exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  SkyList *skylist = SkyListByPatch (sky, -1, &REGION);
  if (!skylist) {
      fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
      exit (2);
  }
  
  // hostID is 0 for master program
  photdbc_catalogs (argv[1], skylist, HOST_ID);

  exit (0);
}
