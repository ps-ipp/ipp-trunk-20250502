# include "checkastro.h"

int main (int argc, char **argv) {

  /* get configuration info, args */
  initialize (argc, argv);

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  checkastro_images (skylist);
  exit (0);
}
