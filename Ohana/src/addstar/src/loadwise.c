# include "addstar.h"
# include "WISE.h"

/* This is the DVO program to upload WISE detections into a DVO database.
   It is modeled on the load2mass program, but unlike that case, it is expected to be run only
   rarely (once?).  The WISE data are delivered as *.bz2 files.  This program assumes the files
   have been decompressed.  It does not allow a subset of the sky to be uploaded; entire WISE
   files are loaded if supplied on the command line.

   USAGE: loadwise -D CATDIR (catdir) (wisefile) [...more files]

 */

int main (int argc, char **argv) {

  int i;
  SkyTable *sky;
  SkyList *skylist = NULL;
  AddstarClientOptions options;

  // need to construct these options with args_loadWISE...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadwise (&argc, argv, options);

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

  for (i = 1; i < argc; i++) {
      fprintf (stderr, "loading %s\n", argv[i]);
      loadwise_rawdata (skylist, argv[i], options);
  }
  exit (0);
}  

