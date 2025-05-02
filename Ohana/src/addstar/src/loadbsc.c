# include "addstar.h"
# include "bsc.h"

/* This is the DVO program to upload bsc detections into a DVO database.  It is modeled on
   the loadtycho program.  The bsc data are delivered as a text file distiled from the
   vizier file (see doc/yale_BSC_vizier and doc/run.awk.bsc).  

   USAGE: loadbsc -D CATDIR (catdir) (bscfile)

*/

int main (int argc, char **argv) {

  SkyTable *sky;
  SkyList *skylist = NULL;
  AddstarClientOptions options;

  // need to construct these options with args_loadbsc...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadbsc (&argc, argv, options);

  // load the full sky description table:
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // generate the subset matching the user-selected region
  skylist = SkyListByPatch (sky, -1, &UserPatch);

  // if we only match to existing (already populated) regions, limit the select to those regions:
  if (options.existing_regions) {
    SkyList *tmp;
    tmp = SkyListExistingSubset (skylist, CATDIR);
    SkyListFree (skylist);
    skylist = tmp;
  }

  fprintf (stderr, "loading %s\n", argv[1]);
  loadbsc_rawdata (skylist, argv[1], options);

  exit (0);
}  

