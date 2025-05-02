# include "imregister.h"

/* load the rest of the db table into memory (first extension only) */
int rfits (FITS_DB *db) {

  /* database name must be set first */
  if (db == NULL) {
    fprintf (stderr, "db handle is not set\n");
    return (FALSE);
  }
  if (!gfits_fread_matrix (db[0].f, &db[0].matrix, &db[0].header)) {
    fprintf (stderr, "can't read primary matrix");
    return (FALSE);
  }
  if (!gfits_fread_header (db[0].f, &db[0].theader)) {
    fprintf (stderr, "can't read table header");
    return (FALSE);
  }
  if (!gfits_fread_ftable_data (db[0].f, &db[0].ftable, FALSE)) {
    fprintf (stderr, "can't read table data");
    return (FALSE);
  }
  return (TRUE);
}

