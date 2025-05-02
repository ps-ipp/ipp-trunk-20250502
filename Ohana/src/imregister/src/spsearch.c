# include "imregister.h"
# include "spreg.h"
static char *version = "spsearch $Revision: 1.8 $";

int main (int argc, char **argv) {
 
  off_t Nmatch, Nspectrum, *match;
  Spectrum *spectrum;
  FITS_DB db;

  get_version (argc, argv, version);
  args (argc, argv);

  gfits_db_init (&db);
  db.lockstate = (output.modify || output.delete) ? LCK_HARD : LCK_SOFT;
  db.timeout   = 300.0;

  if (!gfits_db_lock (&db, SpectrumDB)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (!gfits_db_load (&db)) {
    fprintf (stderr, "ERROR: failure to load db\n");
    gfits_db_close (&db);
    exit (1);
  }

  if (!output.modify && !output.delete) gfits_db_close (&db);

  spectrum = gfits_table_get_Spectrum (&db.ftable, &Nspectrum, &db.scaledValue, &db.nativeOrder);
  if (!spectrum) {
    fprintf (stderr, "ERROR: failed to read spectrum\n");
    exit (2);
  }
  
  match = match_criteria (spectrum, Nspectrum, &Nmatch);
  match = unique_entries (spectrum, Nspectrum, match, &Nmatch);

  if (output.modify) ModifySubset (&db, spectrum, Nspectrum, match, Nmatch);
  if (output.delete) DeleteSubset (&db, spectrum, Nspectrum, match, Nmatch);

  OutputSubset (spectrum, Nspectrum, match, Nmatch);
  exit (0);
}
