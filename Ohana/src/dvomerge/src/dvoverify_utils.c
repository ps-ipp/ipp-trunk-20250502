# include "dvoverify.h"

static int Nfailures = 0;
static int NFAILURES = 100;
static char **failures = NULL;

void InitFailures (void) {
  ALLOCATE_ZERO (failures, char *, NFAILURES);
}

void FreeFailures (void) {
  int i;

  if (!failures) return;
  for (i = 0; i < NFAILURES; i++) {
    FREE (failures[i]);
  }
  FREE (failures);
}

void AddFailures (char *filename) {
  failures[Nfailures] = strcreate (filename);
  Nfailures ++;
  CHECK_REALLOCATE (failures, char *, NFAILURES, Nfailures, 100);
}

char **GetFailures (int *N) {
  *N = Nfailures;
  return failures;
}

int dvoverify_single (char *filename) {

  int isGood = TRUE;

  if (!VerifyTableFile (filename)) {
    fprintf (stderr, "bad average table %s\n", filename);
    isGood = FALSE;
  }

  if (!CheckCatalogIndexes(filename, NULL)){
    fprintf (stderr, "bad indexes in %s\n", filename);
    isGood = FALSE;
  }

  // change last 't' to 's':
  int Nlast;
  Nlast = strlen(filename) - 1;
  filename[Nlast] = 's';
  if (!VerifyTableFile (filename)) {
    fprintf (stderr, "bad secfilt table %s\n", filename);
    isGood = FALSE;
  }

  // change last 't' to 's':
  Nlast = strlen(filename) - 1;
  filename[Nlast] = 'm';
  if (!VerifyTableFile (filename)) {
    fprintf (stderr, "bad measure table %s\n", filename);
    isGood = FALSE;
  }

  return (isGood);
}

// is this file a consistent FITS file?
// note that VerifyTableFile only has to read the headers,
// not the data blocks (it uses stat for sizes)
int VerifyTableFile (char *filename) {

  int status, Next;
  off_t Nbytes;
  Header header;

  struct stat fileStats;
  FILE *file;

  // does the file exist?
  status = stat (filename, &fileStats);
  if (status) {
    // some error accessing the file.  there is only one acceptable error: file not found
    switch (errno) {
      case ENOENT:
	if (DEBUG) fprintf (stderr, "file does not exist, skipping %s\n", filename);
	if (LIST_MISSING) return FALSE;
	return TRUE;
      case ENOMEM:
	fprintf (stderr, "Out of memory: %s\n", filename);
	return TRUE;
      case EACCES:
	fprintf (stderr, "Permission error on %s\n", filename);
	return FALSE;
      case EFAULT:
	fprintf (stderr, "Bad address: %s\n", filename);
	return FALSE;
      case ELOOP:
	fprintf (stderr, "Too many symbolic links encountered while traversing the path: %s\n", filename);
	return FALSE;
      case ENAMETOOLONG:
	fprintf (stderr, "File name too long: %s\n", filename);
	return FALSE;
      case ENOTDIR:
	fprintf (stderr, "A component of the path is not a directory: %s\n", filename);
	return FALSE;
      case EOVERFLOW:
	fprintf (stderr, "file too large for program version: %s\n", filename);
	return FALSE;
      default:
	fprintf (stderr, "unknown error: %s\n", filename);
	return FALSE;
    }
  }

  // does it have any data?
  if (fileStats.st_size == 0) {
    fprintf (stderr, "file is empty: %s\n", filename);
    return FALSE;
  }

  // can we open it?
  file = fopen(filename, "r");
  if (!file) {
    fprintf (stderr, "unable to open valid file: %s\n", filename);
    return FALSE;
  }

  // scan all extentions
  Nbytes = 0;
  Next = -1;
  if (DEBUG) fprintf (stderr, "sizes: ("OFF_T_FMT" vs "OFF_T_FMT")\n", Nbytes, fileStats.st_size);
  while (Nbytes < fileStats.st_size) {

    // Check on the PHU
    if (!gfits_fread_header (file, &header)) {
      if (Next == -1) {
	fprintf (stderr, "unable to read PHU header for %s\n", filename);
      } else {
	fprintf (stderr, "unable to read header for %s, extension %d (or file has excess bytes)\n", filename, Next);
      }
      fclose (file);
      gfits_free_header (&header);
      return (FALSE);
    }

    // move to TBL header
    Nbytes += header.datasize + gfits_data_size (&header);
    if (DEBUG) fprintf (stderr, "sizes: ("OFF_T_FMT" vs "OFF_T_FMT")\n", Nbytes, fileStats.st_size);
    if (Nbytes > fileStats.st_size) {
      fprintf (stderr, "file is short ("OFF_T_FMT" vs "OFF_T_FMT"): %s\n", Nbytes, fileStats.st_size, filename);
      gfits_free_header(&header);
      fclose (file);
      return FALSE;
    }
    gfits_free_header(&header);

    status = fseeko (file, Nbytes, SEEK_SET);
    if (status) {
      switch (errno) {
	case EBADF:
	  fprintf (stderr, "something wrong with file handle: %s\n", filename);
	  fclose (file);
	  return FALSE;
	case EINVAL:
	  fprintf (stderr, "invalid offset: %s\n", filename);
	  fclose (file);
	  return FALSE;
	default:
	  fprintf (stderr, "other error in fseeko: %s\n", filename);
	  fclose (file);
	  return FALSE;
      }
    }
    Next ++;
  }
  if (DEBUG) fprintf (stderr, "file is good: %s\n", filename);
  fclose (file);
  return TRUE;
}

int CheckCatalogIndexes (char *filename,  SkyRegion *region) {

  Catalog catalog;
  int i, j, m, status;

  status = TRUE;

  // set the parameters which guide catalog open/load/create
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename  = filename;
  catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ | DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;
  catalog.Nsecfilt  = 0;
  
  // an error exit status here is a significant error (disk I/O or file access)
  if (!dvo_catalog_open (&catalog, region, VERBOSE, "r")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
    dvo_catalog_free (&catalog);
    return FALSE;
  }

  // Naverage_disk == 0 implies an empty catalog file, skip empty catalogs
  if (catalog.Naverage_disk == 0) {
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    return TRUE;
  }

  // if the table is SORTED, then the following can be checked
  // check the following:
  // measure[j].averef -> average[averef]
  // measure[j].objID = average[averef].objID
  // measure[j].catID = average[averef].catID
  // measure[j].measureOffset < Nmeasure
  // \sum average[].Nmeasure = Nmeasure

  // if the table is NOT SORTED, do we have a subset of checks we can make?
  if (!catalog.sorted && !IGNORE_SORTED_STATE) {
    fprintf (stderr, "!");
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    NNotSorted++;
    if (VERBOSE) fprintf (stderr, "file is not sorted: %s\n", filename);
    return TRUE;
  }
  
# define CHECK_TABLE(NAME) {						\
    int Ntotal = 0; int offsetOK = TRUE; int isOK = FALSE;		\
    for (i = 0; i < catalog.Naverage; i++) {				\
      Ntotal += catalog.average[i].N##NAME;				\
      if (VERBOSE && !(Ntotal <= catalog.N##NAME)) {			\
	fprintf (stderr, "Ntotal (%s) > catalog.N##NAME: %d %d : %d > %d\n", #NAME, i, Ntotal, catalog.average[i].N##NAME, (int) catalog.N##NAME); \
      }									\
      isOK = (catalog.average[i].N##NAME == 0) || (catalog.average[i].NAME##Offset < catalog.N##NAME); \
      offsetOK &= isOK;							\
      if (VERBOSE && !isOK) {						\
	fprintf (stderr, "%sOffset >= catalog.N%s: %d : %d >= %d\n", #NAME, #NAME, i, catalog.average[i].NAME##Offset, (int) catalog.N##NAME); \
      }									\
      isOK = (catalog.average[i].N##NAME == 0) || (catalog.average[i].NAME##Offset + catalog.average[i].N##NAME <= catalog.N##NAME); \
      offsetOK &= isOK;							\
      if (VERBOSE && !isOK) {						\
	fprintf (stderr, "%sOffset + N%s > catalog.N%s : %d : %d + %d > %d\n", #NAME, #NAME, #NAME, i, catalog.average[i].N##NAME, catalog.average[i].NAME##Offset, (int) catalog.N##NAME); \
      } }								\
    if (!offsetOK) {							\
      fprintf (stderr, "ERROR: catalog %s has an invalid %sOffset\n", catalog.filename, #NAME); \
      status = FALSE;							\
    }									\
    if (Ntotal != catalog.N##NAME) {					\
      fprintf (stderr, "ERROR: catalog %s has an invalid N%s\n", catalog.filename, #NAME); \
      status = FALSE;							\
    }									\
  }
  
  CHECK_TABLE(measure);
  CHECK_TABLE(lensing);
  CHECK_TABLE(lensobj);
  CHECK_TABLE(starpar);
  CHECK_TABLE(galphot);

  // if we have a problem with Nmeasure and/or measureOffset values, we
  // cannot do any further check -- we risk segfaults
  if (!status) {
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    return (status);
  }

  // check measure <-> average links
  {
    int NobjIDsBAD = 0;
    int NcatIDsBAD = 0;
    int NaverefBAD = 0;

    for (i = 0; i < catalog.Naverage; i++) {
      m = catalog.average[i].measureOffset;
      for (j = 0; j < catalog.average[i].Nmeasure; j++) {
	if (catalog.average[i].objID != catalog.measure[m+j].objID) {
	  NobjIDsBAD ++;

	  // check if objID matches R,D
	  Measure *measure = &catalog.measure[m+j];
	  int iTest = measure->objID;
	  if (catalog.average[iTest].objID == iTest) {
	    double dR = 3600.0*(catalog.average[iTest].R - measure->R) * cos (RAD_DEG*measure->D);
	    double dD = 3600.0*(catalog.average[iTest].D - measure->D);
	    double dRad = hypot (dR, dD);
	    char *date = ohana_sec_to_date (measure->t);
	    fprintf (stderr, "matches ave %d = %d, %s : %6.3f %5d : %f, %f = %f\n", iTest, measure->objID, date, measure->M, measure->photcode, dR, dD, dRad);
	    free (date);
	  } else {
	    fprintf (stderr, "cannot find averef\n");
	  }
	}
	if (catalog.average[i].catID != catalog.measure[m+j].catID) {
	  NcatIDsBAD ++;
	}
	if (catalog.measure[m+j].averef != i) {
	  NaverefBAD ++;
	}
      }
    }
    
    if (NobjIDsBAD) {
      fprintf (stderr, "ERROR: catalog %s has %d invalid obj IDs\n", catalog.filename, NobjIDsBAD);
      status = FALSE;
    }
    if (NcatIDsBAD) {
      fprintf (stderr, "ERROR: catalog %s has %d invalid cat IDs\n", catalog.filename, NcatIDsBAD);
      status = FALSE;
    }
    if (NaverefBAD) {
      fprintf (stderr, "ERROR: catalog %s has %d invalid averef values\n", catalog.filename, NaverefBAD);
      status = FALSE;
    }
  }

  // check lensing <-> average links
  {
    int objIDsOK = TRUE;
    int catIDsOK = TRUE;
    int averefOK = TRUE;

    for (i = 0; i < catalog.Naverage; i++) {
      m = catalog.average[i].lensingOffset;
      for (j = 0; j < catalog.average[i].Nlensing; j++) {
	objIDsOK &= (catalog.average[i].objID == catalog.lensing[m+j].objID);
	catIDsOK &= (catalog.average[i].catID == catalog.lensing[m+j].catID);
	averefOK &= (catalog.lensing[m+j].averef == i);
      }
    }
    
    if (!objIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid lensing obj IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!catIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid lensing cat IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!averefOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid lensing averef values\n", catalog.filename);
      status = FALSE;
    }
  }

  // check lensobj <-> average links
  {
    int objIDsOK = TRUE;
    int catIDsOK = TRUE;

    for (i = 0; i < catalog.Naverage; i++) {
      m = catalog.average[i].lensobjOffset;
      for (j = 0; j < catalog.average[i].Nlensobj; j++) {
	if ((catalog.lensobj[m+j].objID == 0xffffffff) && (catalog.lensobj[m+j].catID == 0xffffffff)) continue;
	objIDsOK &= (catalog.average[i].objID == catalog.lensobj[m+j].objID);
	catIDsOK &= (catalog.average[i].catID == catalog.lensobj[m+j].catID);
	if (!(catalog.average[i].objID == catalog.lensobj[m+j].objID)) {
	  fprintf (stderr, "!");
	}
	if (!(catalog.average[i].catID == catalog.lensobj[m+j].catID)) {
	  fprintf (stderr, "?");
	}
      }
    }
    
    if (!objIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid lensobj obj IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!catIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid lensobj cat IDs\n", catalog.filename);
      status = FALSE;
    }
  }

  // check starpar <-> average links
  {
    int objIDsOK = TRUE;
    int catIDsOK = TRUE;
    int averefOK = TRUE;

    for (i = 0; i < catalog.Naverage; i++) {
      m = catalog.average[i].starparOffset;
      for (j = 0; j < catalog.average[i].Nstarpar; j++) {
	objIDsOK &= (catalog.average[i].objID == catalog.starpar[m+j].objID);
	catIDsOK &= (catalog.average[i].catID == catalog.starpar[m+j].catID);
	averefOK &= (catalog.starpar[m+j].averef == i);
      }
    }
    
    if (!objIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid starpar obj IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!catIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid starpar cat IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!averefOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid starpar averef values\n", catalog.filename);
      status = FALSE;
    }
  }

  // check galphot <-> average links
  {
    int objIDsOK = TRUE;
    int catIDsOK = TRUE;
    int averefOK = TRUE;

    for (i = 0; i < catalog.Naverage; i++) {
      m = catalog.average[i].galphotOffset;
      for (j = 0; j < catalog.average[i].Ngalphot; j++) {
	objIDsOK &= (catalog.average[i].objID == catalog.galphot[m+j].objID);
	catIDsOK &= (catalog.average[i].catID == catalog.galphot[m+j].catID);
	averefOK &= (catalog.galphot[m+j].averef == i);
      }
    }
    
    if (!objIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid galphot obj IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!catIDsOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid galphot cat IDs\n", catalog.filename);
      status = FALSE;
    }
    if (!averefOK) {
      fprintf (stderr, "ERROR: catalog %s has invalid galphot averef values\n", catalog.filename);
      status = FALSE;
    }
  }

  // check the image ID here?
  if (CHECK_IMAGE_ID) {
    int Nfail = CheckImageID (&catalog);
    if (Nfail > 0) {
      fprintf (stderr, "ERROR: catalog %s has invalid %d unmatched image IDs\n", catalog.filename, Nfail);
      status = FALSE;
    }
  }

  dvo_catalog_unlock (&catalog);
  dvo_catalog_free (&catalog);

  return status;
}

static int maxID = 0;
static int *IDlist = NULL;

// check that every measure->imageID (if set) matches an existing 
// image->ID.  return the number of failures.
int CheckImageID (Catalog *catalog) {

  off_t i, j, m, id;
  int Nfail = 0;

  for (i = 0; i < catalog[0].Naverage; i++) {
    m = catalog[0].average[i].measureOffset;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {
      id = catalog[0].measure[m+j].imageID;
      if (id == 0) continue; // detections from ref photcodes can (should) have unset image IDs
      if (id > maxID) {
	Nfail ++;
	continue;
	// is this sufficient to catch IDs set without an image table?
      }
      if (IDlist) {
	if (IDlist[id] < 0) {
	  Nfail ++;
	  continue;
	}
      } else {
	if (id > 0) {
	  Nfail ++;
	  continue;
	}
      }
    }
  }

  for (i = 0; i < catalog[0].Naverage; i++) {
    m = catalog[0].average[i].lensingOffset;
    for (j = 0; j < catalog[0].average[i].Nlensing; j++) {
      id = catalog[0].lensing[m+j].imageID;
      if (id == 0) continue; // detections from ref photcodes can (should) have unset image IDs
      if (id > maxID) {
	Nfail ++;
	continue;
	// is this sufficient to catch IDs set without an image table?
      }
      if (IDlist) {
	if (IDlist[id] < 0) {
	  Nfail ++;
	  continue;
	}
      } else {
	if (id > 0) {
	  Nfail ++;
	  continue;
	}
      }
    }
  }

  return Nfail;
}

int LoadImageIDs (char *catdir) {

  int status;
  off_t Nimages, i;
  Image *images;
  FITS_DB inDB;

  char ImageCat[DVO_MAX_PATH];
  myAssert (snprintf (ImageCat, DVO_MAX_PATH, "%s/Images.dat", catdir) < DVO_MAX_PATH, "overflow");

  // load the iage database table
  gfits_db_init (&inDB);
  status = dvo_image_lock (&inDB, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
  if (!status) {
    fprintf (stderr, "ERROR: failure to lock image catalog %s", inDB.filename);
    exit (3);
  }

  // load the image table 
  if (inDB.dbstate == LCK_EMPTY) {
    dvo_image_unlock (&inDB); // unlock input
    // this is not an error: we can have no image table for, eg, 2MASS only db
    return TRUE;
  }
  if (!dvo_image_load (&inDB, VERBOSE, TRUE)) {
    fprintf (stderr, "can't read input image catalog %s", inDB.filename);
    exit (4);
  }

  images = gfits_table_get_Image (&inDB.ftable, &Nimages, &inDB.scaledValue, &inDB.nativeOrder);
  if (!images) {
    fprintf (stderr, "ERROR: failed to read images from src\n");
    exit (2);
  }

  // generate a lookup table for the images
  
  // first, find the max imageID
  for (i = 0; i < Nimages; i++) {
    maxID = MAX(maxID, images[i].imageID);
  }

  ALLOCATE (IDlist, int, maxID + 1);
  for (i = 0; i < maxID + 1; i++) {
    IDlist[i] = -1;
  }

  for (i = 0; i < Nimages; i++) {
    int id = images[i].imageID;
    IDlist[id] = i;
  }
  
  // (in the future, I'll have to do the image table read in segments
  // it is just getting to be too large...)
  dvo_image_unlock (&inDB); // unlock input
  gfits_db_free (&inDB);

  return TRUE;
}

void FreeImageIDs (void) {
  FREE (IDlist);
}

# define GET_COLUMN(OUT,NAME,TYPE) \
  OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

// write out the IDmap data for clients to read
int SaveImageIDsSmall(char *filename) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "IMAGE_IDS");

  gfits_define_bintable_column (&theader, "J", "IMAGE_IDS", "image IDs", NULL, 1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "IMAGE_IDS", IDlist, maxID + 1);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image ID file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for image ID file %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for image ID file %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for image ID file %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for image ID file %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file image ID file %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file image ID file %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing image ID file file %s\n", filename);

  return TRUE;
}

int LoadImageIDsSmall (char *filename) {

  int Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return FALSE;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return FALSE;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return FALSE;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return FALSE;
  }

  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return FALSE;
  }
  fclose (f);

  char type[16];

  GET_COLUMN (IDlist, "IMAGE_IDS", int);
  maxID = Nrow - 1;
  fprintf (stderr, "loaded data for %lld images\n", (long long) Nrow);

  return TRUE;
}

