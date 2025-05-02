# include "fakeastro.h"

int main (int argc, char **argv) {

  initialize_client (argc, argv);

  // client is called with a pointer to the file to be loaded

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  SkyList *skylist = SkyRegionByCPT (sky, CPT_FILE);

  int Nstars;
  FakeAstro_Stars *stars = fakestar_load_stars (INPUT, &Nstars);

  fakestar_catalog (stars, Nstars, skylist->regions[0], CPT_FILE);

  free (stars);

  exit (0);
}
