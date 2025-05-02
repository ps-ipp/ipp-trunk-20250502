# include "imregister.h"
# include "imphot.h"

void ModifySubset (FITS_DB *db, Image *image, off_t Nimage, off_t *match, off_t Nmatch) {

  off_t i, j;

  /* modify the selected entries */
  for (j = 0; j < Nmatch; j++) {

    i = match[j];

    if (options.modify) {
      if (!strcasecmp (options.ModifyEntry, "and")) {
	image[i].flags &= atoi (options.ModifyValue);
      }
      if (!strcasecmp (options.ModifyEntry, "or")) {
	image[i].flags |= atoi (options.ModifyValue);
      }
      if (!strcasecmp (options.ModifyEntry, "xor")) {
	image[i].flags ^= atoi (options.ModifyValue);
      }
      if (!strcasecmp (options.ModifyEntry, "=")) {
	image[i].flags = atoi (options.ModifyValue);
      }
    }
  }

  /** we may later want to pull this out and put it elsewhere **/
  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, match, Nmatch);
  for (i = 0; i < Nmatch; i++) {
    fprintf (stderr, "need to work with real output format\n");
    exit (2);
    // gfits_convert_Image ((Image *) db[0].vtable.buffer[i], sizeof (Image), 1);
  }
  gfits_db_update (db);
  gfits_db_close (db);
  gfits_db_free (db);

  exit (0);
}
