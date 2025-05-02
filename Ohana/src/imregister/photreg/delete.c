# include "imregister.h"
# include "photreg.h"

void DeleteSubset (FITS_DB *db, PhotPars *photdata, off_t Nphotdata, off_t *match, off_t Nmatch) {

  off_t i, j;
  off_t *keep, Ndel, Nsubset;
  PhotPars *subset;

  ALLOCATE (keep, off_t, MAX (Nphotdata, 1));
  for (i = 0; i < Nphotdata; i++) keep[i] = TRUE;
  fprintf (stderr, "total of "OFF_T_FMT" photdata\n",  Nphotdata);

  Ndel = 0;
  for (i = 0; i < Nmatch; i++) {
    j = match[i];
    if (j == -1) continue;
    keep[j] = FALSE;
    Ndel ++;
  }
  fprintf (stderr, "delete "OFF_T_FMT" photdata\n",  Ndel);

  if (Ndel == 0) { 
    fprintf (stderr, "SUCCESS\n");
    gfits_db_close (db);
    gfits_db_free (db);
    exit (0);
  }

  /* create new data list */
  Nsubset = Nphotdata - Ndel;
  ALLOCATE (subset, PhotPars, MAX (1, Nsubset));
  fprintf (stderr, "keeping "OFF_T_FMT" photdata\n",  Nsubset);
  for (j = i = 0; i < Nphotdata; i++) {
    if (!keep[i]) continue;
    subset[j] = photdata[i];
    j++;
  }

  gfits_table_set_PhotPars (&db[0].ftable, subset, Nsubset, TRUE);
  gfits_db_save (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
