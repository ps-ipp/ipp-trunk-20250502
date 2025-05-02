# include "dvomerge.h"

static int *warpIDs = NULL;
static int *projIDs = NULL;
static int *cellIDs = NULL;
static int *fileIDs = NULL;

# define STRFAIL { fprintf (stderr, "failure on image %s\n", image[i].name); continue; }

int load_warp_ids (char *idfile, int *nfiles);
int warpIDs_bisection (int *warpIDs, int threshold, int Nvalues);

int dvorepairFixWarpIDs (int argc, char **argv) {

  FITS_DB db;  // database handle pointing to input image table

  off_t Nimage;
  
  Image *image;

  if (argc != 3) {
    // NOTE: mode has been stripped
    fprintf (stderr, "USAGE: dvorepair -fix-warp-ids (catdir.list) (warp.ids)\n");
    fprintf (stderr, "  catdir.list : list of databases of interest\n");
    fprintf (stderr, "  warp.ids : list of warp_id, skycell.id, warp_skyfile_id map\n");
    exit (2);
  }

  char *catdir_list  = argv[1];
  char *idfile = argv[2];

  // load the warp_id table
  int Nfiles = 0;
  load_warp_ids (idfile, &Nfiles);
  fprintf (stderr, "loaded %d warp,proj,cell,file matches\n", Nfiles);

  char name[DVO_MAX_PATH];
  char catdir[DVO_MAX_PATH];
  char imageFilenameOld[DVO_MAX_PATH];
  char imageFilenameNew[DVO_MAX_PATH];

  // read the list of catdirs and fix image tables for each 

  FILE *f = fopen (catdir_list, "r");
  myAssert (f, "failed to open catdir.list %s\n", catdir_list);

  while (fscanf (f, "%s", catdir) != EOF) {
    snprintf_nowarn (imageFilenameOld, DVO_MAX_PATH, "%s/Images.dat", catdir);
    snprintf_nowarn (imageFilenameNew, DVO_MAX_PATH, "%s/Images.dat.fixed", catdir);
    
    if ((image = LoadImages (&db, imageFilenameOld, &Nimage)) == NULL) {
      fprintf (stderr, "error loading images\n");
      exit (1);
    }

    int i, j;
    for (i = 0; i < Nimage; i++) {
      if (image[i].externID) continue;
      
      strcpy (name, image[i].name);
    
      char *p0 = strchr (name  , '.'); if (!p0) STRFAIL;
      char *p1 = strchr (p0 + 1, '.'); if (!p1) STRFAIL;
      char *p2 = strchr (p1 + 1, '.'); if (!p2) STRFAIL;
      char *p3 = strchr (p2 + 1, '.'); if (!p3) STRFAIL;
      char *p4 = strchr (p3 + 1, '.'); if (!p4) STRFAIL;
      char *p5 = strchr (p4 + 1, '.'); if (!p5) STRFAIL;
      char *p6 = strchr (p5 + 1, '.'); if (!p6) STRFAIL;
    
      *p6 = 0;
      int myWarpID = atoi (p5 + 1);

      *p3 = 0;
      *p4 = 0;
      int myProjID = atoi (p2 + 1);
      int myCellID = atoi (p3 + 1);

      int Nlo = warpIDs_bisection (warpIDs, myWarpID, Nfiles);

      int found = FALSE;
      for (j = Nlo; !found && (j < Nfiles) && (warpIDs[j] <= myWarpID); j++) {
	if (warpIDs[j] != myWarpID) continue;
	if (projIDs[j] != myProjID) continue;
	if (cellIDs[j] != myCellID) continue;

	// fprintf (stderr, "found it! (%d,%d,%d) = (%d,%d,%d) : %d\n", myWarpID, myProjID, myCellID, warpIDs[j], projIDs[j], cellIDs[j], fileIDs[j]);

	image[i].externID = fileIDs[j];
	image[i].sourceID = 34;

	found = TRUE;
      }
      if (!found) {
	// fprintf (stderr, "did NOT find it! (%d,%d,%d)\n", myWarpID, myProjID, myCellID);
      }
    }
    SaveImages(&db, imageFilenameNew, image, Nimage);
    gfits_db_free (&db);
  }
  free (warpIDs);
  free (projIDs);
  free (cellIDs);
  free (fileIDs);

  fclose (f);
  ohana_memdump (TRUE);

  exit (0);
}

int load_warp_ids (char *idfile, int *nfiles) {

  // load the warp ids
  FILE *f = fopen (idfile, "r");
  myAssert (f, "failed to open warp file");

  int NBUFFER = 30000000;

  char *buffer;
  ALLOCATE (buffer, char, NBUFFER);

  int Nfiles = 0;
  int NFILES = 1000;
  ALLOCATE (warpIDs, int, NFILES);
  ALLOCATE (projIDs, int, NFILES);
  ALLOCATE (cellIDs, int, NFILES);
  ALLOCATE (fileIDs, int, NFILES);

  int skipFirst = TRUE;

  int Nstart = 0;
  int Ntotal = 0;
  while (TRUE) {
    int Nbytes = NBUFFER - 1 - Nstart;
    bzero (&buffer[Nstart], Nbytes + 1);

    int Nread = fread (&buffer[Nstart], 1, Nbytes, f);

    Ntotal += Nread;
    fprintf (stderr, "reading block: %d %d %d %d\n", Nstart, Nbytes, Nread, Ntotal);

    if (ferror (f)) {
      perror ("error reading data file");
      break;
    }
    if (Nread == 0) break; // end of the file

    int Nlines = 0;

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

      int warp_id, proj_id, cell_id, file_id;
      int Nscan = sscanf (c0, "%d skycell.%d.%d %d", &warp_id, &proj_id, &cell_id, &file_id);
      myAssert (Nscan == 4, "invalid line");

      warpIDs[Nfiles] = warp_id;
      projIDs[Nfiles] = proj_id;
      cellIDs[Nfiles] = cell_id;
      fileIDs[Nfiles] = file_id;
      Nfiles ++;

      if (Nfiles == NFILES) {
	NFILES += 1000;
	REALLOCATE (warpIDs, int, NFILES);
	REALLOCATE (projIDs, int, NFILES);
	REALLOCATE (cellIDs, int, NFILES);
	REALLOCATE (fileIDs, int, NFILES);
      }
      c0 = c1 + 1;
    }
  }
  free (buffer);
  fclose (f);

  isortfour (warpIDs, projIDs, cellIDs, fileIDs, Nfiles);

  *nfiles = Nfiles;
  return TRUE;
}

// return the index of the last value < threshold 
int warpIDs_bisection (int *values, int threshold, int Nvalues) {

  int Nlo = 0; 
  int Nhi = Nvalues - 1;

  if (Nvalues < 1) return (-1);
  if (threshold < values[Nlo]) return (-1);

  if (Nvalues < 2) return (0);
  if (threshold > values[Nhi]) return (-1);

  int N;
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
