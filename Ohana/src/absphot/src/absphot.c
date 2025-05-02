# include "relphot.h"

int main (int argc, char **argv) {

  // select the list of database region files, based on region
  // loop over the catalogs, load the magnitudes in the selected bands
  // determine the fit

  int i, status, Ncatalog;
  Catalog *catalog;
  FITS_DB db;

  SkyList *skylist = NULL;

  /* get configuration info, args */
  initialize (argc, argv);

  /* register database handle with shutdown procedure */
  set_db (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);

  /* load regions based on specified sky patch */
  skylist = load_regions (&db, &UserPatch, UserPatchSelect);

  /* load catalog data from region files */
  catalog = load_catalogs (skylist, &Ncatalog);
  
  fitTrends (catalog);

}
