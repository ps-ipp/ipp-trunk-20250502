# include "imregister.h"
# include "photreg.h"
static char *version = "photreg $Revision: 1.9 $";

int main (int argc, char **argv) {
 
  char *filename;
  PhotPars newdata;
  FITS_DB db;
  
  get_version (argc, argv, version);
  regargs (argc, argv, &newdata);

  gfits_db_init (&db);
  db.lockstate = LCK_HARD;
  db.timeout   = 300.0;

  filename = PhotDB;
  if (!strcmp (output.db, "trans")) filename = TransDB;

  if (!gfits_db_lock (&db, filename)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (db.dbstate == LCK_EMPTY) {
    gfits_db_create (&db);
    gfits_table_set_PhotPars (&db.ftable, NULL, 0, TRUE);
    /* EXTNAME is set to ZERO_POINTS_3.0 by default */
    if (!strcmp (output.db, "trans")) {
      gfits_modify (&db.theader, "EXTNAME", "%s", 1, "TRANS_POINTS_3.0");
    }
  } else {  
    if (!gfits_db_load (&db)) {
      fprintf (stderr, "ERROR: failure to load db\n");
      gfits_db_close (&db);
      exit (1);
    }
  }

  /** we may later want to pull this out and put it elsewhere **/
  gfits_convert_PhotPars (&newdata, sizeof (PhotPars), 1);
  gfits_table_to_vtable (&db.ftable, &db.vtable, 0, 0);
  gfits_vadd_rows (&db.vtable, (char *) &newdata, 1, sizeof(PhotPars));

  gfits_db_update (&db);
  gfits_db_close (&db);
  gfits_db_free (&db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}

  /* data values are set in args by matching flags */

/*** stick the header, table, theader, ftable creation in a single API? ***/
