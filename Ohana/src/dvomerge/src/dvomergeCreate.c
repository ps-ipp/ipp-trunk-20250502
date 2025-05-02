# include "dvomerge.h"

// dvomergeCreate (catdir.in1) and (catdir.in2) to (catdir.output)
// the output db may have a different SKY_DEPTH from the input dbs
int dvomergeCreate (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);

  int depth1, depth2;
  off_t i, j, Ns, Ne;
  SkyTable *outsky, *insky1, *insky2;
  SkyList *inlist;
  Catalog incatalog, outcatalog;
  char filename[256];
  char filename1[256];
  char *codesFilename;
  char *input1, *input2, *output;
  IDmapType IDmap1, IDmap2;
  int NsecfiltInput1, NsecfiltInput2, NsecfiltOutput;

  if (strcasecmp (argv[2], "and")) dvomerge_usage();
  if (strcasecmp (argv[4], "to")) dvomerge_usage();

  input1 = argv[1];
  input2 = argv[3];
  output = argv[5];

  dvomergeImagesCreate (&IDmap1, input1, &IDmap2, input2, output);

  PhotCodeData *input1Photcodes = NULL;
  PhotCodeData *input2Photcodes = NULL;
  PhotCodeData *outputPhotcodes = NULL;
  int *secfiltMap1 = NULL;
  int *secfiltMap2 = NULL;

  // Read the input1 photcodes
  SetPhotcodeTable(NULL);
  sprintf (filename1, "%s/Photcodes.dat", input1);
  if (!LoadPhotcodes (filename1, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", filename1);
    exit (1);
  }
  input1Photcodes = GetPhotcodeTable();
  NsecfiltInput1 = GetPhotcodeNsecfilt();

  // Read the input2 photcodes
  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", input2);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", filename);
    exit (1);
  }
  input2Photcodes = GetPhotcodeTable();
  NsecfiltInput2 = GetPhotcodeNsecfilt();

  if (ALTERNATE_PHOTCODE_FILE) {
    SetPhotcodeTable(NULL);
    // First attempt to read file as fits table ...
    if (!LoadPhotcodes(ALTERNATE_PHOTCODE_FILE, NULL, FALSE)) {
      // and if that fails try it as a text file
      if (!LoadPhotcodesText(ALTERNATE_PHOTCODE_FILE)) {
        fprintf (stderr, "failed to read photcode file %s\n", ALTERNATE_PHOTCODE_FILE);
        exit (1);
      }
    }
    outputPhotcodes = GetPhotcodeTable();
    NsecfiltOutput = GetPhotcodeNsecfilt();

    secfiltMap1 = GetSecFiltMap(outputPhotcodes, input1Photcodes);
    if (!secfiltMap1) {
      fprintf (stderr, "failed to map %s secfilt photcodes to alternate photcodes table %s\n", input1,
        ALTERNATE_PHOTCODE_FILE);
      exit (1);
    }

    codesFilename = ALTERNATE_PHOTCODE_FILE;

  } else {
    // Use the first input file's photcodes
    outputPhotcodes = input1Photcodes;
    codesFilename = filename1;
    NsecfiltOutput = NsecfiltInput1;
    // secfitMap1 can remain NULL since input1 defines the set of codes
  }

  secfiltMap2 = GetSecFiltMap(outputPhotcodes, input2Photcodes);
  if (!secfiltMap2) {
    fprintf (stderr, "falied to map %s secfilt photcodes to output photcodes table %s\n", input2,
      codesFilename);
    exit (1);
  }

  // trigger any dependencies, if any
  SetPhotcodeTable(NULL);

  // save the output photcodes in the output catdir
  SetPhotcodeTable(outputPhotcodes);
  sprintf (filename, "%s/Photcodes.dat", output);
  if (!check_file_access (filename, TRUE, TRUE, VERBOSE)) {
    fprintf (stderr, "error creating output catdir %s\n", output);
    exit (1);
  }
  if (!SavePhotcodesFITS (filename)) {
    fprintf (stderr, "error saving photcode table %s\n", filename);
    exit (1);
  }

  // load the sky table for the existing database
  insky1 = SkyTableLoadOptimal (input1, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (insky1, input1, "cpt");

  insky2 = SkyTableLoadOptimal (input2, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (insky2, input2, "cpt");

  // generate an output table populated at the desired depth
  outsky = SkyTableLoadOptimal (output, NULL, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (outsky, output, "cpt");

  // loop over the populatable output tables; check for data in input1 and/or input2
  // in the corresponding regions

  SkyTablePopulatedRange (&Ns, &Ne, insky1, 0);
  depth1 = insky1[0].regions[Ns].depth;
  
  SkyTablePopulatedRange (&Ns, &Ne, insky2, 0);
  depth2 = insky2[0].regions[Ns].depth;

  SkyList *outlist = SkyListByPatch (outsky, -1, &UserPatch);

  // loop over the populatable output regions
  for (i = 0; i < outlist[0].Nregions; i++) {
    if (!outlist[0].regions[i][0].table) continue;
    if (VERBOSE) fprintf (stderr, "output: %s\n", outlist[0].regions[i][0].name);

    // load / create output catalog
    LoadCatalog (&outcatalog, outlist[0].regions[i], outlist[0].filename[i], "w", NsecfiltOutput);

    // combine only tables at equal or larger depth
      
    // load in all of the tables from input1 for this region
    inlist = SkyListByBounds (insky1, depth1, outlist[0].regions[i][0].Rmin + 0.01, outlist[0].regions[i][0].Rmax - 0.01, outlist[0].regions[i][0].Dmin + 0.01, outlist[0].regions[i][0].Dmax - 0.01);
    for (j = 0; j < inlist[0].Nregions; j++) {
      if (VERBOSE) fprintf (stderr, "input 1: %s\n", inlist[0].regions[j][0].name);

      // load input catalog (1)
      LoadCatalog (&incatalog, inlist[0].regions[j], inlist[0].filename[j], "r", NsecfiltInput1);

      // skip empty input catalogs
      if (!incatalog.Naverage_disk) {
	dvo_catalog_unlock (&incatalog);
	dvo_catalog_free (&incatalog);
	continue;
      }
      dvo_update_image_IDs (&IDmap1, &incatalog);
      merge_catalogs_new (outlist[0].regions[i], &outcatalog, &incatalog, secfiltMap1);
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
    }
    SkyListFree (inlist);

    // load in all of the tables from input2 for this region
    inlist = SkyListByBounds (insky2, depth2, outlist[0].regions[i][0].Rmin + 0.01, outlist[0].regions[i][0].Rmax - 0.01, outlist[0].regions[i][0].Dmin + 0.01, outlist[0].regions[i][0].Dmax - 0.01);
    for (j = 0; j < inlist[0].Nregions; j++) {
      if (VERBOSE) fprintf (stderr, "input 2: %s\n", inlist[0].regions[j][0].name);

      // load input catalog (2)
      LoadCatalog (&incatalog, inlist[0].regions[j], inlist[0].filename[j], "r", NsecfiltInput2);

      // skip empty input catalogs
      if (!incatalog.Naverage_disk) {
	dvo_catalog_unlock (&incatalog);
	dvo_catalog_free (&incatalog);
	continue;
      }
      dvo_update_image_IDs (&IDmap2, &incatalog);
      merge_catalogs_old (outlist[0].regions[i], &outcatalog, &incatalog, RADIUS, secfiltMap2);
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
    }
    SkyListFree (inlist);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", outcatalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", outcatalog.filename); exit (1); }
    SetProtect (FALSE);

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

  /*** update the image table ***/
int dvomergeImagesCreate (IDmapType *IDmap1, char *input1, IDmapType *IDmap2, char *input2, char *output) { 

    FITS_DB in1DB;
    FITS_DB in2DB;
    FITS_DB outDB;
    int    status;

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


    /*** load input1/Images.dat ***/
    sprintf (ImageCat, "%s/Images.dat", input1);
    gfits_db_init (&in1DB);
    in1DB.mode   = dvo_catalog_catmode (CATMODE);
    in1DB.format = dvo_catalog_catformat (CATFORMAT);
    status       = dvo_image_lock (&in1DB, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
    if (!status) Shutdown ("ERROR: failure to lock image catalog %s", in1DB.filename);

    // load the image table 
    if (in1DB.dbstate != LCK_EMPTY) {
      if (!dvo_image_load (&in1DB, VERBOSE, TRUE)) {
	Shutdown ("can't read input (1) image catalog %s", in1DB.filename);
      }
    }
      
    // convert database table to internal structure & add to output image db
    // if in1DB has no images, we will (later) insist that there are no image IDs to map
    dvo_image_merge_dbs(IDmap1, &outDB, &in1DB);
    dvo_image_unlock (&in1DB); // unlock input1

    /*** load input2/Images.dat ***/
    sprintf (ImageCat, "%s/Images.dat", input2);
    gfits_db_init (&in2DB);
    in2DB.mode   = dvo_catalog_catmode (CATMODE);
    in2DB.format = dvo_catalog_catformat (CATFORMAT);
    status       = dvo_image_lock (&in2DB, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
    if (!status) Shutdown ("ERROR: failure to lock image catalog %s", in2DB.filename);

    /* load the image table */
    if (in2DB.dbstate != LCK_EMPTY) {
      if (!dvo_image_load (&in2DB, VERBOSE, TRUE)) {
	Shutdown ("can't read input (2) image catalog %s", in2DB.filename);
      }
    }

    // convert database table to internal structure & add to output image db
    // if in1DB has no images, we will (later) insist that there are no image IDs to map
    dvo_image_merge_dbs(IDmap2, &outDB, &in2DB);
    dvo_image_unlock (&in2DB); // unlock input2

    // write out the image table
    SetProtect (TRUE);
    dvo_image_save (&outDB, VERBOSE);
    SetProtect (FALSE);
    dvo_image_unlock (&outDB); // unlock output

    return TRUE;
  }

