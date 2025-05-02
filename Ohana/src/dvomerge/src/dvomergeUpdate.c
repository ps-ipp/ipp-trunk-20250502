# include "dvomerge.h"

int dvomergeUpdate (int argc, char **argv) {

  int CONTINUE;
  SkyTable *outsky, *insky;
  SkyList *inlist;
  char filename[256], *input, *output;
  IDmapType IDmap;
  int *secfiltMap = NULL;
  int NsecfiltInput, NsecfiltOutput;

  INITTIME;

  dvo_image_map_init (&IDmap);

  CONTINUE = FALSE;
  if (strcasecmp (argv[2], "into")) dvomerge_usage();
  if (argc == 5) {
    if (strcasecmp (argv[4], "continue")) dvomerge_usage();
    CONTINUE = TRUE;
    if (IMAGES_ONLY) {
      fprintf (stderr, "-images-only is not compatible with the 'continue' option\n");
      exit (5);
    }
  }
  if (VERIFY) CONTINUE = TRUE;

  input  = argv[1];
  output = argv[3];

  if (ALTERNATE_PHOTCODE_FILE) {
    fprintf (stderr, "cannot specify photcodes when merging into an existing catdir\n");
    exit (1);
  }

  PhotCodeData *inputPhotcodes  = NULL;
  PhotCodeData *outputPhotcodes = NULL;

  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", input);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading input database directory %s\n", input);
    exit (1);
  }	
  inputPhotcodes = GetPhotcodeTable();
  NsecfiltInput = GetPhotcodeNsecfilt();

  if (REPLACE_TYCHO) replace_tycho_init();

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

  // do not merge the images unless requested
  if (!SKIP_IMAGES) {
    if (CONTINUE) {
      // need to determine the mapping from the input to the output images
      dvomergeImagesGetMap (&IDmap, input, output);
    } else {
      dvomergeImagesUpdate (&IDmap, input, output);
      if (IMAGES_ONLY) exit (0);
    }
  }
    
  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (input, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  if (!insky) {
      Shutdown ("can't read SkyTable for %s", input);
  }
  SkyTableSetFilenames (insky, input, "cpt");

  // generate the subset matching the user-selected region
  inlist = SkyListByPatch (insky, -1, &UserPatch);

  // modify the list if we are restricting:
  if (CPTLIST) {
    inlist = SkyListMatchList (inlist, CPTLIST, NCPTLIST);
  }

  // generate an output table populated at the desired depth
  outsky = SkyTableLoadOptimal (output, NULL, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  if (!outsky) {
      Shutdown ("can't read or create SkyTable for %s", output);
  }
  SkyTableSetFilenames (outsky, output, "cpt");

  // loop over the populatable output tables; check for data in input in the corresponding regions

  // XXX this is not really used...
  // SkyListPopulatedRange (&Ns, &Ne, inlist, 0);
  // depth = inlist[0].regions[Ns][0].depth;
  
  SetPhotcodeTable(NULL);

  int status = dvomergeUpdate_catalogs (input, output, inlist, outsky, NsecfiltInput, NsecfiltOutput, &IDmap, secfiltMap);

  // save the output sky table copy
  char *skyfile = SkyTableFilename (output);
  check_file_access (skyfile, TRUE, TRUE, VERBOSE);
  if (!SkyTableSave (outsky, skyfile)) {
    fprintf (stderr, "ERROR: failed to save sky table for %s\n", output);
    exit (1);
  }
  FREE (skyfile);

  SkyTableFree (insky);
  SkyTableFree (outsky);
  SkyListFree (inlist);

  FREE (secfiltMap);
  dvo_image_map_free (&IDmap);
  if (outputPhotcodes != inputPhotcodes) { FreePhotcodeData (outputPhotcodes); }
  FreePhotcodeData (inputPhotcodes);
  FreePhotcodeTable ();

  if (!status) {
    MARKTIME ("ERROR: elapsed time %9.4f sec\n", dtime);
    exit (3);
  }

  MARKTIME ("SUCCESS: elapsed time %9.4f sec\n", dtime);
  return TRUE;
}
