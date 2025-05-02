# include "imregister.h"
# include "spreg.h"

void ModifySubset (FITS_DB *db, Spectrum *spectrum, off_t Nspectrum, off_t *match, off_t Nmatch) {

  off_t i, j, Nold;
  char *tmppath;

  Nold = 0;
  tmppath = NULL;

  /* create some necessary variables */
  if (output.modify_path) { 
    Nold = strlen (output.oldpath);
    ALLOCATE (tmppath, char, 128);
  }

  /* modify the selected entries */
  for (j = 0; j < Nmatch; j++) {

    i = match[j];

    if (output.modify_path) {
      if (!strncmp (spectrum[i].pathname, output.oldpath, Nold)) {
	strcpy (tmppath, &spectrum[i].pathname[Nold]);
	snprintf (spectrum[i].pathname, 64, "%s%s", output.newpath, tmppath);
      }
    }

    if (output.modify_mode) {
      spectrum[i].mode = output.mode;
    }

    if (output.modify_state) {
      spectrum[i].state = output.state;
    }
  }

  /** we may later want to pull this out and put it elsewhere **/
  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, match, Nmatch);
  for (i = 0; i < Nmatch; i++) {
    gfits_convert_Spectrum ((Spectrum *) db[0].vtable.buffer[i], sizeof (Spectrum), 1);
  }
  gfits_db_update (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
