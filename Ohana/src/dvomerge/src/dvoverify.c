# include "dvoverify.h"

/* things we can verify easily:
   table sizes: (NAXIS1 vs EXTTYPE; NAXIS2 vs data size)
   sum of catalog.average.Nmeasure == catalog.Nmeasure
   averef, obj_id consistent between average and measure
   do we need a checksum?
*/

int main (int argc, char **argv) {

  SkyTable *sky;
  SkyList *skylist;

  // check various options
  SetSignals ();
  dvoverify_args (&argc, argv);
  CATDIR = argv[1];

  int Nbad = 0;

  // XXX make this step optional
  if (CHECK_TOPLEVEL) {
    char filename[DVO_MAX_PATH];

    // check the photcode table
    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/Photcodes.dat", CATDIR) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
    }

    // check the skytable
    char *skyfile = SkyTableFilename (CATDIR);
    if (!VerifyTableFile (skyfile)) {
      Nbad ++;
    }
    free (skyfile);

    // check the image table
    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/Images.dat", CATDIR) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
    }
  }

  if (CHECK_IMAGE_ID) {
    LoadImageIDs (CATDIR);
  }

  // load the sky table for the existing database
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  myAssert(sky, "can't read SkyTable");
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  skylist = SkyListByPatch (sky, -1, &UserPatch);
  
  dvoverify_catalogs (skylist, &Nbad);

  int i, Nfailures;
  char **failures = GetFailures (&Nfailures);

  fprintf (stderr, "---- files with errors ---- \n");
  for (i = 0; i < Nfailures; i++) {
    fprintf (stderr, "%s\n", failures[i]);
  }

  if (Nbad > 0) {
    fprintf (stderr, "ERROR: %d files are bad\n", Nbad);
    exit (1);
  }

  fprintf (stderr, "SUCCESS: no files are bad\n");
  if (NNotSorted) {
    fprintf (stderr, "NOTE: %d files are not sorted\n", NNotSorted);
  }

  FreeImageIDs ();
  FreeFailures ();
  SkyTableFree (sky);
  SkyListFree (skylist);

  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
  exit (0);
}

