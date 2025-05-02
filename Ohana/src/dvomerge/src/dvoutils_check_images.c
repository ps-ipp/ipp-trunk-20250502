# include "dvoutils.h"

static off_t *externIDs = NULL;
static off_t *externIDc = NULL;

# define STRFAIL { fprintf (stderr, "failure on image %s\n", image[i].name); continue; }

void osort (off_t *value, off_t N);
off_t load_extern_ids (char *idfile, off_t *nids);
off_t externIDs_bisection (off_t *values, off_t threshold, off_t Nvalues);
Image *LoadImages (FITS_DB *db, char *filename, off_t *Nimage);

int dvoutils_check_images (void) {

  FITS_DB db;  // database handle pointing to input image table

  off_t i, k, Nimage;
  
  Image *image;

  // load the extern_id table
  off_t NexternIDs = 0;
  load_extern_ids (EXTERN_ID_LIST, &NexternIDs);
  fprintf (stderr, "loaded "OFF_T_FMT" extern IDs\n", NexternIDs);

  char imageFilename[DVO_MAX_PATH];

  snprintf (imageFilename, DVO_MAX_PATH, "%s/Images.dat", CATDIR);

  if ((image = LoadImages (&db, imageFilename, &Nimage)) == NULL) {
    fprintf (stderr, "error loading images\n");
    exit (1);
  }
  fprintf (stderr, "loaded "OFF_T_FMT" images\n", Nimage);

  off_t NzeroIDs = 0;
  off_t Nfound = 0;
  off_t Nmissed = 0;
  off_t Nextmiss = 0;
  off_t Nextdups = 0;

  off_t *imageIDs = NULL;
  off_t *imageIDc = NULL;
  ALLOCATE (imageIDs, off_t, Nimage);
  ALLOCATE (imageIDc, off_t, Nimage);
  for (i = 0; i < Nimage; i++) {
    imageIDs[i] = image[i].externID;
    imageIDc[i] = 0;
  }
  osort (imageIDs, Nimage);

  for (i = 0, k = 0; (i < Nimage) && (k < NexternIDs); ) {

    // skip zero-valued imageIDs 
    if (imageIDs[i] == 0) {
      NzeroIDs ++;
      i++;
      continue;
    }

    // we have either found this one or not, but it is time to move on
    if (imageIDs[i] < externIDs[k]) { i++; continue; }

    // try the next extern ID
    if (imageIDs[i] > externIDs[k]) { k++; continue; }
    
    // we now have imageIDs[i] == externIDs[k].  
    // find all imageIDs which match this externID:

    imageIDc[i] ++;
    externIDc[k] ++;

    // by advancing imageIDs[i], I am ignoring externIDs[k] duplicates.  these are supposed to be uniq in the db
    i++;
  }

  FILE *fmiss = fopen ("missed.txt", "w");
  myAssert (fmiss, "failed to open missed.txt");

  for (i = 0; i < Nimage; i++) {
    if (imageIDs[i] == 0) continue;
    if (imageIDc[i] == 0) {
      fprintf (fmiss, OFF_T_FMT "\n", imageIDs[i]);
      Nmissed ++;
    }
    if (imageIDc[i] == 1) { Nfound ++; continue; }
    myAssert (imageIDc[i] <= 1, "impossible");
  }
  fclose (fmiss);

  fmiss = fopen ("missed.extern.txt", "w");
  myAssert (fmiss, "failed to open missed.extern.txt");

  FILE *fdups = fopen ("duplicate.txt", "w");
  myAssert (fdups, "failed to open duplicate.txt");

  for (i = 0; i < NexternIDs; i++) {
    if (externIDc[i] == 1) continue;
    if (externIDc[i] == 0) {
      fprintf (fmiss, "missing " OFF_T_FMT "\n", externIDs[i]);
      Nextmiss ++;
    }
    if (externIDc[i] > 0) {
      fprintf (fdups, "duplicate " OFF_T_FMT " " OFF_T_FMT "\n", externIDs[i], externIDc[i]);
      Nextdups ++;
    }
  }
  fclose (fdups);
  fclose (fmiss);

  fprintf (stderr, OFF_T_FMT " in catdir zero ID\n", NzeroIDs);
  fprintf (stderr, OFF_T_FMT " in catdir found\n", Nfound);
  fprintf (stderr, OFF_T_FMT " in catdir not found\n", Nmissed);
  fprintf (stderr, OFF_T_FMT " in extern not found\n", Nextmiss);
  fprintf (stderr, OFF_T_FMT " in extern duplicate\n", Nextdups);

  gfits_db_free (&db);
  free (externIDs);
  free (externIDc);
  free (imageIDs);
  free (imageIDc);

  free (CATDIR);
  free (EXTERN_ID_LIST);

  ohana_memdump (TRUE);

  exit (0);
}

// I'm loading a table of expected extern IDs and initing a table of their count
off_t load_extern_ids (char *idfile, off_t *nids) {

  // load the extern ids
  FILE *f = fopen (idfile, "r");
  myAssert (f, "failed to open idfile");

  off_t NBUFFER = 30000000;

  char *buffer;
  ALLOCATE (buffer, char, NBUFFER);

  off_t Nids = 0;
  off_t NIDS = 1000;
  ALLOCATE (externIDs, off_t, NIDS);
  ALLOCATE (externIDc, off_t, NIDS);

  off_t skipFirst = TRUE;

  off_t Nstart = 0;
  off_t Ntotal = 0;
  while (TRUE) {
    off_t Nbytes = NBUFFER - 1 - Nstart;
    bzero (&buffer[Nstart], Nbytes + 1);

    off_t Nread = fread (&buffer[Nstart], 1, Nbytes, f);

    Ntotal += Nread;
    fprintf (stderr, "reading block: " OFF_T_FMT " " OFF_T_FMT " " OFF_T_FMT " " OFF_T_FMT "\n", Nstart, Nbytes, Nread, Ntotal);

    if (ferror (f)) {
      perror ("error reading data file");
      break;
    }
    if (Nread == 0) break; // end of the file

    off_t Nlines = 0;

    int bufferStatus = TRUE; 
    char *c0 = buffer; // c0 always marks the start of a line
    while (bufferStatus) {
      char *c1 = strchr (c0, '\n'); // find the end of this current line
      if (!c1) {
	Nstart = strlen (c0);
	memmove (buffer, c0, Nstart);
	bufferStatus = FALSE;
	continue;
      }
      *c1 = 0; // mark the end of the line 
      Nlines ++;

      if (skipFirst) {
	skipFirst = FALSE;
	c0 = c1 + 1;
	continue;
      }

      off_t extern_id;
      int Nscan = sscanf (c0, OFF_T_FMT, &extern_id);
      myAssert (Nscan == 1, "invalid line");

      externIDs[Nids] = extern_id;
      externIDc[Nids] = 0;
      Nids ++;

      if (Nids == NIDS) {
	NIDS += 1000;
	REALLOCATE (externIDs, off_t, NIDS);
	REALLOCATE (externIDc, off_t, NIDS);
      }
      c0 = c1 + 1;
    }
  }
  free (buffer);
  fclose (f);

  osort (externIDs, Nids);

  *nids = Nids;
  return TRUE;
}

// return the index of the last value < threshold 
off_t externIDs_bisection (off_t *values, off_t threshold, off_t Nvalues) {

  off_t Nlo = 0; 
  off_t Nhi = Nvalues - 1;

  if (Nvalues < 1) return (-1);
  if (threshold < values[Nlo]) return (-1);

  if (Nvalues < 2) return (0);
  if (threshold > values[Nhi]) return (-1);

  off_t N;
  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (values[N] < threshold) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nvalues - 1);
    }
  }
  // values[Nlo] < threshold 
  // values[Nhi] >= threshold 

  for (N = Nlo; N < Nhi; N++) {
    if (values[N] >= threshold) {
      return (N-1);
    }
  }
  return (N);
}

void osort (off_t *value, off_t N) {

# define SWAPFUNC(A,B){ off_t tmp = value[A]; value[A] = value[B]; value[B] = tmp; }
# define COMPARE(A,B)(value[A] < value[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

