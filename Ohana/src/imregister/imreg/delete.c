# include "imregister.h"
# include "imreg.h"

void DeleteSubset (FITS_DB *db, RegImage *image, off_t Nimage, off_t *match, off_t Nmatch) {

  off_t i, j;
  off_t *keep, Ndel, Nsubset;
  RegImage *subset;

  ALLOCATE (keep, off_t, MAX (Nimage, 1));
  for (i = 0; i < Nimage; i++) keep[i] = TRUE;
  fprintf (stderr, "total of "OFF_T_FMT" images\n",  Nimage);

  Ndel = 0;
  for (i = 0; i < Nmatch; i++) {
    j = match[i];
    if (j == -1) continue;
    keep[j] = FALSE;
    Ndel ++;
  }
  fprintf (stderr, "delete "OFF_T_FMT" images\n",  Ndel);
  if (Ndel == 0) { 
    fprintf (stderr, "SUCCESS\n");
    gfits_db_close (db);
    exit (0);
  }

  /* create new data list */
  Nsubset = Nimage - Ndel;
  ALLOCATE (subset, RegImage, MAX (1, Nsubset));
  fprintf (stderr, "keeping "OFF_T_FMT" images\n",  Nsubset);
  for (j = i = 0; i < Nimage; i++) {
    if (!keep[i]) continue;
    subset[j] = image[i];
    j++;
  }

  free (keep);
  free (image);

  /** we may later want to pull this out and put it elsewhere **/
  /** free db[0].theader, db[0].table.buffer? **/
  gfits_table_set_RegImage (&db[0].ftable, subset, Nsubset, TRUE);
  gfits_db_save (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);

}
