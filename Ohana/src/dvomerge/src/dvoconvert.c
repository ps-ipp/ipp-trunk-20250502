# include "dvomerge.h"
int dvoConvert_copy_images (char *input, char *output);

int main (int argc, char **argv) {

  char filename[256], *input, *output;

  int depth, Nsecfilt;
  off_t i, j, Ns, Ne;
  SkyTable *outsky, *insky;
  SkyList *inlist;
  Catalog incatalog, outcatalog;

  SetSignals ();
  dvoconvert_help (argc, argv);
  ConfigInit (&argc, argv);
  dvoconvert_args (&argc, argv);

  if (strcasecmp (argv[2], "to")) dvoconvert_usage();
  input = argv[1];
  output = argv[3];

  // load input images, save to output images
  dvoConvert_copy_images (input, output);

  // copy photcode table
  { 
    // the first input defines the photcode table & db layout
    sprintf (filename, "%s/Photcodes.dat", input);
    if (!LoadPhotcodes (filename, NULL, FALSE)) {
      fprintf (stderr, "error loading photcode table %s\n", filename);
      exit (1);
    }
    Nsecfilt = GetPhotcodeNsecfilt();

    // save the photcodes in the output catdir
    sprintf (filename, "%s/Photcodes.dat", output);
    if (!check_file_access (filename, TRUE, TRUE, VERBOSE)) {
      fprintf (stderr, "error creating output catdir %s\n", output);
      exit (1);
    }
    if (!SavePhotcodesFITS (filename)) {
      fprintf (stderr, "error saving photcode table %s\n", filename);
      exit (1);
    }
  }

  // copy skytable
  { 
    // load the sky table for the existing database
    insky = SkyTableLoadOptimal (input, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
    SkyTableSetFilenames (insky, input, "cpt");

    // generate an output table populated at the desired depth
    // XXX force this to match the input?
    outsky = SkyTableLoadOptimal (output, NULL, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
    SkyTableSetFilenames (outsky, output, "cpt");

    SkyTablePopulatedRange (&Ns, &Ne, insky, 0);
    depth = insky[0].regions[Ns].depth;
    // XXX this seems to imply that insky is a uniform depth...
  }

  // loop over all input catalogs, save to output catalogs
  // loop over the populatable output regions
  for (i = 0; i < outsky[0].Nregions; i++) {
    if (!outsky[0].regions[i].table) continue;
    if (VERBOSE) fprintf (stderr, "output: %s\n", outsky[0].regions[i].name);

    // load / create output catalog
    LoadCatalog (&outcatalog, &outsky[0].regions[i], outsky[0].filename[i], "w", Nsecfilt);

    // combine only tables at equal or larger depth
      
    // load in all of the tables from input for this region
    inlist = SkyListByBounds (insky, depth, outsky[0].regions[i].Rmin, outsky[0].regions[i].Rmax, outsky[0].regions[i].Dmin, outsky[0].regions[i].Dmax);
    for (j = 0; j < inlist[0].Nregions; j++) {
      if (VERBOSE) fprintf (stderr, "input : %s\n", inlist[0].regions[j][0].name);

      // load input catalog
      LoadCatalog (&incatalog, inlist[0].regions[j], inlist[0].filename[j], "r", Nsecfilt);

      // skip empty input catalogs
      if (!incatalog.Naverage_disk) {
	dvo_catalog_unlock (&incatalog);
	dvo_catalog_free (&incatalog);
	continue;
      }
      merge_catalogs_new (&outsky[0].regions[i], &outcatalog, &incatalog, NULL);
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
    }
    SkyListFree (inlist);

    outcatalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

    SetProtect (TRUE);
    if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", outcatalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", outcatalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_save (&outcatalog, VERBOSE);
    dvo_catalog_unlock (&outcatalog);
    dvo_catalog_free (&outcatalog);
  }

  // save the output sky table copy
  char *skyfile = SkyTableFilename (output);
  check_file_access (skyfile, TRUE, TRUE, VERBOSE);
  if (!SkyTableSave (outsky, skyfile)) {
    fprintf (stderr, "ERROR: failed to save sky table for %s\n", output);
    exit (1);
  }

  exit (0);
}

int dvoConvert_copy_images (char *input, char *output) {

  FITS_DB inDB;
  FITS_DB outDB;
  int    status;

  Image *images;
  off_t Nimages;
  off_t ID;

  /*** load output/Images.dat ***/
  sprintf (ImageCat, "%s/Images.dat", output);
  gfits_db_init (&outDB);
  outDB.mode   = dvo_catalog_catmode (CATMODE);
  outDB.format = dvo_catalog_catformat (CATFORMAT);
  status       = dvo_image_lock (&outDB, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", outDB.filename);

  // output image table should not already exist
  if (outDB.dbstate != LCK_EMPTY) {
    Shutdown ("ERROR: image table %s already exists", outDB.filename);
  }
  dvo_image_create (&outDB, GetZeroPoint());

  /*** load input/Images.dat ***/
  sprintf (ImageCat, "%s/Images.dat", input);
  gfits_db_init (&inDB);
  // inDB.mode   = dvo_catalog_catmode (CATMODE);
  // inDB.format = dvo_catalog_catformat (CATFORMAT);
  status       = dvo_image_lock (&inDB, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", inDB.filename);

  // load the image table 
  if (inDB.dbstate == LCK_EMPTY) {
    Shutdown ("can't find input image catalog %s", inDB.filename);
  }
  if (!dvo_image_load (&inDB, VERBOSE, TRUE)) {
    Shutdown ("can't read input image catalog %s", inDB.filename);
  }

  // convert the raw image table to Image type (byteswap if needed)
  images = gfits_table_get_Image (&inDB.ftable, &Nimages, &inDB.scaledValue, &inDB.nativeOrder);
  if (!images) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  // update additional metadata
  gfits_scan (&inDB.header, "IMAGEID", OFF_T_FMT, 1,  &ID);
  gfits_modify (&outDB.header, "NIMAGES", OFF_T_FMT, 1,  Nimages);
  gfits_modify (&outDB.header, "IMAGEID", OFF_T_FMT, 1,  ID);

  // copy input rows to output table
  gfits_add_rows (&outDB.ftable, (char *) images, Nimages, sizeof(Image));

  // write out the image table to disk
  SetProtect (TRUE);
  dvo_image_save (&outDB, VERBOSE);
  SetProtect (FALSE);
  dvo_image_unlock (&outDB); // unlock output
  dvo_image_unlock (&inDB); // unlock input1

  return TRUE;
}
