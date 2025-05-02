# include <ohana.h>
# include <gfitsio.h>

# define GFITS_FREAD_COMPLETE_READ 0
# define GFITS_FREAD_INCOMPLETE_FILE 1
# define GFITS_FREAD_INCOMPLETE_READ 2

/*********************** fits read table ***********************************/
int gfits_read_ftable (char *filename, FTable *table, char *extname) {

  int status;
  FILE *f;

  f = fopen (filename, "r");
  if (f == NULL) return (FALSE);

  status = gfits_fread_ftable (f, table, extname);
  fclose (f);
  return (status);
}	

/*********************** fits read table ***********************************/
int gfits_fread_ftable (FILE *f, FTable *table, char *extname) {

  Header *header;

  header = table[0].header;
  if (!gfits_find_Xheader (f, header, extname)) return FALSE;

  if (gfits_fread_ftable_data (f, table, FALSE)) return (TRUE);
  gfits_free_header (header);
  return (FALSE);
}

/*********************** gfits_fread_incomplete_case ***********************************/
int gfits_fread_incomplete_case (FILE *f, FTable *table, int padIfShort, off_t Nread, off_t Nbytes) {
  char string[128];

  snprintf (string, 128, "FITS file is short (%s)", __func__);
  perror (string);
  if (Nread < gfits_data_min_size (table[0].header)) {
    fprintf (stderr, "error: fits read error in %s, read "OFF_T_FMT", need "OFF_T_FMT"\n", __func__,  Nread,  gfits_data_min_size (table[0].header));
    if (!padIfShort) {
      gfits_free_table (table);
      return FALSE;
    }
    memset (&table[0].buffer[Nread], 0, Nbytes - Nread);
    fprintf (stderr, "warning: file missing data, padding with zeros: USE AT YOUR OWN RISK!\n");
  } else {
    fprintf (stderr, "warning: file missing pad\n");
  }
  return TRUE;
}

// retry several times to read the file, if we hit the EOF, give up, otherwise try again
// sleep for 500,000 nanoseconds between attempts (or longer?)
off_t gfits_fread_retry (int *status, char *buffer, off_t Nbytes, FILE *f, int Nretry) {
  
  off_t Nread = 0;

  for (int i = 0; (i < Nretry) && (Nread < Nbytes); i++) {
    off_t Nbyte_left = Nbytes - Nread;
    off_t Nread_pass = fread (&buffer[Nread], sizeof (char), Nbyte_left, f);
    
    Nread += Nread_pass;
    if (Nread != Nbytes) {
      if (feof(f)) {
	*status = GFITS_FREAD_INCOMPLETE_FILE;
	return Nread;
      }
      sleep (2);
      if (i < Nretry - 1) fprintf (stderr, "incomplete file read, retrying\n");
    }
  }

  if (Nread != Nbytes) {
    if (feof(f)) {
      *status = GFITS_FREAD_INCOMPLETE_FILE;
    } else {
      *status = GFITS_FREAD_INCOMPLETE_READ;
    }
  } else {
    *status = GFITS_FREAD_COMPLETE_READ;
  }
  return Nread;
}

/*********************** fits read ftable data ***********************************/
int gfits_fread_ftable_data (FILE *f, FTable *table, int padIfShort) {

  int status;
  off_t Nbytes, Nread;

  /* find buffer size */
  Nbytes = gfits_data_size (table[0].header);
  ALLOCATE (table[0].buffer, char, Nbytes);

  Nread = gfits_fread_retry (&status, table[0].buffer, Nbytes, f, 5);
  if (status != GFITS_FREAD_COMPLETE_READ) {
    if (!gfits_fread_incomplete_case(f, table, padIfShort, Nread, Nbytes)) return FALSE;
  }
  table[0].validsize = Nread;
  table[0].datasize = Nbytes;
  table[0].heap_start = gfits_heap_start (table->header);
  return (TRUE);
}	

/*********************** fits read ftable data ***********************************/
int gfits_fread_ftable_range (FILE *f, int padIfShort, int noSeek, FTable *table, off_t start, off_t Nrows) {

  off_t Nbytes, Nread, Nskip, Nx, Ny;

  /* find disk table size */
  Nx = table[0].header[0].Naxis[0];
  Ny = table[0].header[0].Naxis[1];

  // it is an error to ask for data starting out-of-bounds
  if (start < 0) return (FALSE);
  if (start >= Ny) return (FALSE);
  
  // if we request more data than is available, we will stop at the table end.
  Nrows = MIN (Nrows, Ny - start);

  Nskip = start * Nx;
  Nbytes = Nrows * Nx;

  if (table[0].buffer) {
    if (table[0].datasize < Nbytes) {
      REALLOCATE (table[0].buffer, char, MAX (Nbytes, 1));
    }
  } else {
    ALLOCATE (table[0].buffer, char, MAX (Nbytes, 1));
  }

  if (!noSeek) {
    fseeko (f, Nskip, SEEK_CUR);
  }

  int status;
  Nread = gfits_fread_retry (&status, table[0].buffer, Nbytes, f, 5);
  if (status != GFITS_FREAD_COMPLETE_READ) {
    if (!gfits_fread_incomplete_case(f, table, padIfShort, Nread, Nbytes)) return FALSE;
  }

  /* modify structure and header to match actual read rows Ny */
  table[0].header[0].Naxis[1] = Nrows;
  gfits_modify (table[0].header, "NAXIS2",  OFF_T_FMT, 1,  Nrows);
  table[0].validsize = Nread;
  table[0].datasize = Nbytes;
  table[0].heap_start = gfits_heap_start (table->header);

  return (TRUE);
}	

/*********************** fits read ftable data ***********************************/
int gfits_fread_vtable_range (FILE *f, VTable *table, off_t start, off_t Nrows) {

  off_t i, Nbytes, Nread, Nskip, Nx, Ny;
  char *buffer;

  /* find buffer size */
  Nx = table[0].header[0].Naxis[0];
  Ny = table[0].header[0].Naxis[1];
  table[0].datasize = gfits_data_size (table[0].header);
  // table[0].heap_start = gfits_heap_start (table[0].header);
  table[0].pad = table[0].datasize - Nx*Ny;

  if (start < 0) return (FALSE);
  if (start + Nrows >= Ny) return (FALSE);
  
  Nskip = start * Nx;
  Nbytes = Nrows * Nx;
  ALLOCATE (buffer, char, MAX (Nbytes, 1));

  fseeko (f, Nskip, SEEK_CUR);
  Nread = fread (buffer, sizeof (char), Nbytes, f);
  if (Nread != Nbytes) {
    if (feof(f)) {
      perror ("fits read error");
      free (buffer);
      return (FALSE);
    } else {
      // try a second time
      off_t Nextra = Nbytes - Nread;
      off_t Nxread = fread (&buffer[Nread], sizeof (char), Nextra, f);
      Nread += Nxread;
      if (Nread != Nbytes) {
	perror ("fits read error");
	free (buffer);
	return (FALSE);
      }
    }
  }

  ALLOCATE (table[0].row, off_t, MAX (Nrows, 1));
  ALLOCATE (table[0].buffer, char *, MAX (Nrows, 1));
  for (i = 0; i < Nrows; i++) {
    ALLOCATE (table[0].buffer[i], char, MAX (Nx, 1));
    memcpy (table[0].buffer[i], &buffer[i*Nx], Nx);
    table[0].row[i] = start + i;
  }
  free (buffer);

  return (TRUE);
}	

/*********************** fits read virtual table ***********************************/
int gfits_fread_vtable (FILE *f, VTable *table, char *extname, off_t Nrow, off_t *row) {

  off_t i, j;
  off_t Nbytes, Nread;
  off_t start, Nx, Ny, offset;
  Header *header;
  char tname[80];

  header = table[0].header;
  fseeko (f, 0, SEEK_SET);

  for (j = -1; TRUE; j++) {
    /* load data for this header */
    if (!gfits_load_header (f, header)) return (FALSE);

    /* find buffer size */
    Nbytes = gfits_data_size (header);

    /* check if this is the correct extension or not */
    bzero (tname, 80);
    gfits_scan (header, "EXTNAME", "%s", 1, tname);
    if (strcmp (tname, extname)) {
      /* skip to next header */
      fseeko (f, Nbytes, SEEK_CUR);
      gfits_free_header (header);
      continue;
    }

    /* file pointer is at beginning of desired table data */
    start = ftello (f);

    gfits_scan (header, "NAXIS1", OFF_T_FMT, 1, &Nx);
    gfits_scan (header, "NAXIS2", OFF_T_FMT, 1, &Ny);

    for (i = 0; i < Nrow; i++) {
      if (row[i] > Ny) { return (FALSE); }
    }

    ALLOCATE (table[0].buffer, char *, MAX (1, Nrow));
    for (i = 0; i < Nrow; i++) {
      ALLOCATE (table[0].buffer[i], char, MAX (1, Nx));
      offset = start + Nx*row[i];
      fseeko (f, offset, SEEK_SET);
      Nread = fread (table[0].buffer[i], sizeof (char), Nx, f);
      if (Nread != Nx) { 
	if (feof(f)) {
	  perror ("fits read error");
	  return (FALSE);
	} else {
	  // try a second time
	  off_t Nextra = Nx - Nread;
	  off_t Nxread = fread (&table[0].buffer[i][Nread], sizeof (char), Nextra, f);
	  Nread += Nxread;
	  if (Nread != Nx) {
	    perror ("fits read error");
	    return (FALSE);
	  }
	}
      }
    }

    table[0].Nrow  = Nrow;
    ALLOCATE (table[0].row, off_t, MAX (1, Nrow));    
    for (i = 0; i < Nrow; i++) table[0].row[i] = row[i];
    table[0].datasize = gfits_data_size (table[0].header);
    // table[0].heap_start = gfits_heap_start (table[0].header);
    table[0].pad      = table[0].datasize - Nx*Ny;
    return (TRUE);
  }
}	

int gfits_fread_header_extname (FILE *f, Header *header, char *extname) {

  off_t Nbytes;
  char current[80];

  fseeko (f, 0, SEEK_SET);
  gfits_fread_header (f, header);

  if (!strcasecmp (extname, "PHU")) return (TRUE);

  Nbytes = gfits_data_size (header);
  fseeko (f, Nbytes, SEEK_CUR);

  while (gfits_fread_header (f, header)) {
    gfits_scan (header, "EXTNAME", "%s", 1, current);
    if (!strcmp (current, extname)) return (TRUE);
    Nbytes = gfits_data_size (header);
    fseeko (f, Nbytes, SEEK_CUR);
  }
  return (FALSE);
}
