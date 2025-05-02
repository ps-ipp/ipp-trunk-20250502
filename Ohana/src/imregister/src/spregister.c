# include "imregister.h"
# include "spreg.h"
static char *version = "spregister $Revision: 1.9 $";

int main (int argc, char **argv) {
 
  Spectrum *spectrum;
  FITS_DB db;

  get_version (argc, argv, version);
  args (argc, argv);

  spectrum = spinfo (argv[1]);

  if (DUMP) showinfo (spectrum);

  if (NoReg) exit (0);

  gfits_db_init (&db);
  db.lockstate = LCK_HARD;
  db.timeout   = 300.0;

  if (!gfits_db_lock (&db, SpectrumDB)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (db.dbstate == LCK_EMPTY) {
    gfits_db_create (&db);
    gfits_table_set_Spectrum (&db.ftable, NULL, 0, TRUE);
  } else {  
    if (!gfits_db_load (&db)) {
      fprintf (stderr, "ERROR: failure to load db\n");
      gfits_db_close (&db);
      exit (1);
    }
  }

  gfits_convert_Spectrum (spectrum, sizeof (Spectrum), 1);
  gfits_table_to_vtable (&db.ftable, &db.vtable, 0, 0);
  gfits_vadd_rows (&db.vtable, (char *) spectrum, 1, sizeof(Spectrum));

  gfits_db_update (&db);
  gfits_db_close (&db);
  gfits_db_free (&db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}


/* notes:
   SpectrumDB set in args:ConfigInit by config variable SPECTRUM_DB

*/
