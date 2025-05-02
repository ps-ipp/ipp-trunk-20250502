# include "imregister.h"
# include "imphot.h"

enum {FITS, TEXT};

int db_load (FITS_DB *db) {

  int Nx, Ny, Naxis;
  int mode;

  /* database name must be set first */
  if (db == NULL) {
    fprintf (stderr, "db handle is not set\n");
    return (FALSE);
  }

  /* init & load in FITS table data - return FALSE on error */
  if (!gfits_fread_header (db[0].f, &db[0].header)) {
    fprintf (stderr, "can't read primary header\n"); 
    return (FALSE);
  }

  gfits_scan (&db[0].header, "NAXIS",  "%d", 1, &Naxis);
  gfits_scan (&db[0].header, "NAXIS1", "%d", 1, &Nx);
  gfits_scan (&db[0].header, "NAXIS2", "%d", 1, &Ny);
  
  mode = FITS;
  if ((Naxis == 2) && (Nx == 1106) && (Ny == 1024)) {
    mode = TEXT;
  }
  
  /* how do we decide if it is text or fits? must examine header */
  if (mode == FITS) {
    rfits (db);
  } else {
    rtext (db);
  }
  return (TRUE);
}
