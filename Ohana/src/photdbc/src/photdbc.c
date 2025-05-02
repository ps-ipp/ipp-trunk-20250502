# include "photdbc.h"

int main (int argc, char **argv) {

  SkyTable *sky;
  SkyList *skylist;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize (argc, argv);

  // the output catalog needs to inherit the SKY_DEPTH of the input catalog
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, 0, VERBOSE);
  if (!sky) {
      fprintf (stderr, "ERROR loading sky table from %s\n", CATDIR);
      exit (2);
  }

  SkyTableSetFilenames (sky, CATDIR, "cpt");
  skylist = SkyListByPatch (sky, -1, &REGION);

  // load and copy the image table
  if (!SKIP_IMAGES) {
    copy_images (argv[1], skylist);
  }

  // hostID is 0 for master program
  if (!ONLY_IMAGES) {
    photdbc_catalogs (argv[1], skylist, 0);
  }

  char *skyfile = SkyTableFilename (argv[1]);
  SkyTableSave (sky, skyfile);
  exit (0);
}
