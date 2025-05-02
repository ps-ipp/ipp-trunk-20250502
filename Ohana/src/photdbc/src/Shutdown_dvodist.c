# include "dvodist.h"

static FITS_DB db;

void LockDatabase (char *catdir) {

  char ImageCat[1024];

  // lock image table
  snprintf (ImageCat, 1024, "%s/Images.dat", catdir);

  // lock the image db table 
  gfits_db_init (&db);
  int status = dvo_image_lock (&db, ImageCat, 60.0, LCK_XCLD);
  if (!status) {
    Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  }

  // if the file is missing, the function above will succeed, but the db.dbstate will have
  // a value of either: LCK_EMPTY or LCK_MISSING
  if ((db.dbstate == LCK_EMPTY) || (db.dbstate == LCK_MISSING)) {
    Shutdown ("no images?");
  }

  // XXX this program should be able to operate on a dvo database without any images (like
  // 2MASS).  but, we still need to lock the database when moving the files around to
  // avoid collisions.  So, we probably need to create an empty image table and lock it if
  // there are no images at this time.
}

/* clean up open / locked ImageCat before shutting down */
int Shutdown (char *format, ...) {  
  va_list argp;
  char *formatplus;
  
  ALLOCATE (formatplus, char, strlen(format));
  strcpy (formatplus, format);
  strcat (formatplus, "\n");

  va_start (argp, format);
  vfprintf (stderr, formatplus, argp);
  free (formatplus);
  va_end (argp);

  fprintf (stderr, "ERROR: dvodist halted\n");
  exit (1);
}
