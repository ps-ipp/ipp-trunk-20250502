# include "dvomerge.h"

int LoadCatalog (Catalog *catalog, SkyRegion *region, char *filename, char *mode, int Nsecfilt) {

  // set the parameters which guide catalog open/load/create
  dvo_catalog_init (catalog, TRUE);
  catalog[0].filename  = filename;
  catalog[0].Nsecfilt  = Nsecfilt;

  // always load all of the data (if any exists)

  catalog[0].catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;

  if (SKIP_MEASURE) {
    catalog[0].catflags = catalog[0].catflags | DVO_SKIP_MEASURE;
  } else {
    catalog[0].catflags = catalog[0].catflags | DVO_LOAD_MEASURE;
  }

  if (SKIP_MISSING) {
    catalog[0].catflags = catalog[0].catflags | DVO_SKIP_MISSING;
  } else {
    catalog[0].catflags = catalog[0].catflags | DVO_LOAD_MISSING;
  }

  if (SKIP_LENSING)  {
    catalog[0].catflags = catalog[0].catflags | DVO_SKIP_LENSING;
  } else {
    catalog[0].catflags = catalog[0].catflags | DVO_LOAD_LENSING;
  }

  if (SKIP_LENSOBJ)  {
    catalog[0].catflags = catalog[0].catflags | DVO_SKIP_LENSOBJ;
  } else {
    catalog[0].catflags = catalog[0].catflags | DVO_LOAD_LENSOBJ;
  }

  if (SKIP_STARPAR) {
    catalog[0].catflags = catalog[0].catflags | DVO_SKIP_STARPAR;
  } else {
    catalog[0].catflags = catalog[0].catflags | DVO_LOAD_STARPAR;
  }
  
  if (SKIP_GALPHOT)  {
    catalog[0].catflags = catalog[0].catflags | DVO_SKIP_GALPHOT;
  } else {
    catalog[0].catflags = catalog[0].catflags | DVO_LOAD_GALPHOT;
  }

  catalog[0].catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
  catalog[0].catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
  
  if (!dvo_catalog_open (catalog, region, VERBOSE, mode)) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", filename);
    exit (2);
  }
  return (TRUE);
}

int ResetStarPar (Catalog *catalog) {

  off_t i;

  // we are going to delete the starpar values:

  catalog[0].Nstarpar = 0;
  catalog[0].Nstarpar_disk = 0;
  catalog[0].Nstarpar_off = 0;

  REALLOCATE (catalog[0].starpar, StarPar, 1);

  for (i = 0; i < catalog[0].Naverage; i++) {
    catalog[0].average[i].Nstarpar = 0;
    catalog[0].average[i].starparOffset = -1;
  }
  return TRUE;
}

int ResetLensing (Catalog *catalog) {

  off_t i;

  // we are going to delete the lensing values:

  catalog[0].Nlensing = 0;
  catalog[0].Nlensing_disk = 0;
  catalog[0].Nlensing_off = 0;

  REALLOCATE (catalog[0].lensing, Lensing, 1);

  for (i = 0; i < catalog[0].Naverage; i++) {
    catalog[0].average[i].Nlensing = 0;
    catalog[0].average[i].lensingOffset = -1;
  }
  return TRUE;
}
