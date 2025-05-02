# include "dvomerge.h"

int dvorepair_by_objID (int argc, char **argv) {

  if (argc != 2) {
    fprintf (stderr, "USAGE: dvorepair -fix-cpt-by-objID (cptfile)\n");
    exit (2);
  }

  char *cptFile = argv[1];

  Catalog catalog;

  // set the parameters which guide catalog open/load/create
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename  = cptFile;

  // always load all of the data (if any exists)
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ | DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;
  catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
  catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
  
  if (!dvo_catalog_open (&catalog, NULL, VERBOSE, "w")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", cptFile);
    exit (2);
  }

  repair_catalog_by_objID (&catalog);

  if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save catalog %s\n", catalog.filename); exit (1); }
  if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock catalog %s\n", catalog.filename); exit (1); }
  
  return (TRUE);
}

