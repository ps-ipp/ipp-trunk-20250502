# include "relastro.h"

void save_catalogs (Catalog *catalog, int Ncatalog) {

  int i;

  /* load data from each region file */
  for (i = 0; i < Ncatalog; i++) {

    if (VERBOSE2) fprintf (stderr, "saving catalog %s\n", catalog[i].filename);
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog[i], VERBOSE2)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog[i].filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog[i])) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog[i].filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog[i]);
  }
}
