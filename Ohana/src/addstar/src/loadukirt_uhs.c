# include "addstar.h"
# include "ukirt_uhs.h"

/* This is the DVO program to upload UKIRT_UHS detections from cvs files supplied by Mike
   Read into a DVO database.  It is modeled on the loadgaia_dr2 program with column modifications
   determined by Zhoujian Zhang

   USAGE: loadukirt_uhs -D CATDIR (catdir) (ukirt_uhsfile) [...more files]
*/

int main (int argc, char **argv) {

  SkyTable *sky;
  SkyList *skylist = NULL;
  AddstarClientOptions options;

  // need to construct these options with args_loadtycho...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadukirt_uhs (&argc, argv, options);

  // load the full sky description table:
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // generate the subset matching the user-selected region
  skylist = SkyListByPatch (sky, -1, &UserPatch);

  loadukirt_uhs_table (skylist, argv[1], &options);

  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  free (CATDIR);

  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
  exit (0);
}  

