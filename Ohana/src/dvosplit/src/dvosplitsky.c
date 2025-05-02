# include "dvosplit.h"

// dvosplitsky (catdir) -region Rmin Rmax Dmin Dmax
// subdivide the SkyTable by one level for the given region (L4 -> L5 only for now)
int main (int argc, char **argv) {

  SetSignals ();
  ConfigInit (&argc, argv);
  args (argc, argv);

  char *CATDIR = strcreate (argv[1]);

  // load the photcode table (for Nsecfilt and related)
  /* XXX should not be needed for dvosplitsky
  char photcodeFile[1024];
  snprintf (photcodeFile, 1024, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (photcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", photcodeFile);
    exit (1);
  }
  */

  // load the sky table for the existing database
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, SKY_DEPTH_HST, VERBOSE);
  if (!sky) {
    fprintf (stderr, "ERROR: failed to read sky table from %s\n", CATDIR);
    exit (2);
  }

  // for L4 entries in a certain ra,dec range, generate L5 entries:
  SkyTable L5; 
  L5.Nregions = 0;
  L5.Nalloc   = 1000;
  ALLOCATE (L5.regions, SkyRegion, L5.Nalloc);

  // get the list of populated regions at Level 4
  SkyList *skylist  = SkyListByPatch (sky, 4, &UserPatch);
  
  for (int i = 0; i < skylist->Nregions; i++) {
    SkyTableL5fromL4_List (skylist->regions[i], &L5, sky->Nregions);
  }

  // the third argument is the depth of the sky table.  it is used to set the
  // boolean which specifies that a table exists at that depth.  we assume that
  // a table does not since we are now splitting the table.  use -1 to prevent.
  SkyTableExtend (sky, &L5, -1);
  free (L5.regions);

  char *skytable_file = SkyTableFilename (CATDIR);
  int success = SkyTableSave (sky, skytable_file);

  if (!success) fprintf (stderr, "ERROR: failed to write updated sky table to %s\n", skytable_file);

  free (skytable_file);
  SkyTableFree (sky);
  SkyListFree (skylist);

  if (!success) exit (2);
  exit (0);
}
