# include "delstar.h"

int delete_photcodes_single (char *cptname) {

  Catalog catalog;

  int Nphotcodes = 0;
  PhotCode **photcodes = ParsePhotcodeList (PHOTCODE_LIST, &Nphotcodes, FALSE);

  dvo_catalog_init (&catalog, TRUE);
  catalog.filename  = cptname;
  catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

  if (VERBOSE) fprintf (stderr, "deleting from %s\n", catalog.filename);

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, NULL, VERBOSE2, "a")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
    return FALSE;
  }
  if (!catalog.Naverage_disk) {
    if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    return FALSE;
  }

  delete_photcodes_catalog (&catalog, photcodes, Nphotcodes);
  SetProtect (TRUE);
  dvo_catalog_save_complete (&catalog, VERBOSE2);
  dvo_catalog_unlock (&catalog);
  SetProtect (FALSE);
  dvo_catalog_free (&catalog);
  return TRUE;
}
