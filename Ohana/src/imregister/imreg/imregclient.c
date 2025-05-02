# include "imregister.h"
# include "imreg.h"

int imregclient (char *fitsfile, char *statfile, char *datfile) {

  int i, Nentry, Nslice, tmpint;
  FILE *f;
  RegImage *image;
  FITS_DB db;

  Nentry = 0;
  image = iminfo (fitsfile);
  
  /* if images is MEF or SPLIT/SINGLE, load stats file */
  /* get stats file (has sky, bias, etc) */
  switch (image[0].mode) {
  case M_MEF:
  case M_SPLIT:
    f = fopen (statfile, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file.stats\n");
      exit (1);
    }
    if (fscanf (f, "%f %f", &image[0].sky, &image[0].bias) != 2) {
      fprintf (stderr, "error reading stats\n");
    }
    fclose (f);
    image[0].fwhm = get_fwhm (datfile);
    Nentry = 1;
    break;
  case M_SINGLE:
    f = fopen (statfile, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file.stats\n");
      exit (1);
    }
    if (fscanf (f, "%d %f %f", &tmpint, &image[0].sky, &image[0].fwhm) != 3) {
      fprintf (stderr, "error reading stats\n");
    }
    image[0].ccd = tmpint;
    fclose (f);
    Nentry = 1;
    break;
  case M_CUBE:
    f = fopen (statfile, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open file.stats\n");
      exit (1);
    }
    Nslice = image[0].seq;
    Nentry = Nslice + 1;
    REALLOCATE (image, RegImage, Nentry);
    for (i = 0; i < Nentry; i++) {
      image[i] = image[0];
      if (fscanf (f, "%d %f %f", &tmpint, &image[i].sky, &image[i].fwhm) != 3) {
	fprintf (stderr, "error reading stats\n");
      }
      image[i].seq = tmpint;
      if (image[i].seq == Nslice) continue;
      image[i].obstime += image[0].seqtime*image[i].seq;
    }
    fclose (f);
    Nentry = i;
    break;
  }
  if (NoReg) dump_data (image, Nentry);

  gfits_db_init (&db);
  db.lockstate = LCK_HARD;
  db.timeout   = 300.0;

  if (!gfits_db_lock (&db, TempDB)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (db.dbstate == LCK_EMPTY) {
    gfits_db_create (&db);
    gfits_table_set_RegImage (&db.ftable, NULL, 0, TRUE);
  } else {  
    if (!gfits_db_load (&db)) {
      fprintf (stderr, "ERROR: failure to load db\n");
      gfits_db_close (&db);
      exit (1);
    }
  }

  gfits_convert_RegImage (image, sizeof (RegImage), Nentry);
  gfits_table_to_vtable (&db.ftable, &db.vtable, 0, 0);
  gfits_vadd_rows (&db.vtable, (char *) image, Nentry, sizeof(RegImage));

  gfits_db_update (&db);
  gfits_db_close (&db);
  gfits_db_free (&db);

  fprintf (stderr, "SUCCESS: wrote temp image data\n");
  exit (0);
}
