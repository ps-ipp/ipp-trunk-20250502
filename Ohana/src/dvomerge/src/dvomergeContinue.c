NOTE:
/// this is now not used; it has been merged with dvomergeUpdate

# include "dvomerge.h"

int dvomergeContinue (int argc, char **argv) {

  int depth;
  off_t i, j, Ns, Ne;
  SkyTable *outsky, *insky;
  SkyList *outlist, *inlist;
  Catalog incatalog, outcatalog;
  char filename[256], *input, *output;
  IDmapType IDmap;
  PhotCodeData *inputPhotcodes;
  PhotCodeData *outputPhotcodes;
  int *secfiltMap = NULL;
  int NsecfiltInput, NsecfiltOutput;

  double dtime;
  struct timeval start, stop;
  gettimeofday (&start, NULL);

  if (strcasecmp (argv[2], "into")) dvomerge_usage();
  if (strcasecmp (argv[4], "continue")) dvomerge_usage();

  input  = argv[1];
  output = argv[3];

  if (ALTERNATE_PHOTCODE_FILE) {
    fprintf (stderr, "cannot specify photcodes when merging into an existing catdir\n");
    exit (1);
  }

  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", input);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading input database directory %s\n", input);
    exit (1);
  }	
  inputPhotcodes = GetPhotcodeTable();
  NsecfiltInput = GetPhotcodeNsecfilt();

  // since we are merging the input db into the output db, the output defines the photcode
  // table & db layout but, this requires the output to exist.  if it does not, instead use the
  // input.

  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", output);
  if (LoadPhotcodes (filename, NULL, FALSE)) {

    outputPhotcodes = GetPhotcodeTable();
    NsecfiltOutput = GetPhotcodeNsecfilt();

    secfiltMap = GetSecFiltMap(outputPhotcodes, inputPhotcodes);
    if (!secfiltMap) {
      fprintf (stderr, "failed to map input secfilt photcodes to output photcodes table\n");
      exit (1);
    }
  } else {
    fprintf(stderr, "%s not found using photcodes from %s/Photcodes.dat\n", filename, input);
    outputPhotcodes = inputPhotcodes;
    NsecfiltOutput = NsecfiltInput;

    if (!check_dir_access (output, VERBOSE)) {
      fprintf (stderr, "error creating output database directory %s\n", output);
      exit (1);
    }
    SetPhotcodeTable(inputPhotcodes);
    if (!SavePhotcodesFITS (filename)) {
      fprintf (stderr, "error saving photcode table in %s/Photcodes.dat\n", output);
      exit (1);
    }
  }

  // need to determine the mapping from the input to the output images
  dvomergeImagesGetMap (&IDmap, input, output);

  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (input, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  if (!insky) {
      Shutdown ("can't read SkyTable for %s", input);
  }
  SkyTableSetFilenames (insky, input, "cpt");

  // XXX apply this...generate the subset matching the user-selected region
  inlist = SkyListByPatch (insky, -1, &UserPatch);

  // generate an output table populated at the desired depth
  outsky = SkyTableLoadOptimal (output, NULL, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  if (!outsky) {
      Shutdown ("can't read or create SkyTable for %s", output);
  }
  SkyTableSetFilenames (outsky, output, "cpt");

  // loop over the populatable output tables; check for data in input in the corresponding regions

  SkyListPopulatedRange (&Ns, &Ne, inlist, 0);
  depth = inlist[0].regions[Ns][0].depth;
  
  SetPhotcodeTable(NULL);

  // loop over the populated input regions
  for (i = 0; i < inlist[0].Nregions; i++) {
    if (!inlist[0].regions[i][0].table) continue;
    if (VERBOSE) fprintf (stderr, "input: %s\n", inlist[0].regions[i][0].name);

    // load / create output catalog (if catalog does not exist, it will be created)
    LoadCatalog (&incatalog, &inlist[0].regions[i][0], inlist[0].filename[i], "r", NsecfiltInput);
    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
	dvo_catalog_unlock (&incatalog);
	dvo_catalog_free (&incatalog);
	continue;
    }

    // combine only tables at equal or larger depth
      
    // load in all of the tables from input for this region
    // SkyListByBounds will return neighbor catalogs if the boundaries exactly match (due to rounding).  Since the regions are not infinitely small, 
    // compare to a slightly reduced footprint
    float dPos = 2.0/3600.0;
    outlist = SkyListByBounds (outsky, depth, inlist[0].regions[i][0].Rmin + dPos, inlist[0].regions[i][0].Rmax - dPos, inlist[0].regions[i][0].Dmin + dPos, inlist[0].regions[i][0].Dmax - dPos);
    for (j = 0; j < outlist[0].Nregions; j++) {
      if (VERBOSE) fprintf (stderr, "output : %s\n", outlist[0].regions[j][0].name);

      // load input catalog
      LoadCatalog (&outcatalog, outlist[0].regions[j], outlist[0].filename[j], "w", NsecfiltOutput);

      dvo_update_image_IDs (&IDmap, &incatalog);
      merge_catalogs_old (&outsky[0].regions[j], &outcatalog, &incatalog, RADIUS, secfiltMap);

      // if we receive a signal which would cause us to exit, wait until the full catalog is written
      SetProtect (TRUE);
      if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", outcatalog.filename); exit (1); }
      if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", outcatalog.filename); exit (1); }
      SetProtect (FALSE);

      dvo_catalog_free (&outcatalog);

      fprintf (stderr, "merged %s into %s\n", inlist[0].regions[i][0].name, outlist[0].regions[j][0].name);
    }
    SkyListFree (outlist);

    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);
  }

  // save the output sky table copy
  char *skyfile = SkyTableFilename (output);
  check_file_access (skyfile, TRUE, TRUE, VERBOSE);
  if (!SkyTableSave (outsky, skyfile)) {
    fprintf (stderr, "ERROR: failed to save sky table for %s\n", output);
    exit (1);
  }

  gettimeofday (&stop, NULL);
  dtime = DTIME (stop, start);
  fprintf (stderr, "SUCCESS: elapsed time %9.4f sec\n", dtime);

  exit (0);
}
