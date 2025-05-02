# include "imregister.h"
# include "imreg.h"
static char *version = "imregtable $Revision: 3.8 $";

int main (int argc, char **argv) {
 
  off_t Nimage;
  char *infile;
  RegImage *image;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;
  FITS_DB db;

  get_version (argc, argv, version);
  ConfigInit (&argc, argv);
  ConfigCamera ();
  ConfigFilter ();

  if (argc != 2) {
    fprintf (stderr, "USAGE: imregtable (table)\n");
    exit (1);
  }

  /* load in table data */
  infile = argv[1];

  /* need to error check these */
  ftable.header = &theader;
  gfits_read_header  (infile, &header);
  gfits_read_matrix  (infile, &matrix);
  gfits_read_ftable  (infile, &ftable, "IMAGE_DATABASE");

  image = gfits_table_get_RegImage (&ftable, &Nimage, NULL, NULL);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  /* load database table */
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

  gfits_convert_RegImage (image, sizeof (RegImage), Nimage);
  gfits_table_to_vtable (&db.ftable, &db.vtable, 0, 0);
  gfits_vadd_rows (&db.vtable, (char *) image, Nimage, sizeof(RegImage));

  gfits_db_update (&db);
  gfits_db_close (&db);
  gfits_db_free (&db);

  fprintf (stderr, "SUCCESS: registered %s\n", argv[1]);
  exit (0);
}
