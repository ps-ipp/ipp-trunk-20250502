# include "dvoutils.h"

# ifndef MAX_INT
# define MAX_INT 2147483647
# endif

int dvoutils_uniq_images(char *filename) {

  // given a list of Images.dat tables, find the unique subset of extern_id
  // format of the input list is just (dirname)/Images.dat.  commented lines
  // starting with # are allowed

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "cannot read input list %s\n", filename);
    exit (1);
  }

  int    minID  = -1;
  int    maxID  = -1;
  int  *IDlist  = NULL;
  int  *DBfound = NULL;
  int  *DBindex = NULL;
  
  int Ndb = 0;
  int NDB = 100;
  char **DBlist = NULL;
  ALLOCATE (DBlist, char *, NDB); // list of the names of the image database tables.  

  char imFile[1024];
  while (scan_line_maxlen (f, imFile, 1024) != EOF) {

    stripwhite (imFile);

    if (imFile[0] == '#') continue;

    if (strchr(imFile, ' ') != NULL) {
      fprintf (stderr, "line has extra spaces\n%s\n", imFile);
      exit (2);
    }

    ImageData *imdata = dvoutils_load_image_index (imFile);
    if (!imdata) continue;

    DBlist[Ndb] = strcreate (imFile);

    // now loop over all of the images and accumulate the array of extern_id values (skip
    // the ones with extern_id == 0)

    // things I need to track:
    // minID, maxID (only allocate an array of length maxID - minID + 1 + padding)

    int myMaxID = 0;
    int myMinID = MAX_INT;

    int i;
    int Nbad = 0;
    for (i = 0; i < imdata->Nimages; i++) {
      if (!imdata->photcode[i] && imdata->externID[i]) myAbort ("invalid combo");

      if (!imdata->photcode[i]) continue;
      if (!imdata->externID[i]) {
	Nbad ++;
	continue;
      }
      myMaxID = MAX(imdata->externID[i], myMaxID);
      myMinID = MIN(imdata->externID[i], myMinID);
    }

    fprintf (stderr, "read %s, %d images, %d - %d vs %d - %d\n", imFile, (int) imdata->Nimages, myMinID, myMaxID, minID, maxID);

    if (Nbad) {
      fprintf (stderr, "WARNING: %d images without extern_id set\n", (int) Nbad);
    }

    // if this is new, we treat it a bit differently
    if (!IDlist) {
      int Nindex = myMaxID - myMinID + 1;
      ALLOCATE (IDlist,  int,  Nindex); // number of times this ID is seen.  IDlist[i] corresponds to extern_id = i + minID
      ALLOCATE (DBfound, int,  Nindex); // index of first DB in which this ID is seen (if n = DBfound[i], the correspond ID was seen in DB n
      ALLOCATE (DBindex, int,  Nindex); // sequence of this image in the first DB for which this ID was found

      for (i = 0; i < Nindex; i++) {
	IDlist[i]  =  0;
	DBfound[i] = -1;
	DBindex[i] = -1;
      }

      minID = myMinID;
      maxID = myMaxID;
    } else {
      int newMinID = MIN(myMinID, minID);
      int newMaxID = MAX(myMaxID, maxID);

      int Nindex = newMaxID - newMinID + 1;

      int oldNindex = maxID - minID + 1;
      myAssert (oldNindex <= Nindex, "impossible!");

      int *newIDlist  = NULL;
      int *newDBfound = NULL;
      int *newDBindex = NULL;

      ALLOCATE (newIDlist,  int, Nindex);
      ALLOCATE (newDBfound, int, Nindex);
      ALLOCATE (newDBindex, int, Nindex);

      for (i = 0; i < Nindex; i++) {
	newIDlist[i]  =  0;
	newDBfound[i] = -1;
	newDBindex[i] = -1;
      }
      
      // if myMinID >= minID, newMinID = minID -> offset = 0
      // if myMinID <  minID, newMinID = myMinID -> offset = minID - myMinID
      int offset = minID - newMinID;

      // save the old values
      memcpy (&newIDlist [offset], IDlist,  oldNindex*sizeof(int));
      memcpy (&newDBfound[offset], DBfound, oldNindex*sizeof(int));
      memcpy (&newDBindex[offset], DBindex, oldNindex*sizeof(int));

      free (IDlist);
      free (DBfound);
      free (DBindex);

      IDlist  = newIDlist;
      DBfound = newDBfound;
      DBindex = newDBindex;

      minID = newMinID;
      maxID = newMaxID;
    }

    for (i = 0; i < imdata->Nimages; i++) {
      if (!imdata->externID[i]) continue;
      int n = imdata->externID[i] - minID;
      if (IDlist[n] == 0) {
	DBfound[n] = Ndb;
	DBindex[n] = i;
      }
      IDlist[n] ++;
      if (VERBOSE && (IDlist[n] > 1)) {
	fprintf (stderr, "duplicate ext_id %d in file %s\n", imdata->externID[i], imFile);
      }
    }

    if (0) {
      fprintf (stderr, "------------ %s ----------\n", imFile);

      for (i = 0; i < imdata->Nimages; i++) {
	if (!imdata->externID[i]) continue;
	fprintf (stderr, "%d => %d\n", imdata->externID[i], imdata->externID[i] - minID);
      }

      int Nindex = maxID - minID + 1;
      
      for (i = 0; i < Nindex; i++) {
	if (!IDlist[i]) continue;
	fprintf (stderr, "%d : %d : %d\n", i, i + minID, IDlist[i]);
      }
    }

    // we are done with the db file, bump the counter
    Ndb ++;
    CHECK_REALLOCATE (DBlist, char *, NDB, Ndb, 100);
  }

  int Nindex = maxID - minID + 1;

  int i;
  for (i = 0; i < Nindex; i++) {
    if (IDlist[i] < 2) continue;
    int n = DBfound[i];
    myAssert (n > -1, "impossible!");
    fprintf (stdout, "primary %d : %d -- in %s (%d) : %d \n", i + minID, IDlist[i], DBlist[n], DBfound[i], DBindex[i]);
  }

  exit (0);
}

