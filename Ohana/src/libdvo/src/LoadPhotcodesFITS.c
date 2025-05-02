# include <dvo.h>

/* this is a read-only function to load the FITS photcode file into the internal photcode table */
/* locking is used to avoid collisions with programs trying to update the photcodes values */
/* XXX better distinction between NOT FOUND and FAILURE */
int LoadPhotcodesFITS (char *filename) {

  PhotCodeData *table = NULL;
  PhotCode *photcode = NULL;
  FITS_DB db;
  char extname[80];

  int i, code, Nsec, Nsecfilt;
  off_t Ncode;

  /* XXX choose more sensible lock timeouts! */
  gfits_db_init (&db);
  db.timeout   = 60.0;
  db.lockstate = LCK_SOFT;

  /* does this mean the db is empty, non-existent, or has access errors? */
  // XXX we need better error handling here
  if (!gfits_db_lock (&db, filename)) {
    // fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    return FALSE;
  }
  if (db.dbstate == LCK_EMPTY) {
    // fprintf (stderr, "ERROR: db is empty\n");
    gfits_db_close (&db);
    return FALSE;
  } 
  if (!gfits_db_load (&db)) {
    // fprintf (stderr, "ERROR: failure to load db\n");
    gfits_db_close (&db);
    return FALSE;
  }
  gfits_db_close (&db);

  /* convert FITS format data to internal format (just byteswaps) */
  gfits_scan (&db.theader, "EXTNAME", "%s", 1, extname);

# define CONVERT_FORMAT(NAME, FORMAT)					\
  if (!strcmp (extname, NAME)) {					\
    PhotCode_##FORMAT *photcode_input = gfits_table_get_PhotCode_##FORMAT (&db.ftable, &Ncode, &db.scaledValue, &db.nativeOrder); \
    if (!photcode_input) {							\
      fprintf (stderr, "ERROR: failed to read photcodes in LoadPhotcodesFITS.c\n");		\
      fprintf (stderr, "file is either corrupted or this is a programming error\n");		\
      fprintf (stderr, "exiting to avoid damaging dvo db\n");		\
      exit (2);								\
    }									\
    photcode = PhotCode_##FORMAT##_To_Internal (photcode_input, Ncode); \
  }

  CONVERT_FORMAT("DVO_PHOTCODE",             Elixir);
  CONVERT_FORMAT("DVO_PHOTCODE_ELIXIR",      Elixir);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_DEV_1",   PS1_DEV_1);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_DEV_2",   PS1_DEV_2);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_DEV_3",   PS1_DEV_3);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_REF",     PS1_REF);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_REF_V2",  PS1_REF_V2);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_REF_V3",  PS1_REF_V3);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V1",      PS1_V1);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V2",      PS1_V2);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V3",      PS1_V3);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V4",      PS1_V4);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V5",      PS1_V5);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V6",      PS1_V6);
  CONVERT_FORMAT("DVO_PHOTCODE_PS1_V5_LOAD", PS1_V5_LOAD);

  gfits_db_free (&db);

  table = GetPhotcodeTable ();
  if (table[0].code != NULL) free (table[0].code);
  /* we are using a 16-bit int for the photcodes, so these indexes can be fixed-length */
  /* XXX if we need to go with a larger photcode, we'll need to use a sequenced index and a
     binary search to get to a given value (0x100000000 ints would take quite a few
     bytes...) */
  for (i = 0; i < 0x10000; i++) {
    table[0].hashcode[i] = -1;
    table[0].hashNsec[i] = -1;
    table[0].codeNsec[i] = -1;
  }

  /* set up photcode indexes (see dvo_photcode_ops.c) */
  Nsecfilt = 0;
  for (i = 0; i < Ncode; i++) {
    if (photcode[i].type == PHOT_ALT) continue; /* no hashcode for ALT codes */
    if (table[0].hashcode[photcode[i].code] != -1) {
      fprintf (stderr, "duplicate photcodes in file\n");
      code = table[0].hashcode[photcode[i].code];
      fprintf (stderr, "conflict between %s (%d) and %s (%d)\n",
	       photcode[i].name, photcode[i].code, photcode[code].name, photcode[code].code);
      free (photcode);
      return FALSE;
    }
    table[0].hashcode[photcode[i].code] = i;
    if (photcode[i].type == PHOT_SEC) {
      table[0].hashNsec[photcode[i].code] = Nsecfilt;
      table[0].codeNsec[Nsecfilt] = photcode[i].code;
      Nsecfilt ++;
    }
  }

  // validity check for references
  // photcode.equiv of 0 means "undefined"
  for (i = 0; i < Ncode; i++) {
    if (!strcasecmp(photcode[i].name, "MAG")) {
      fprintf (stderr, "MAG is not an allowed photcode name (reserved to DVO internals)\n");
      free (photcode);
      return FALSE;
    }
    if (!strcasecmp(photcode[i].name, "FLUX")) {
      fprintf (stderr, "FLUX is not an allowed photcode name (reserved to DVO internals)\n");
      free (photcode);
      return FALSE;
    }

    if (photcode[i].type == PHOT_DEP) {
      if (photcode[i].equiv == 0) continue;
      Nsec = table[0].hashcode[photcode[i].equiv];
      if ((Nsec >= Ncode) || (Nsec < 0)) {
	fprintf (stderr, "reference for dependent photcode is not in photcodes\n");
	free (photcode);
	return FALSE;
      }
      if (photcode[Nsec].type != PHOT_SEC) {
	fprintf (stderr, "reference for dependent photcode is not an average photcode\n");
	free (photcode);
	return FALSE;
      }
    }
    if (photcode[i].type == PHOT_ALT) {
      Nsec = table[0].hashcode[photcode[i].code];
      if ((Nsec >= Ncode) || (Nsec < 0)) {
	fprintf (stderr, "reference for alternate photcode is not in photcodes\n");
	free (photcode);
	return FALSE;
      }
      if (photcode[Nsec].type != PHOT_SEC) {
	fprintf (stderr, "reference for alternate photcode is not an average photcode\n");
	free (photcode);
	return FALSE;
      }
    }
  }

  table[0].code     = photcode;
  table[0].Ncode    = Ncode;
  table[0].Nsecfilt = Nsecfilt;

  return (TRUE);
}
