# include "dvosplit.h"

// dvosplit (catdir) (outlevel) [-outdir (outcat)} [-region Rmin Rmax Dmin Dmax]
// (inherits the input catalog's format and mode, unless -set-format or -set-mode are used)
int main (int argc, char **argv) {

  int i, j, OUT_DEPTH;
  SkyTable *sky;
  SkyList *skylist, *outlist;
  Catalog incatalog, *outcatalogs;
  AveLinks *avelinks;
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

    // if !LocalCopy, always copy the file
    // if (current level >  out level) skip: cannot currently merge catalogs
    // if (current level == out level) skip: no action is needed

    if (skylist[0].regions[i][0].depth > OUT_DEPTH) {
      if (VERBOSE) fprintf (stderr, "WARNING, cannot merge deeper catalog %s (%d vs %d)\n", skylist[0].regions[i][0].name, skylist[0].regions[i][0].depth, OUT_DEPTH);
      continue;
    }

    if (LocalCopy && (skylist[0].regions[i][0].depth == OUT_DEPTH)) continue;

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&incatalog, TRUE);
    incatalog.filename = skylist[0].filename[i];
    incatalog.Nsecfilt = GetPhotcodeNsecfilt ();
    incatalog.catflags = DVO_LOAD_NONE;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&incatalog, skylist[0].regions[i], VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", incatalog.filename);
      exit (2);
    }

    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
      continue;
    }

    // change sky.regions[i].depth for these regions
    outlist = SkyListByPatch (sky, OUT_DEPTH, skylist[0].regions[i]);

    // Modify the outlist filenames to match the output directory.  Note that the
    // filenames are now owned by the list (and are not freed by SkyListFree)
    SkyListSetFilenames (outlist, OUTDIR, "cpt");

    // inherit the input catalog's format and mode (unless overridden)
    outcatalogs = open_output_catalogs (outlist, incatalog.catformat, incatalog.catmode);

    avelinks = split_averages (&incatalog, outlist, outcatalogs); 

    split_measures (&incatalog, outlist, outcatalogs, avelinks); 

    // XXX missing entries have to be reconstructed if they are desired
    // split_missings (&incatalog, outlist, outcatalogs, avelinks); 

    free (avelinks[0].outref);
    free (avelinks[0].outcat);

    dvo_catalog_unlock (&incatalog);

    for (j = 0; j < outlist[0].Nregions; j++) {
      dvo_catalog_unlock (&outcatalogs[j]);
    }

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

  if (!LocalCopy) {
    // copy the images table
    char line[2048];
    snprintf (line, 2048, "cp %s/Images.dat %s/Images.dat", CATDIR, OUTDIR);
    int status = system (line);
    if (status) {
      fprintf (stderr, "ERROR: failed to copy Images.dat\n");
      exit (1);
    }
  }

  exit (0);
}
