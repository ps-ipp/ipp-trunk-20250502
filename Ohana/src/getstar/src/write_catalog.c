# include "getstar.h"

int write_catalog (Catalog *catalog) {    

  /* write out the selected stars */
  // XXX need to set the catalog boundaries by hand? RA0-RA1, DEC0-DEC1
  SetProtect (TRUE);
  if (!dvo_catalog_save (catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog->filename); exit (1); }
  if (!dvo_catalog_unlock (catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog->filename); exit (1); }
  SetProtect (FALSE);
  dvo_catalog_free (catalog);
  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
