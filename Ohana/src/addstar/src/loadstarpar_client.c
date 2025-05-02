# include "addstar.h"
# include "loadstarpar.h"

int main (int argc, char **argv) {

  AddstarClientOptions options;

  // need to construct these options with args_loadstarpar...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadstarpar_client (&argc, argv, options);

  // client is called with a pointer to the file to be loaded

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  SkyList *skylist = SkyRegionByCPT (sky, CPT_FILE);

  int Nstars;
  StarPar_Stars *stars = loadstarpar_load_stars (INPUT, &Nstars);

  char filename[DVO_MAX_PATH];
  snprintf (filename, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR, CPT_FILE);

  loadstarpar_catalog (stars, Nstars, skylist->regions[0], filename, &options);

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
