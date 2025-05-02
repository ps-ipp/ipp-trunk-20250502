# include "checkastro.h"

// checkastro_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// there are four modes:

// checkastro_client -load-objects : extract the bright catalog subset from the client's
//                                 tables and save to a local FITS table for checkastro to
//                                 load and used

int main (int argc, char **argv) {

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, etc) 
  initialize_client (argc, argv);

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  // USAGE: checkastro_client -load-objects
  int Ncatalog;
  Catalog *catalog = load_catalogs (skylist, &Ncatalog, TRUE, HOST_ID, HOSTDIR);
  if (!catalog) {
    fprintf (stderr, "ERROR loading catalogs from %s\n", CATDIR);
    exit (2);
  }

  BrightCatalog *bcatalog = BrightCatalogMerge (catalog, Ncatalog);
  if (!BrightCatalogSave (BCATALOG, bcatalog)) {
    fprintf (stderr, "ERROR saving bright catalog from %s\n", CATDIR);
    exit (2);
  }

  exit (0);
}
