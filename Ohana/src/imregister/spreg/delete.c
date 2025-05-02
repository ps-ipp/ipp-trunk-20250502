# include "imregister.h"
# include "spreg.h"

void DeleteSubset (FITS_DB *db, Spectrum *spectrum, off_t Nspectrum, off_t *match, off_t Nmatch) {

  off_t i, j;
  off_t *keep, Nbad, Nsubset;
  Spectrum *subset;

  ALLOCATE (keep, off_t, MAX (Nspectrum, 1));
  for (i = 0; i < Nspectrum; i++) keep[i] = TRUE;
  fprintf (stderr, "total of "OFF_T_FMT" spectra\n",  Nspectrum);

  Nbad = 0;
  for (i = 0; i < Nmatch; i++) {
    j = match[i];
    if (j == -1) continue;
    keep[j] = FALSE;
    Nbad ++;
  }
  fprintf (stderr, "delete "OFF_T_FMT" spectra\n",  Nbad);
  if (Nbad == 0) { 
    fprintf (stderr, "SUCCESS\n");
    gfits_db_close (db);
    exit (0);
  }

  Nsubset = Nspectrum - Nbad;
  ALLOCATE (subset, Spectrum, MAX (1, Nsubset));
  fprintf (stderr, "keeping "OFF_T_FMT" spectra\n",  Nsubset);
  for (j = i = 0; i < Nspectrum; i++) {
    if (!keep[i]) continue;
    subset[j] = spectrum[i];
    j++;
  }
  free (keep);
  free (spectrum);

  /** we may later want to pull this out and put it elsewhere **/
  /** free db[0].theader, db[0].table.buffer? **/
  gfits_table_set_Spectrum (&db[0].ftable, subset, Nsubset, TRUE);
  gfits_db_save (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
