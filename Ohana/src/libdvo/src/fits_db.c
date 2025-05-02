# include <dvo.h>

/* init the db structure */
int gfits_db_init (FITS_DB *db) {

  db[0].f             = NULL;
  db[0].filename      = NULL;
  db[0].dbstate       = 0;
  db[0].lockstate     = 0;
  db[0].mode          = 0;
  db[0].format        = 0;
  db[0].virtual       = 0;
  db[0].nativeOrder   = 0;
  db[0].scaledValue   = 0;
  
  gfits_init_header (&db->header);
  gfits_init_matrix (&db->matrix);
  gfits_init_header (&db->theader);
  gfits_init_table  (&db->ftable);
  db->ftable.header = &db->theader;
  gfits_init_vtable (&db->vtable);
  return (TRUE);
}

/* create an empty db */
int gfits_db_create (FITS_DB *db) {
  if (!gfits_init_header (&db[0].header)) return (FALSE);
  db[0].header.extend = TRUE;
  if (!gfits_create_header (&db[0].header))  return (FALSE);
  if (!gfits_create_matrix (&db[0].header, &db[0].matrix)) return (FALSE);
  if (!gfits_print (&db[0].header, "NEXTEND", "%d", 1, 1)) return (FALSE);
  db[0].ftable.header = &db[0].theader;
  return (TRUE);
}

int gfits_db_lock (FITS_DB *db, char *filename) {
  
  /* database name must be set first */
  if (filename == NULL) {
    fprintf (stderr, "db file is not set\n");
    return (FALSE);
  }

  /* database handle must be set first */
  if (db == NULL) {
    fprintf (stderr, "db handle is not set\n");
    return (FALSE);
  }

  /* lock & open database */
  db[0].f = fsetlockfile (filename, db[0].timeout, db[0].lockstate, &db[0].dbstate);
  if (db[0].f == NULL) {
    if (db[0].dbstate == LCK_MISSING) {
      fprintf (stderr, "no data in db %s\n", filename);
    } else {
      fprintf (stderr, "cannot set lock on db file %s\n", filename);
    }
    return (FALSE);
  }
  
  db[0].filename = strcreate (filename);
  return (TRUE);
}

/* load the complete db table into memory - load first extension, do not validate EXTNAME */
int gfits_db_load (FITS_DB *db) {

  /* database name must be set first */
  if (db == NULL) {
    fprintf (stderr, "db handle is not set\n");
    return (FALSE);
  }

  /* init & load in FITS table data - return FALSE on error */
  if (!gfits_fread_header (db[0].f, &db[0].header)) {
    fprintf (stderr, "can't read primary header\n"); 
    gfits_db_free (db);
    return (FALSE);
  }
  if (!gfits_fread_matrix (db[0].f, &db[0].matrix, &db[0].header)) {
    fprintf (stderr, "can't read primary matrix\n");
    gfits_db_free (db);
    return (FALSE);
  }
  if (!gfits_fread_header (db[0].f, &db[0].theader)) {
    fprintf (stderr, "can't read table header\n");
    gfits_db_free (db);
    return (FALSE);
  }
  if (!gfits_fread_ftable_data (db[0].f, &db[0].ftable, FALSE)) {
    fprintf (stderr, "can't read table data\n");
    gfits_db_free (db);
    return (FALSE);
  }
  db[0].nativeOrder = FALSE;  /* table does not have internal byte-order */
  db[0].scaledValue = FALSE;  /* table has not been scaled by BZERO,BSCALE */
  return (TRUE);
}

/* load the Nrows data from table starting at start; load header, etc if needed */
int gfits_db_load_segment (FITS_DB *db, off_t start, off_t Nrows) {

  off_t Nskip;

  /* database name must be set first */
  if (db == NULL) {
    fprintf (stderr, "db handle is not set\n");
    return (FALSE);
  }

  /* database name must be opened */
  if (db[0].f == NULL) {
    fprintf (stderr, "db is not opened\n");
    return (FALSE);
  }

  /* position to start of file */
  Fseek (db[0].f, 0, SEEK_SET);

  /* load or skip header */
  if (db[0].header.buffer == NULL) {
    if (!gfits_fread_header (db[0].f, &db[0].header)) {
      fprintf (stderr, "can't read primary header\n"); 
      return (FALSE);
    }
  } else {
    Nskip = db[0].header.datasize;
    Fseek (db[0].f, Nskip, SEEK_CUR);
  }

  /* load or skip matrix */
  if (db[0].matrix.buffer == NULL) {
    if (!gfits_fread_matrix (db[0].f, &db[0].matrix, &db[0].header)) {
      fprintf (stderr, "can't read primary matrix\n");
      return (FALSE);
    }
  } else {
    Nskip = gfits_data_size (&db[0].header);
    Fseek (db[0].f, Nskip, SEEK_CUR);
  }

  /* load or skip table header */
  if (db[0].theader.buffer == NULL) {
    if (!gfits_fread_header (db[0].f, &db[0].theader)) {
      fprintf (stderr, "can't read table header\n");
      return (FALSE);
    }
  } else {
    Nskip = db[0].header.datasize;
    Fseek (db[0].f, Nskip, SEEK_CUR);
  }

  /* read table segment into vtable */
  if (!gfits_fread_vtable_range (db[0].f, &db[0].vtable, start, Nrows)) {
    fprintf (stderr, "can't read table data\n");
    return (FALSE);
  }
  return (TRUE);
}

/* write complete db file */
int gfits_db_save (FITS_DB *db) {

  /* write all data to file */
  make_backup (db[0].filename);
  Fseek (db[0].f, 0, SEEK_SET);

  int fd = fileno (db->f);
  if (ftruncate (fd, 0)) {
    perror ("gfits_db_save: ");
    return (FALSE);
  }

  if (!gfits_fwrite_header  (db[0].f, &db[0].header)) {
    fprintf (stderr, "can't write primary header\n");
    return (FALSE);
  }
  if (!gfits_fwrite_matrix  (db[0].f, &db[0].matrix)) {
    fprintf (stderr, "can't write primary matrix\n");
    return (FALSE);
  }
  if (!gfits_fwrite_Theader (db[0].f, &db[0].theader)) {
    fprintf (stderr, "can't write table header\n");
    return (FALSE);
  }
  if (!gfits_fwrite_table   (db[0].f, &db[0].ftable)) {
    fprintf (stderr, "can't write table data\n");
    return (FALSE);
  }
  return (TRUE);
}

/* write vtable to db file (also appends rows to the end of the table) */
int gfits_db_update (FITS_DB *db) {

  /* this section is not valid if we have changed the size of header, matrix, theader */

  /* write subset to file */
  // make_backup (db[0].filename);  /*** drop this!!?? ***/
  Fseek (db[0].f, 0, SEEK_SET);

  /* do we revert to the old version if this fails? */
  if (!gfits_fwrite_header   (db[0].f, &db[0].header))  {
    fprintf (stderr, "can't update primary header\n");
    return (FALSE);
  }
  if (!gfits_fwrite_matrix   (db[0].f, &db[0].matrix))  {
    fprintf (stderr, "can't update primary matrix\n");
    return (FALSE);
  }
  if (!gfits_fwrite_Theader  (db[0].f, &db[0].theader)) {
    fprintf (stderr, "can't update table header\n");
    return (FALSE);
  }
  if (!gfits_fwrite_vtable   (db[0].f, &db[0].vtable))  {
    fprintf (stderr, "can't update table data\n");
    return (FALSE);
  }
  return (TRUE);
}

/* free memory associated with db handle */
int gfits_db_free (FITS_DB *db) {
  gfits_free_header (&db[0].header);
  gfits_free_matrix (&db[0].matrix);
  gfits_free_header (&db[0].theader);
  gfits_free_table  (&db[0].ftable);
  gfits_free_vtable (&db[0].vtable);
  if (db[0].filename != NULL) {
    free (db[0].filename);
    db[0].filename = NULL;
  }
  return (TRUE);
}

/* close the db files (close open file & unlock) */
int gfits_db_close (FITS_DB *db) {
  if (db[0].f == NULL) return (TRUE);
  fclearlockfile (db[0].filename, db[0].f, db[0].lockstate, &db[0].dbstate);
  db[0].f = NULL;
  return (TRUE);
}  

/* for this to be atomic, need to unlink before we unlock */
/* unlink file here if resulting file is empty? */
/* if (db[0].Nrow == 0) && (lockstate != LCK_SOFT)) unlink (dBFile); */
