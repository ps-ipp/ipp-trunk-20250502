# include "imregister.h"
# include "imreg.h"
static char *version = "imregister $Revision: 3.8 $";

int main (int argc, char **argv) {
 
  off_t Nregimage;
  RegImage *image, *regimage;
  FITS_DB db;

  get_version (argc, argv, version);
  args (argc, argv);

  image = iminfo (argv[1]);
  regimage = newimages (image, &Nregimage);

  if (NoReg) goto skip_reg;

  gfits_db_init (&db);
  db.lockstate = LCK_HARD;
  db.timeout   = 300.0;

  if (!gfits_db_lock (&db, ImageDB)) {
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

  gfits_convert_RegImage (regimage, sizeof (RegImage), Nregimage);
  gfits_table_to_vtable (&db.ftable, &db.vtable, 0, 0);
  gfits_vadd_rows (&db.vtable, (char *) regimage, Nregimage, sizeof(RegImage));

  gfits_db_update (&db);
  gfits_db_close (&db);
  gfits_db_free (&db);

skip_reg:
  if (IMSORT) SubmitImages (image);
  fprintf (stderr, "SUCCESS: registered %s\n", argv[1]);
  exit (0);
}

