# include "addstar.h"
# include "loadICRF.h"

int main (int argc, char **argv) {

  AddstarClientOptions options;

  // need to construct these options with args_loadICRF...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadICRF_client (&argc, argv, options);

  // client is called with a pointer to the file to be loaded

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  SkyList *skylist = SkyRegionByCPT (sky, CPT_FILE);

  char cptfile[1024];
  snprintf (cptfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[0]->name);

  int Nstars;
  ICRF_Stars *stars = loadICRF_load_stars (INPUT, &Nstars);

  loadICRF_catalog (stars, Nstars, skylist->regions[0], cptfile, &options);

  free (stars);

  exit (0);
}
