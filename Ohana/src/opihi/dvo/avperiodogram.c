# include "dvoshell.h"

int avperiodogram (int argc, char **argv) {
  
  int N, next, Nfields;

  dbStack *stack = NULL;
  dbField *fields = NULL;
  dbValue *values = NULL;
  SkyList *skylist = NULL;
  SkyRegionSelection *selection = NULL;

  // **** parse the optional arguments ****
  if ((N = get_argument (argc, argv, "-h"))) goto help;
  if ((N = get_argument (argc, argv, "--help"))) goto help;

  int VERBOSE = FALSE;
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

  /* load photcode information (this needs to come before the SelectRegions command below to define the catdir */
  if (!InitPhotcodes ()) goto escape;
  int Nsecfilt = GetPhotcodeNsecfilt ();

  // parse skyregion options.  NOTE: this is stripped off in parallel operation and always
  // defined for the client via the -skyregion option.  The dvo_client parses this
  // argument in the main program, before it is passed to the command (like mextract)
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) {
    print_error(); 
    goto escape; 
  }

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  // **** launch parallel / remote jobs ****
  // the result files are left behind for the user to access (no automatic re-merge)
  if (PARALLEL && !HOST_ID) {

    // check for -region and apply to current sky region
    if (!SetSkyRegions (selection)) {
      FreeSkyRegionSelection (selection);
      dbFreeFields (fields, Nfields);
      goto help;
    }

    int status = HostTableParallelOps (skylist, argc, argv, NULL, FALSE, 0, VERBOSE);
    SkyListFree (skylist);
    FreeSkyRegionSelection (selection);
    return status;
  }

  // NOTE: optional arguments below are parsed after the possible remote call so they are also passed to the dvo_client

  if ((N = get_argument (argc, argv, "-min-period"))) {
    remove_argument (N, &argc, argv);
    PeriodogramSetOptions (atof (argv[N]), NAN);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-max-period"))) {
    remove_argument (N, &argc, argv);
    PeriodogramSetOptions (NAN, atof (argv[N]));
    remove_argument (N, &argc, argv);
  }

  // the first three Measure fields must be time, mag, dmag (mag:err)
  int Nmfields = 3;
  ALLOCATE_PTR (mfields, dbField, Nmfields);
  dbInitField (&mfields[0]); ParseMeasureField (&mfields[0], "time");
  dbInitField (&mfields[1]); ParseMeasureField (&mfields[1], "mag");
  dbInitField (&mfields[2]); ParseMeasureField (&mfields[2], "mag:err");
  
  int Nmstack = 0;
  dbStack *mstack = NULL;
  dbValue *mvalues = NULL;

  // we can restrict these with a -where-measure clause
  if ((N = get_argument (argc, argv, "-where-measure"))) {
    remove_argument (N, &argc, argv);

    // parse the remainder of the line as a boolean math expression
    unsigned int Nmcstack;
    char **mcstack = isolate_elements (argc-N, &argv[N], &Nmcstack);
  
    // construct the db Boolean math stack (frees cmstack)
    mstack = dbRPN (Nmcstack, mcstack, &Nmstack);
    if (Nmcstack && !Nmstack) {
      print_error(); 
      goto escape; 
    }
    
    // parse stack elements into fields and scalars as needed (supplement mfields)
    if (!dbCheckStack (mstack, Nmstack, DVO_TABLE_MEASURE, &mfields, &Nmfields)) goto escape;

    argc = N; // hide remaining entries from the rest of the code below
  }    
  // output values
  ALLOCATE (mvalues, dbValue, Nmfields);

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

  // command-line is of the form: avperiodogram (output) where (field op value)...
  if (argc < 2) goto help;
  char *output = strcreate (argv[1]);

  // **** generate output file ****
  // we save the results in a single FITS file, one extension per object
  FILE *foutput = NULL;
  if (RESULT_FILE) {
    foutput = fopen (RESULT_FILE, "w");
  } else {
    foutput = fopen (output, "w");
  }
  // generate the PHU and write to disk
  // XXX add some metadata here?
  Header header;
  Matrix matrix;
  gfits_init_header (&header);
  gfits_init_matrix (&matrix);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_fwrite_header (foutput, &header);
  gfits_fwrite_matrix (foutput, &matrix);
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  
  Catalog catalog;
  dvo_catalog_init (&catalog, TRUE);

  // init locally static variables (time refs)
  dbExtractAveragesInit (); 
  dbExtractMeasuresInit(HOST_ID);

  // examine line for 'where' or 'match to'.  'match to' is forbidden
  int state = dbCmdlineConditions (argc, argv, 2, &next);
  if (state == DVO_DB_CMDLINE_ERROR) goto escape;
  if (state == DVO_DB_CMDLINE_IS_MATCH) goto escape; // not allowed for avperiodogram

  // parse the remainder of the line as a boolean math expression
  unsigned int Ncstack;
  char **cstack = isolate_elements (argc-next, &argv[next], &Ncstack);
  
  // construct the db Boolean math stack (frees cstack)
  int Nstack;
  stack = dbRPN (Ncstack, cstack, &Nstack);
  if (Ncstack && !Nstack) {
    print_error(); 
    goto escape; 
  }

  // add the skyregion limits to the where statement (or create)
  dbAstroRegionLimits (&stack, &Nstack, selection, DVO_TABLE_AVERAGE);

  // parse stack elements into fields and scalars as needed
  Nfields = 0;
  ALLOCATE (fields, dbField, 1);
  if (!dbCheckStack (stack, Nstack, DVO_TABLE_AVERAGE, &fields, &Nfields)) goto escape;

  // output values
  ALLOCATE (values, dbValue, Nfields);

  int Nobject = 0;
  char extname[80];

  // grab data from all selected sky regions
  struct sigaction *old_sigaction = SetInterrupt();
  for (off_t i = 0; (i < skylist[0].Nregions) && !interrupt; i++) {

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
    catalog.filename = (HOST_ID) ? hostfile : skylist[0].filename[i];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE;
    catalog.Nsecfilt = 0;

    if (VERBOSE) gprint (GP_ERR, "trying %s ("OFF_T_FMT" of "OFF_T_FMT")\n", catalog.filename,  i,  skylist[0].Nregions);
      
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, VERBOSE2, "r")) {
      gprint (GP_ERR, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    // Scan the catalog for objects which match the WHERE clause
    for (off_t j = 0; (j < catalog.Naverage) && !interrupt; j++) {
      dbExtractAveragesInitAve ();  // reset counters for saved fields (costs very little)

      Average *average = &catalog.average[j];

      off_t mOff = average->measureOffset;
      Measure *measure = &catalog.measure[mOff];

      off_t nSec = j*Nsecfilt;
      SecFilt *secfilt = &catalog.secfilt[nSec];

      // extract the values needed to evaluate the WHERE clause
      for (off_t n = 0; n < Nfields; n++) {
	// for Measure, we are passing in the *first* measure, but average->Nmeasure gives the count
	// for Secfilt, we are passing in the first entry; photcode.equiv gives the entry
	// for Lensobj, we are passing in the first entry or NULL; photcode.equiv gives the entry
	values[n] = dbExtractAverages (average, secfilt, measure, NULL, NULL, NULL, &fields[n]);
      }

      // test the conditional statement
      if (!dbBooleanCond (stack, Nstack, values)) continue;

      // generate time (mjd), mag, dmag vectors
      ALLOCATE_PTR (time, opihi_flt, average->Nmeasure);
      ALLOCATE_PTR (mag,  opihi_flt, average->Nmeasure);
      ALLOCATE_PTR (dmag, opihi_flt, average->Nmeasure);

      int Npts = 0;

      // this object passes the conditions above: now grab its measures which match our conditions?
      for (off_t k = 0; (k < average->Nmeasure); k++) {
	if (measure[k].averef != j) {
	  gprint (GP_ERR, "ERROR: inconsistent measure->average link.  Unsorted database?\n");
	  goto escape;
	}

	// extract the relevant values for this measurement
	dbExtractMeasuresInitMeas (); // reset counters for saved fields  (costs very little

	// XXX I need to have a different boolean expression here to restrict the measurements
	// The first 3 values[] / fields[] will be time, mag, dmag
	for (off_t n = 0; n < Nmfields; n++) {
	  mvalues[n] = dbExtractMeasures (average, secfilt, &measure[k], NULL, NULL, &mfields[n]);
	}
	if (!dbBooleanCond (mstack, Nmstack, mvalues)) continue;

	// do not allow any NAN or Inf values to polute the result
	if (!isfinite(mvalues[0].Flt)) continue;
	if (!isfinite(mvalues[1].Flt)) continue;
	if (!isfinite(mvalues[2].Flt)) continue;

	time[Npts] = mvalues[0].Flt;
	mag[Npts]  = mvalues[1].Flt;
	dmag[Npts] = mvalues[2].Flt;

	Npts++;
      }

      PeriodogramResult *result = PeriodogramRawFloatingMean (time, mag, dmag, Npts);
      free (time);
      free (mag);
      free (dmag);

      if (!result) continue;

      if (VERBOSE2) gprint (GP_ERR, "periodogram for %d, %d\n", (int) average->catID, (int) average->objID);

      // XXX if we have 1e6 objects, we will have output files with sizes of ~80GB
      // XXX warn or exit if Nboject > 1e6?
      snprintf (extname, 80, "OBJ_%06d", Nobject);
      PeriodogramResultSave (result, extname, foutput, average);
      PeriodogramResultFree (result);
      Nobject ++;
    }
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  fclose (foutput);
  fflush (foutput);

  // free measure stack stuff
  FREE (mvalues);
  dbFreeFields(mfields, Nmfields);
  dbFreeStack(mstack, Nmstack);
  FREE (mstack);

  FREE (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  FREE (stack);

  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);

  free (output);
  return (TRUE);

 escape:
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  if (stack) free (stack);
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  dvo_catalog_free (&catalog);
  return (FALSE);


 help:
  gprint (GP_ERR, "USAGE: avperiodogram (output) where (expression) -where-measure (expression)\n");
  gprint (GP_ERR, "  the where (expression) is a boolean to limit the objects analysed based on average properties\n");
  gprint (GP_ERR, "  NOTE: the optional -where-measure (expression) MUST come after the average where expression\n");
  return (FALSE);
}
