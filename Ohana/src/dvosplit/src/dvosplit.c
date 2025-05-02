# include "dvosplit.h"

void dvosplit_free_outlist (SkyList *outlist);

// dvosplit (catdir) (outlevel) [-outdir (outcat)} [-region Rmin Rmax Dmin Dmax]
// (inherits the input catalog's format and mode, unless -set-format or -set-mode are used)
int main (int argc, char **argv) {

  int i, j, OUT_DEPTH;
  SkyTable *sky;
  SkyList *skylist = NULL;
  SkyList *outlist = NULL;
  Catalog incatalog, *outcatalogs;
  char *filename, *CATDIR;

  SetSignals ();
  ConfigInit (&argc, argv);
  args (argc, argv);

  CATDIR = strcreate (argv[1]);
  OUT_DEPTH = atoi (argv[2]);
  if (!OUTDIR) {
    OUTDIR = strcreate (CATDIR);
  }

  // load the photcode table (for Nsecfilt and related)
  char photcodeFile[1024];
  snprintf (photcodeFile, 1024, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (photcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", photcodeFile);
    exit (1);
  }

  // dvosplit can be run locally (no -outdir specified); otherwise, copy Photcode.dat,
  // Images.dat, SkyTable.fits
  int LocalCopy = !strcmp (CATDIR, OUTDIR);

  // if not local, save the photcode table (for Nsecfilt and related)
  if (!LocalCopy) {
    if (!check_dir_access (OUTDIR, VERBOSE)) {
      fprintf (stderr, "failed to create output directory %s\n", OUTDIR);
      exit (1);
    }
    snprintf (photcodeFile, 1024, "%s/Photcodes.dat", OUTDIR);
    if (!SavePhotcodesFITS (photcodeFile)) {
      fprintf (stderr, "error loading photcode table %s\n", photcodeFile);
      exit (1);
    }
  }

  // load the sky table for the existing database
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  // get the list of populated regions
  skylist  = SkyListByPatch (sky, -1, &UserPatch);
  
  for (i = 0; i < skylist[0].Nregions; i++) {
    if (VERBOSE) fprintf (stderr, "%s\n", skylist[0].regions[i][0].name);
    // fprintf (stderr, "reading from %s\n", skylist[0].regions[i][0].name);

    // if !LocalCopy, always copy the file
    // if (current level >  out level) skip: cannot currently merge catalogs
    // if (current level == out level) skip: no action is needed

    if (skylist[0].regions[i][0].depth > OUT_DEPTH) {
      if (VERBOSE) fprintf (stderr, "WARNING, cannot merge deeper catalog %s (%d vs %d)\n", skylist[0].regions[i][0].name, skylist[0].regions[i][0].depth, OUT_DEPTH);
      continue;
    }

    if (LocalCopy && (skylist[0].regions[i][0].depth == OUT_DEPTH)) continue;

    // find the list of output filenames (do not yet open)
    // we need to check if we have already done the split

    // change sky.regions[i].depth for these regions
    // outlist = SkyListByPatch (sky, OUT_DEPTH, skylist[0].regions[i]);
    outlist = SkyListChildrenByBounds (sky, skylist[0].regions[i][0].index, OUT_DEPTH,
				       skylist[0].regions[i][0].Rmin + 0.01, skylist[0].regions[i][0].Rmax - 0.01,
				       skylist[0].regions[i][0].Dmin + 0.01, skylist[0].regions[i][0].Dmax - 0.01 );

    // Modify the outlist filenames to match the output directory.  Note that the
    // filenames are now owned by the list (and are not freed by SkyListFree)
    SkyListSetFilenames (outlist, OUTDIR, "cpt");

    if (SKIP_EXIST) {
      int allFound = TRUE;
      for (j = 0; allFound && (j < outlist[0].Nregions); j++) {
	// check that outlist[0].filename[j] exists
	struct stat filestat;
	int fstatus = stat (outlist[0].filename[j], &filestat);
	if (fstatus) allFound = FALSE;
      }
      if (allFound) {
	fprintf (stderr, "skipping %s, already split\n", skylist[0].regions[i][0].name);
	dvosplit_free_outlist (outlist);
	continue;
      }
    }

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&incatalog, TRUE);
    incatalog.filename = skylist[0].filename[i];
    incatalog.Nsecfilt = GetPhotcodeNsecfilt ();
    incatalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE | DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ | DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;
    // load all of the tables at once

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&incatalog, skylist[0].regions[i], VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", incatalog.filename);
      exit (2);
    }

    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
      if (SKIP_EXIST) dvosplit_free_outlist (outlist);
      continue;
    }

    // inherit the input catalog's format and mode (unless overridden)
    outcatalogs = open_output_catalogs (outlist, incatalog.catformat, incatalog.catmode);

    split_averages (&incatalog, outlist, outcatalogs); 

    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);

    for (j = 0; j < outlist[0].Nregions; j++) {
      outcatalogs[j].sorted = TRUE; // split_averages generates a sorted database
      fprintf (stderr, "save: %s\n", outcatalogs[j].filename);
      dvo_catalog_save (&outcatalogs[j], VERBOSE);
      dvo_catalog_unlock (&outcatalogs[j]);
      dvo_catalog_free (&outcatalogs[j]);
    }
    FREE (outcatalogs);

    // adjust depth
    skylist[0].regions[i][0].table = FALSE;
    for (j = 0; j < outlist[0].Nregions; j++) {
      outlist[0].regions[j][0].table = TRUE;
    }

    // free the newly allocated filenames
    for (j = 0; j < outlist[0].Nregions; j++) {
      free (outlist[0].filename[j]);
    }

    // free the rest of the list
    SkyListFree (outlist);
  }

  // save sky table copy (Local or not Local : the depth has changed)
  filename = SkyTableFilename (OUTDIR);
  check_file_access (filename, TRUE, TRUE, VERBOSE);
  if (!SkyTableSave (sky, filename)) {
    fprintf (stderr, "ERROR: failed to save sky table for %s\n", OUTDIR);
    exit (1);
  }
  free (filename);

  if (!LocalCopy) {
    // copy the images table (there are others we should copy as well)
    char line[2048];
    snprintf (line, 2048, "cp %s/Images.dat %s/Images.dat", CATDIR, OUTDIR);
    int status = system (line);
    if (status) {
      fprintf (stderr, "ERROR: failed to copy Images.dat\n");
      exit (1);
    }
  }

  free (CATDIR);
  dvosplit_free (sky, skylist);
  exit (0);
}

void dvosplit_free_outlist (SkyList *outlist) {

  // free outlist & continue
  for (int j = 0; j < outlist[0].Nregions; j++) {
    free (outlist[0].filename[j]);
  }
  // free the rest of the list
  SkyListFree (outlist);
  return;
}
