# include "imregister.h"
# include "imphot.h"

static char *version = "imphotsearch $Revision: 1.10 $";

int main (int argc, char **argv) {
 
  off_t Nmatch, Nimage;
  off_t *match;
  Image *image;
  FITS_DB db;

  get_version (argc, argv, version);
  args (argc, argv);
 
  gfits_db_init (&db);
  db.lockstate = (options.modify) ? LCK_HARD : LCK_SOFT;
  db.timeout   = 300.0;

  /* don't create a new image table if not available */
  if (!gfits_db_lock (&db, ImPhotDB)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  /* we use a varient of gfits_db_load since the file may be in text format */
  if (!dvo_image_load (&db, VERBOSE, FORCE_READ)) {
    fprintf (stderr, "ERROR: failure to load db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (!options.modify) gfits_db_close (&db);

  image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  match = subset (image, Nimage, &Nmatch);
  if (options.modify) ModifySubset (&db, image, Nimage, match, Nmatch);

  output (image, match, Nmatch);

  if (VERBOSE) fprintf (stderr, "SUCCESS\n");
  exit (0);
}

