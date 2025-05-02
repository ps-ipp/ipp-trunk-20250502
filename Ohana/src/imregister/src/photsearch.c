# include "imregister.h"
# include "photreg.h"
static char *version = "photsearch $Revision: 1.9 $";

int main (int argc, char **argv) {

  char *filename;
  off_t Nmatch, Nphotpars;
  off_t *match;
  FITS_DB db;
  PhotPars *photpars;
  PhotParsOld *photpars_old;

  get_version (argc, argv, version);
  args (argc, argv);

  gfits_db_init (&db);
  db.lockstate = (output.modify || output.delete) ? LCK_HARD : LCK_SOFT;
  db.timeout   = 300.0;

  if (!strcmp (output.db, "phot")) {
    filename = PhotDB;
  } else {
    filename = TransDB;
  }

  if (!gfits_db_lock (&db, filename)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  
  if (!gfits_db_load (&db)) {
    fprintf (stderr, "ERROR: failure to load db\n");
    gfits_db_close (&db);
    exit (1);
  }

  if (!output.modify && !output.delete) gfits_db_close (&db);

  /* add test to EXTNAME? */
  if (output.convert) {
    photpars_old = gfits_table_get_PhotParsOld (&db.ftable, &Nphotpars, &db.scaledValue, &db.nativeOrder);
    if (!photpars_old) {
      fprintf (stderr, "ERROR: failed to read photometry parameters\n");
      exit (2);
    }
    photpars = PhotParsOld_to_PhotPars (photpars_old, Nphotpars);
  } else {
    photpars = gfits_table_get_PhotPars (&db.ftable, &Nphotpars, &db.scaledValue, &db.nativeOrder);
    if (!photpars) {
      fprintf (stderr, "ERROR: failed to read photometry parameters\n");
      exit (2);
    }
  }

  match = match_criteria (photpars, Nphotpars, &Nmatch);

  if (output.delete) DeleteSubset (&db, photpars, Nphotpars, match, Nmatch);

  OutputSubset (photpars, Nphotpars, match, Nmatch);
  exit (0);
}

  /* db selection is set in args, based on -trans */

/* valid EXTNAME values:

   phot, !output.convert, bintable: "ZERO_POINTS_3.0"
   phot, !output.convert, table:    "IMAGE_ZPTS"

   trans, !output.convert, bintable: "TRANS_POINTS_3.0"
   trans, !output.convert, table:    "SUMMARY_ZPTS"

   phot, !output.convert, bintable: "ZERO_POINTS"
   phot, !output.convert, table:    "IMAGE_ZPTS"

   trans, !output.convert, bintable: "TRANS_POINTS"
   trans, !output.convert, table:    "SUMMARY_ZPTS"

*/
