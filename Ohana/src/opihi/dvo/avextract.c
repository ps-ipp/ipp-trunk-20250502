# include "dvoshell.h"

int avextract (int argc, char **argv) {
  
  off_t i, j, n, m;
  int N, Npts, NPTS, last, next, state, Nfields, Nreturn, Nstack;
  int Nsecfilt, VERBOSE;
  char **cstack, name[1024];
  unsigned int Ncstack;

  Catalog catalog;

  Vector **vec;
  dbStack *stack;
  dbField *fields;
  dbValue *values;
  SkyList *skylist;
  SkyRegionSelection *selection;

  /* defaults */
  vec = NULL;
  stack = NULL;
  fields = NULL;
  values = NULL;
  skylist = NULL;
  selection = NULL;

  if ((N = get_argument (argc, argv, "-h"))) goto help;
  if ((N = get_argument (argc, argv, "--help"))) goto help;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  int VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-vv"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
    VERBOSE2 = TRUE;
  }

  int PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    remove_argument (N, &argc, argv);
    PARALLEL = TRUE;
  }

  // use remote tables, but not dvo_client..
  int PARALLEL_LOCAL = FALSE;
  HostTable *table = NULL;
  if ((N = get_argument (argc, argv, "-parallel-local"))) {
    remove_argument (N, &argc, argv);
    PARALLEL_LOCAL = TRUE;

    char *CATDIR = GetCATDIR();
    if (!CATDIR) {
      gprint (GP_ERR, "CATDIR is not set\n");
      return FALSE;
    }
    SkyTable *sky = GetSkyTable();
    if (!sky) {
      gprint (GP_ERR, "failed to load sky table for database\n");
      return FALSE;
    }
    table = HostTableLoad (CATDIR, sky->hosts);
    if (!table) {
      gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      return FALSE;
    }    
  }

  // this is used to NOT save the results in the results file
  // use this option when mextract is used in a script which does its
  // own job of packaging the results
  int SKIP_RESULTS = FALSE;
  if ((N = get_argument (argc, argv, "-skip-results"))) {
    remove_argument (N, &argc, argv);
    SKIP_RESULTS = TRUE;
  }

  dvo_catalog_init (&catalog, TRUE);

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();

  // parse skyregion options.  NOTE: this is stripped off in parallel operation and always
  // defined for the client via the -skyregion option.  The dvo_client parses this
  // argument in the main program, before it is passed to the command (like mextract)
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    print_error(); 
    goto escape; 
  }

  // init locally static variables (time refs)
  dbExtractAveragesInit (); 

  // command-line is of the form: avextract field,field, field [where (field op value)...]

  // parse the fields to be extracted and returned
  fields = dbCmdlineFields (argc, argv, DVO_TABLE_AVERAGE, &last, &Nfields);
  if (fields == NULL) goto escape;
  if (Nfields == 0) {
    FreeSkyRegionSelection (selection);
    dbFreeFields (fields, Nfields);
    dvo_catalog_free (&catalog);
    goto help;
  }

  // examine line for 'where' or 'match to'.  'match to' is forbidden
  state = dbCmdlineConditions (argc, argv, last, &next);
  if (state == DVO_DB_CMDLINE_ERROR) goto escape;
  if (state == DVO_DB_CMDLINE_IS_MATCH) goto escape; // not allowed for mextract

  // parse the remainder of the line as a boolean math expression
  cstack = isolate_elements (argc-next, &argv[next], &Ncstack);
  
  // construct the db Boolean math stack (frees cstack)
  stack = dbRPN (Ncstack, cstack, &Nstack);
  if (Ncstack && !Nstack) {
    print_error(); 
    goto escape; 
  }

  // add the skyregion limits to the where statement (or create)
  dbAstroRegionLimits (&stack, &Nstack, selection, DVO_TABLE_AVERAGE);

  // parse stack elements into fields and scalars as needed
  Nreturn = Nfields; 
  if (!dbCheckStack (stack, Nstack, DVO_TABLE_AVERAGE, &fields, &Nfields)) goto escape;

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  // this does all the work of re-packaging the command, calling it on the remote machines, then loading in the results
  if (PARALLEL && !HOST_ID) {

    if (!SetSkyRegions (selection)) {
      FreeSkyRegionSelection (selection);
      dbFreeFields (fields, Nfields);
      dvo_catalog_free (&catalog);
      goto help;
    }

    int status = HostTableParallelOps (skylist, argc, argv, RESULT_FILE, TRUE, 0, VERBOSE);

    dbFreeFields (fields, Nfields);
    dbFreeStack (stack, Nstack);
    free (stack);
    FreeSkyRegionSelection (selection);
    dvo_catalog_free (&catalog);

    return status;
  }

  /* create output storage vectors */
  Npts = 0;
  NPTS = 100;
  ALLOCATE (values, dbValue, Nfields);
  ALLOCATE (vec, Vector *, Nreturn);
  for (i = 0; i < Nreturn; i++) {
    if (ISNUM(fields[i].name[0])) {
      sprintf (name, "v_%s", fields[i].name);
    } else {
      sprintf (name, "%s", fields[i].name);
    }
    if ((vec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) goto escape;
    ResetVector (vec[i], fields[i].type, NPTS);
  }

  // check the requested fields
  int needMeasure = dbFieldNeedMeasure (fields, Nfields);
  int needLensobj = dbFieldNeedLensobj (fields, Nfields);
  int needStarpar = dbFieldNeedStarpar (fields, Nfields, TRUE);
  int needGalphot = dbFieldNeedGalphot (fields, Nfields);

  // grab data from all selected sky regions
  struct sigaction *old_sigaction = SetInterrupt();
  for (i = 0; (i < skylist[0].Nregions) && !interrupt; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    /* lock, load, unlock catalog */
    char hostfile[1024];
    if (PARALLEL_LOCAL) {
      int hostID = (skylist[0].regions[i]->hostFlags & DATA_USE_BCK) ? skylist[0].regions[i]->backupID : skylist[0].regions[i]->hostID;
      int seq = table->index[hostID];
      HOSTDIR = table->hosts[seq].pathname;
    }
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = (HOST_ID || PARALLEL_LOCAL) ? hostfile : skylist[0].filename[i];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
    catalog.catflags |= needMeasure ? DVO_LOAD_MEASURE : DVO_SKIP_MEASURE;
    catalog.catflags |= needLensobj ? DVO_LOAD_LENSOBJ : DVO_SKIP_LENSOBJ;
    catalog.catflags |= needStarpar ? DVO_LOAD_STARPAR : DVO_SKIP_STARPAR;
    catalog.catflags |= needGalphot ? DVO_LOAD_GALPHOT : DVO_SKIP_GALPHOT;
    catalog.Nsecfilt = 0;

    if (VERBOSE) gprint (GP_ERR, "trying %s ("OFF_T_FMT" of "OFF_T_FMT")\n", catalog.filename,  i,  skylist[0].Nregions);
      
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, VERBOSE2, "r")) {
      gprint (GP_ERR, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    for (j = 0; (j < catalog.Naverage) && !interrupt; j++) {
      // extract the relevant values
      // XXX for measure values, this could be optimized for one loop over measures...

      dbExtractAveragesInitAve ();  // reset counters for saved fields (costs very little)

      Average *average = &catalog.average[j];

      m = average->measureOffset;
      Measure *measure = needMeasure && average->Nmeasure ? &catalog.measure[m] : NULL;

      m = average->lensobjOffset;
      Lensobj *lensobj = needLensobj && average->Nlensobj ? &catalog.lensobj[m] : NULL;

      m = average->starparOffset;
      StarPar *starpar = needStarpar && average->Nstarpar ? &catalog.starpar[m] : NULL;

      m = average->galphotOffset;
      GalPhot *galphot = needGalphot && average->Ngalphot ? &catalog.galphot[m] : NULL;

      m = j*Nsecfilt;
      SecFilt *secfilt = &catalog.secfilt[m];

      for (n = 0; n < Nfields; n++) {
	// for Measure, we are passing in the *first* measure, but average->Nmeasure gives the count
	// for Secfilt, we are passing in the first entry; photcode.equiv gives the entry
	// for Lensobj, we are passing in the first entry or NULL; photcode.equiv gives the entry
	values[n] = dbExtractAverages (average, secfilt, measure, lensobj, starpar, galphot, &fields[n]);
      }

      // test the conditional statement
      if (!dbBooleanCond (stack, Nstack, values)) continue;
      for (n = 0; n < Nreturn; n++) {
	if (vec[n][0].type == OPIHI_FLT) {
	  vec[n][0].elements.Flt[Npts] = values[n].Flt;
	} else {
	  vec[n][0].elements.Int[Npts] = values[n].Int;
	}
      }
      Npts++;
      if (Npts >= NPTS) {
	NPTS += 2000;
	for (n = 0; n < Nreturn; n++) {
	  if (vec[n][0].type == OPIHI_FLT) {
	    REALLOCATE (vec[n][0].elements.Flt, opihi_flt, NPTS);
	  } else {
	    REALLOCATE (vec[n][0].elements.Int, opihi_int, NPTS);
	  }
	}
      }
    }
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  for (n = 0; n < Nreturn; n++) {
    ResetVector (vec[n], fields[n].type, Npts);
  }

  // write vectors to a table (this is used by parallel dvo operations, but can be used elsewhere)
  if (RESULT_FILE && !SKIP_RESULTS) {
    int status = WriteVectorTableFITS (RESULT_FILE, "RESULT", NULL, vec, Nreturn, FALSE, FALSE, NULL, 0);
    if (!status) {
      goto escape;
    }
  }

  if (table) free (table);
  if (vec) free (vec);
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  if (stack) free (stack);
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  return (TRUE);

 escape:
  if (table) free (table);
  if (vec) free (vec);
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  if (stack) free (stack);
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  dvo_catalog_free (&catalog);
  return (FALSE);

 help:
  gprint (GP_ERR, "USAGE: avextract field[,field,field...] where (expression)\n");

  if ((argc > N + 1) && !strcasecmp (argv[N+1], "fields")) {
    gprint (GP_ERR, "  RA : right ascension (J2000) [degrees]\n");
    gprint (GP_ERR, "  DEC : declination [degrees]\n");
    gprint (GP_ERR, "  GLON : galactic longitude [degrees]\n");
    gprint (GP_ERR, "  GLAT : galactic latitude [degrees]\n");
    gprint (GP_ERR, "  ELON : ecliptic longitude [degrees]\n");
    gprint (GP_ERR, "  ELAT : ecliptic latitude [degrees]\n");
    gprint (GP_ERR, "  dRA : ra scatter [degrees]\n");
    gprint (GP_ERR, "  dDEC : dec scatter [degrees]\n");
    gprint (GP_ERR, "  uRA : proper motion in ra [arcseconds]\n");
    gprint (GP_ERR, "  uDEC : proper motion in dec [arcseconds]\n");
    gprint (GP_ERR, "  duRA : proper motion error in ra [arcseconds]\n");
    gprint (GP_ERR, "  duDEC : proper motion error in dec [arcseconds]\n");
    gprint (GP_ERR, "  PAR : parallax\n");
    gprint (GP_ERR, "  dPAR : parallax error \n");

    gprint (GP_ERR, "  ChiSqPos : chi square of position fit \n");
    gprint (GP_ERR, "  ChiSqPM  : chi square of proper-motion fit \n");
    gprint (GP_ERR, "  ChiSqPar : chi square of parallax fit \n");

    gprint (GP_ERR, "  Tmean : mean epoch (reference for proper motion)\n");
    gprint (GP_ERR, "  Trange : range of times used for proper motion/parallax fit\n");

    gprint (GP_ERR, "  Nmeas : number of measurements\n");
    gprint (GP_ERR, "  Nmiss : number of non-detections\n");
    gprint (GP_ERR, "  Npos  : number of measurments used for astrometry\n");
    gprint (GP_ERR, "  Nastrom  : number of measurments used for astrometry (= Npos)\n");

    gprint (GP_ERR, "  flags     : object flags\n");
    gprint (GP_ERR, "  objflags  : object flags\n");
    gprint (GP_ERR, "  obj_flags : object flags\n");

    gprint (GP_ERR, "  objID : object ID (32 bit, unique in catalog)\n");
    gprint (GP_ERR, "  catID : catalog ID (32 bit)\n");
    gprint (GP_ERR, "  extID_hi : external ID (upper 32 of 64 bit) -- eg, PSPS ID\n");
    gprint (GP_ERR, "  extID_lo : external ID (lower 32 of 64 bit) -- eg, PSPS ID\n");

    gprint (GP_ERR, "  <photcode>:ave : average magnitude for <photcode>\n");
    gprint (GP_ERR, "  <photcode>:ref : reference magnitude system for <photcode>\n");
    gprint (GP_ERR, "  <photcode>:inst : first instrumental magnitude for <photcode>\n");
    gprint (GP_ERR, "  <photcode>:cat : first catalog magnitude for <photcode>\n");
    gprint (GP_ERR, "  <photcode>:sys : first system magnitude for <photcode>\n");
    gprint (GP_ERR, "  <photcode>:rel : first relative magnitude for <photcode>\n");
    gprint (GP_ERR, "  <photcode>:cal : first calibrated magnitude for <photcode> \n");
    gprint (GP_ERR, "  <photcode>:err : magnitude error for photcode\n");
    gprint (GP_ERR, "  <photcode>:chisq : raw chi-square of magnitude fit\n");
    gprint (GP_ERR, "  <photcode>:ncode : number of measurements matching photcode \n");
    gprint (GP_ERR, "  <photcode>:nphot : number of measurements used for average magnitude in this photcode\n");

    // gprint (GP_ERR, "  <photcode>:ap :  catalog aperture magnitude for photcode\n");
    // gprint (GP_ERR, "  <photcode>:aper :  catalog aperture magnitude for photcode\n");
    // gprint (GP_ERR, "  <photcode>:aveerr : average error (stdev)\n");
    // gprint (GP_ERR, "  <photcode>:aperinst : aperture flux\n");
    // gprint (GP_ERR, "  <photcode>:aper_inst : aperture flux\n");

    gprint (GP_ERR, "  <photcode>:kron : kron flux\n");
    // gprint (GP_ERR, "  <photcode>:kronerr : kron error\n");
    gprint (GP_ERR, "  <photcode>:photflags : photometry flags for measurements\n");
    gprint (GP_ERR, "  <photcode>:flags : photometry flags for measurements\n");
    gprint (GP_ERR, "  <photcode>:stdev : standard deviation of measurements\n");
    gprint (GP_ERR, "  <photcode>:20 : 20 percentile psf mag\n");
    gprint (GP_ERR, "  <photcode>:80 : 80 percentile psf mag\n");
    gprint (GP_ERR, "  <photcode>:ucdist : distance to ubercalibrated exposure (in exposure overlaps)\n");
    gprint (GP_ERR, "  <photcode>:stackDetectID : PSPS ID for stack detection\n");
    gprint (GP_ERR, "  <photcode>:fluxpsf : psf flux\n");
    gprint (GP_ERR, "  <photcode>:fluxpsferr : psf flux error\n");
    gprint (GP_ERR, "  <photcode>:fluxkron : kron flux\n");
    gprint (GP_ERR, "  <photcode>:fluxkronerr : kron flux error\n");

    // gprint (GP_ERR, "  type : dophot type (unused)\n");
    // gprint (GP_ERR, "  typefrac : dophot type fraction (unused)\n");
    return (FALSE);
  }
  gprint (GP_ERR, " avextract --help fields : for a complete listing of allowed fields\n");
  return (FALSE);
}
