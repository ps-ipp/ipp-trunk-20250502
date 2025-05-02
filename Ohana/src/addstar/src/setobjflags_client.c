# include "addstar.h"
# include "setobjflags.h"

int main (int argc, char **argv) {


  // need to construct these options with args_loadstarpar...
  SetSignals ();
  ConfigInit_setobjflags (&argc, argv);
  args_setobjflags_client (&argc, argv);

  // client is called with a pointer to the file to be loaded

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  SkyList *skylist = SkyRegionByCPT (sky, CPT_FILE);

  int Nstars;
  MyStars *stars = setobjflags_load_stars (INPUT, &Nstars);

  char filename[DVO_MAX_PATH];
  snprintf (filename, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR, CPT_FILE);

  setobjflags_catalog (stars, Nstars, skylist->regions[0], filename);

  free (stars);

  FreeConfig ();
  FreePhotcodeTable ();
  SkyListFree (skylist);
  SkyTableFree (sky);

  free (CPT_FILE);
  free (HOSTDIR);
  free (INPUT);

  ohana_memcheck(TRUE);
  ohana_memdump(TRUE);
  exit (0);
}
