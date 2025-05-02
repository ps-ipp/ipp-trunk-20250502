# include "imregister.h"
# include "imreg.h"
static char *version = "imstatreg $Revision: 3.16 $";

int main (int argc, char **argv) {
 
  int child;
  off_t i, *match, Nmatch, Nimage, Nsubset;
  FILE *f;
  RegImage *image, *subset;
  FITS_DB image_db, temp_db;

  get_version (argc, argv, version);
  args (argc, argv);
  SetSignals ();

  if (CLIENT) imregclient (argv[1], argv[2], argv[3]);

  /* fork in background */
  child = fork ();
  if (child == -1) {
    fprintf (stderr, "error forking imstatreg -daemon \n");
    exit (1);
  } 
  if (child !=  0) {
    fprintf (stderr, "starting imstatreg, logging to %s\n", LogFile);
    exit (0);
  }

  /* child process, check for previous process */
  ConfigPID (PIDFILE);

  /* redirect stderr, stdout to logfile */
  f = freopen (LogFile, "a", stdout);
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open log file %s, writing to stderr\n", LogFile);
  } else {
    /* an error here will be missed, but is unlikely since we have access in the above test */
    f = freopen (LogFile, "a", stderr);
  }

  gfits_db_init (&image_db);
  image_db.lockstate = LCK_HARD;
  image_db.timeout   = 300.0;

  temp_db.lockstate = LCK_HARD;
  temp_db.timeout   = 300.0;
  gfits_db_init (&temp_db);

  /* start loop */
  while (1) {

    /* check / load / delete temporary database */
    if (!gfits_db_lock (&temp_db, TempDB)) {
      gfits_db_close (&temp_db);
      fprintf (stderr, "error locking temp db (path missing? access permission?)\n");
      exit (1);
    }
    if (temp_db.dbstate == LCK_EMPTY) {
      fprintf (stderr, "temporary database empty\n");
      gfits_db_close (&temp_db);
      goto next;
    } 
    if (!gfits_db_load (&temp_db)) {
      fprintf (stderr, "error reading temp db\n");
      exit (1);
    }
    subset = gfits_table_get_RegImage (&temp_db.ftable, &Nsubset, &temp_db.scaledValue, &temp_db.nativeOrder);
    if (!subset) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
    }
    fprintf (stderr, "temporary database read\n");

    /* delete, unlock existing database */
    if (truncate (TempDB, 0)) {
      fprintf (stderr, "error truncating temp db\n");
      exit (1);
    }
    gfits_db_close (&temp_db);
    fprintf (stderr, "temporary database closed\n");
 
    /* check / load main database */
    if (!gfits_db_lock (&image_db, ImageDB)) {
      gfits_db_close (&image_db);
      fprintf (stderr, "error locking image db (path missing? access permission?)\n");
      exit (1);
    }
    if (temp_db.dbstate == LCK_EMPTY) {
      fprintf (stderr, "main database empty\n");
      gfits_db_close (&image_db);
      gfits_db_free (&temp_db);
      goto next;
      /* this is a type of error: we read entries from the
	 temp db, but there were no entries to match in the main db
	 we will just drop the temp db data */
    }
    image = gfits_table_get_RegImage (&image_db.ftable, &Nimage, &image_db.scaledValue, &image_db.nativeOrder);
    if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
    }
    fprintf (stderr, "main database read\n");

    /* match temp image with main images */
    match = match_images (image, Nimage, subset, Nsubset, &Nmatch);
    if (Nmatch != Nsubset) fprintf (stderr, "WARNING: some images missed\n");

    /* update entries in main db */
    gfits_vtable_from_ftable (&image_db.ftable, &image_db.vtable, match, Nmatch);
    for (i = 0; i < Nmatch; i++) {
      gfits_convert_RegImage ((RegImage *) image_db.vtable.buffer[i], sizeof (RegImage), 1);
    }
    gfits_db_update (&image_db);
    gfits_db_close (&image_db);

    gfits_db_free (&image_db);
    gfits_db_free (&temp_db);
    if (match != NULL) free (match);
    if (image != NULL) free (image);
      
  next:
    fflush (stderr);
    fflush (stdout);
    sleep (LOOP_DELAY);
  }
}
