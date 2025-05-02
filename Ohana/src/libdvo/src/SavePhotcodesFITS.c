# include <dvo.h>

/* this function saves the FITS photcode file into the internal photcode table */
/* locking is used to avoid collisions with programs trying to update the photcodes values */
/* XXX better distinction between NOT FOUND and FAILURE */

int SavePhotcodesFITS (char *filename) {

  PhotCodeData *table = NULL;
  FITS_DB db;

  table = GetPhotcodeTable ();
  if (table == NULL) {
    fprintf (stderr, "ERROR: no internal photcode table is defined\n");
    return FALSE;
  }

  /* XXX choose more sensible lock timeouts! */
  gfits_db_init (&db);
  db.lockstate = LCK_XCLD;
  db.timeout   = 10.0;

  /* does this mean the db is empty, non-existent, or has access errors? */
  if (!gfits_db_lock (&db, filename)) {
    fprintf (stderr, "ERROR: failure to lock db, cannot save photcode table to %s\n", filename);
    gfits_db_close (&db);
    return FALSE;
  }

  // for the moment, we simply support the latest photcode format for output
  // XXX update this as needed as new formats are defined
  PhotCode_PS1_V6 *photcode_output = PhotCode_Internal_To_PS1_V6 (table[0].code, table[0].Ncode);

  /* convert FITS format data to internal format (byteswaps & EXTNAME) */
  if (!gfits_db_create (&db)) return (FALSE);
  if (!gfits_table_set_PhotCode_PS1_V6 (&db.ftable, photcode_output, table[0].Ncode, TRUE)) return (FALSE);
  if (!gfits_db_save (&db)) return (FALSE);
  if (!gfits_db_close (&db)) return (FALSE);
  if (!gfits_db_free (&db)) return (FALSE);

  free (photcode_output);

  return TRUE;
}
