# include "dvocompress.h"

int main (int argc, char **argv) {

  SetSignals ();
  args (&argc, argv);

  char *CATDIR = argv[1];

  // the output catalog needs to inherit the SKY_DEPTH of the input catalog
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, 0, VERBOSE);
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
  dvocompress_catalogs (CATDIR, skylist, 0);
  exit (0);
}
