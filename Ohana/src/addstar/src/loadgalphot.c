# include "addstar.h"
# include "loadgalphot.h"

/* This is the DVO program to upload fullforce summary cmf data for galaxies into a DVO database.

   USAGE: loadgalphot -D CATDIR (catdir) (file.cmf)
*/

int main (int argc, char **argv) {

  int i;
  AddstarClientOptions options;

  // need to construct these options with args_loadgalphot...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadgalphot (&argc, argv, options);

  // load the full sky description table (dvodb must exist)
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // generate the subset matching the user-selected region
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  for (i = 1; i < argc; i++) {
    fprintf (stderr, "loading %s\n", argv[i]);
    loadgalphot_table (skylist, NULL, argv[i], &options);
  }
  exit (0);
}  
