# include "imregister.h"
# include "detrend.h"

void DeleteSubset (FITS_DB *db, DetReg *image, off_t Nimage, Match *match, off_t Nmatch) {

  off_t i, j;
  off_t *keep, Ndel, Nsubset;
  DetReg *subset;

  ALLOCATE (keep, off_t, MAX (Nimage, 1));
  for (i = 0; i < Nimage; i++) keep[i] = TRUE;
  fprintf (stderr, "total of "OFF_T_FMT" detrend images\n",  Nimage);

  Ndel = 0;
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    if (i == -1) continue;
    keep[i] = FALSE;
    Ndel ++;
    delete_image (&image[i]);
  }
  fprintf (stderr, "delete "OFF_T_FMT" images\n",  Ndel);
  if (Ndel == 0) { 
    fprintf (stderr, "SUCCESS\n");
    gfits_db_close (db);
    exit (0);
  }

  /* create new data list */
  Nsubset = Nimage - Ndel;
  ALLOCATE (subset, DetReg, MAX (1, Nsubset));
  fprintf (stderr, "keeping "OFF_T_FMT" images\n",  Nsubset);
  for (j = i = 0; i < Nimage; i++) {
    if (!keep[i]) continue;
    subset[j] = image[i];
    j ++;
  }
  free (keep);
  free (image);

  /** we may later want to pull this out and put it elsewhere **/
  /** free db[0].theader, db[0].table.buffer? **/
  gfits_table_set_DetReg (&db[0].ftable, subset, Nsubset, TRUE);
  gfits_db_save (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
