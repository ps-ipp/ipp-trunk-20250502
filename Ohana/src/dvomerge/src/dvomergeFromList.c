# include "dvomerge.h"

// merge from list implies 'continue' (ie, we must have already done a partial merge)
int dvomergeFromList (int argc, char **argv) {

  off_t i, j;
  Catalog incatalog, outcatalog;
  char filename[256], *input, *output;
  IDmapType IDmap;
  int *secfiltMap = NULL;
  char inputfile[256], outputfile[256];
  int NsecfiltInput, NsecfiltOutput;

  INITTIME;

  dvo_image_map_init (&IDmap);

  if (strcasecmp (argv[2], "into")) dvomerge_usage();
  if (strcasecmp (argv[4], "from")) dvomerge_usage();

  input  = argv[1];
  output = argv[3];
  listname = argv[5];

  int Ncptlist;
  char *cptlist = load_cptlist (listname, &Ncptlist);

  if (ALTERNATE_PHOTCODE_FILE) {
    fprintf (stderr, "cannot specify photcodes when merging into an existing catdir\n");
    exit (1);
  }

  PhotCodeData *inputPhotcodes = NULL;
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

  // need to determine the mapping from the input to the output images
  // output dvo images must already have been merged
  dvomergeImagesGetMap (&IDmap, input, output);

  // XXX we are not loading the skytable info

  SetPhotcodeTable(NULL);

  // XXX the stuff below is equiv to stuff in dvomergeUpdate_catalogs()

  // loop over the populated input regions
  for (i = 0; i < Ncptlist; i++) {
    if (VERBOSE) fprintf (stderr, "input: %s\n", cptlist[i]);

    sprintf (inputfile,  "%s/%s", input,  cptlist[i]);
    sprintf (outputfile, "%s/%s", output, cptlist[i]);

    OutputStatus *outstat = OutputStatusInit (1);
    outstat[0].history = dmhObjectRead (outputfile);

    dmhObjectStats *inStats = dmhObjectStatsRead (inputfile);

    // XXX : we are not checking for already-merged entries

    LoadCatalog (&incatalog, NULL, inputfile, "r", NsecfiltInput);

    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
	dvo_catalog_unlock (&incatalog);
	dvo_catalog_free (&incatalog);
	continue;
    }

    // combine only tables at equal depth
      
    SkyRegion skyregion;
    gfits_scan (&incatalog.header, "RA0",  "%f", 1, &skyregion.Rmin);
    gfits_scan (&incatalog.header, "RA1",  "%f", 1, &skyregion.Rmax);
    gfits_scan (&incatalog.header, "DEC0", "%f", 1, &skyregion.Dmin);
    gfits_scan (&incatalog.header, "DEC1", "%f", 1, &skyregion.Dmax);

    // load input catalog
    // SetPhotcodeTable(outputPhotcodes);
    LoadCatalog (&outcatalog, NULL, outputfile, "w", NsecfiltOutput);

    if (outcatalog.Naverage == 0) {
      gfits_modify (&outcatalog.header, "RA0",  "%f", 1, skyregion.Rmin);
      gfits_modify (&outcatalog.header, "DEC0", "%f", 1, skyregion.Dmin);
      gfits_modify (&outcatalog.header, "RA1",  "%f", 1, skyregion.Rmax);
      gfits_modify (&outcatalog.header, "DEC1", "%f", 1, skyregion.Dmax);
      gfits_modify (&outcatalog.header, "CATID", "%d", 1, incatalog.catID);
      outcatalog.catID = incatalog.catID;
    }

    dvo_update_image_IDs (&IDmap, &incatalog);
    merge_catalogs_old (&skyregion, &outcatalog, &incatalog, RADIUS, secfiltMap);
    
    dmhObjectAdd (outstat[0].history, &outcatalog.header, inStats);

    // if we receive a signal which would cause us to exit, wait until the full catalog is written
    SetProtect (TRUE);
    if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save catalog %s\n", outcatalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock catalog %s\n", outcatalog.filename); exit (1); }
    SetProtect (FALSE);
    
    fprintf (stderr, "merged %s into %s\n", inputfile, outputfile);

    dmhObjectStatsFree (inStats);
    OutputStatusFree (outstat, 1);

    dvo_catalog_free (&outcatalog);
    
    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);
  }

  // XXX we do not update the skytable 

  // XXX need to free things

  exit (0);
}

