# include "addstar.h"
# include "gaia_dr2.h"

/* This is the DVO program to upload gaia_dr2 detections from gaia cvs files into a DVO database.
   It is modeled on the loadgaia program but the source files are CSV tables.

   USAGE: loadgaia_dr2 -D CATDIR (catdir) (gaia_dr2file) [...more files]
*/

int main (int argc, char **argv) {

  SkyTable *sky;
  SkyList *skylist = NULL;
  AddstarClientOptions options;

  // need to construct these options with args_loadtycho...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadgaia_dr2 (&argc, argv, options);

  // load the full sky description table:
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // generate the subset matching the user-selected region
  skylist = SkyListByPatch (sky, -1, &UserPatch);

  int Nstart = 1;
  int Nend = argc;
  while (Nstart < Nend) {
    Nstart = loadgaia_dr2_table (Nstart, Nend, skylist, NULL, argv, &options);
  }

  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  free (CATDIR);

  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
  exit (0);
}  

