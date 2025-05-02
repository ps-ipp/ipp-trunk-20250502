# include "imregister.h"
# include "detrend.h"

int ModifySubset (FITS_DB *db, DetReg *image, off_t Nimage, Match *match, off_t Nmatch) {
  
  off_t i, j, value;
  off_t *list;
  
  value = M_UNDEF;

  ALLOCATE (list, off_t, Nimage);
  if (!strcasecmp (output.ModifyEntry, "mode")) {
    value = get_image_mode (output.ModifyValue);
    if (value == M_UNDEF) {
      fprintf (stderr, "ERROR: invalid image mode %s\n", output.ModifyValue);
      exit (1);
    }
  }
  if (!strcasecmp (output.ModifyEntry, "order")) {
    value = atoi (output.ModifyValue);
  }

  /* list matched images */
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    list[j] = i;
    
    if (!strcasecmp (output.ModifyEntry, "label")) {
      snprintf (image[i].label, 64, "%s", output.ModifyValue);
    }
    if (!strcasecmp (output.ModifyEntry, "order")) {
      image[i].Norder = value;
    }
    if (!strcasecmp (output.ModifyEntry, "mode")) {
      image[i].mode   = value;
    }
    if (!strcasecmp (output.ModifyEntry, "tstart")) {
      image[i].tstart = output.TimeValue;
    }
    if (!strcasecmp (output.ModifyEntry, "tstop")) {
      image[i].tstop  = output.TimeValue;
    }
  }

  /** we may later want to pull this out and put it elsewhere **/
  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, list, Nmatch);
  for (i = 0; i < Nmatch; i++) {
    gfits_convert_DetReg ((DetReg *) db[0].vtable.buffer[i], sizeof (DetReg), 1);
  }
  gfits_db_update (db);
  gfits_db_close (db);
  gfits_db_free (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);

}

