# include "dvoshell.h"

int avperiodomatch (int argc, char **argv) {
  
  int N;

  SkyList *skylist = NULL;
  SkyRegionSelection *selection = NULL;

  int Ninvec = 0;

  Vector **invec = NULL;
  Vector *RAvec = NULL;
  Vector *DECvec = NULL;

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

  // dump results directly to fits file (esp for parallel dvo)
  char *CoordsFile = NULL;
  if ((N = get_argument (argc, argv, "-coords"))) {
    remove_argument (N, &argc, argv);
    CoordsFile = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // XXX arg-parsing is a bit confusing with the -where-measure option
  if (!CoordsFile && (argc < 5)) goto help;
  if ( CoordsFile && (argc < 3)) goto help;

  RAvec  = NULL;
  DECvec = NULL;
  // get vectors corresponding to coordinates of interest
  if (CoordsFile) {
    // read RAvec, DECvec from coords file (1st 2 fields?)
    Ninvec = 0;
    invec = ReadVectorTableFITS (CoordsFile, "COORDS", &Ninvec);
    RAvec = invec[0];
    DECvec = invec[1];
  } else {
    if ((RAvec  = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) goto help;
    if ((DECvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) goto help;
    remove_argument (2, &argc, argv); // strip off RA
    remove_argument (2, &argc, argv); // strip off DEC
  }

  // **** launch parallel / remote jobs ****
  // the result files are left behind for the user to access (no automatic re-merge)
  if (PARALLEL && !HOST_ID) {
    // allocate the temp array and copy all but (RA) (DEC)
    int targc = 0;
    char **targv = NULL;
    ALLOCATE (targv, char *, argc + 2);
    for (int i = 0; i < argc; i++) {
      targv[targc] = strcreate (argv[i]);
      targc ++;
    }

    // if not specified, create the coords.fits input file
    // NOTE: RAvec, DECvec were set above
    if (!CoordsFile) {
      ALLOCATE_PTR (vec, Vector *, 2);
      vec[0] = RAvec;
      vec[1] = DECvec;

      CoordsFile = abspath("coords.fits", 1024);
      int status = WriteVectorTableFITS (CoordsFile, "COORDS", NULL, vec, 2, FALSE, FALSE, NULL, 0);
      if (!status) goto escape;
      free (vec);
    }

    // add the coords file to the args list
    targv[targc+0] = strcreate ("-coords");
    targv[targc+1] = CoordsFile; // this gets freed with targv
    targc += 2;

    int status = HostTableParallelOps (skylist, targc, targv, NULL, FALSE, RAvec->Nelements, VERBOSE);

    // free up targv
    for (int i = 0; i < targc; i++) {
      free (targv[i]);
    }
    free (targv);

    SkyListFree (skylist);
    FreeSkyRegionSelection (selection);
    return status;
  }

  // NOTE: optional arguments below are parsed after the possible remote call so they are also passed to the dvo_client above

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
  if (argc != 3) goto help;

  char *output = strcreate (argv[1]);
  float RADIUS = atof (argv[2]);

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
  dbExtractMeasuresInit(HOST_ID);

  int Nobject = 0;
  char extname[80];

  int NPTS = RAvec->Nelements;
  ALLOCATE_PTR (idxValue, off_t, NPTS);

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

    find_matches_by_vectors_closest (skylist[0].regions[i], &catalog, RAvec, DECvec, RADIUS, idxValue);

    // Scan the catalog for objects which match the WHERE clause
    for (off_t j = 0; (j < NPTS) && !interrupt; j++) {
      off_t Ncat = idxValue[j];

      if (Ncat == -1) continue; // this point is not in this catalog file
      if (Ncat == -2) continue; // no matches to this point

      dbExtractAveragesInitAve ();  // reset counters for saved fields (costs very little)

      Average *average = &catalog.average[Ncat];

      Measure *measure = &catalog.measure[average->measureOffset];

      off_t m = Ncat*Nsecfilt;
      SecFilt *secfilt = &catalog.secfilt[m];

      // generate time (mjd), mag, dmag vectors
      ALLOCATE_PTR (time, opihi_flt, average->Nmeasure);
      ALLOCATE_PTR (mag,  opihi_flt, average->Nmeasure);
      ALLOCATE_PTR (dmag, opihi_flt, average->Nmeasure);

      int Npts = 0;

      // this object passes the conditions above: now grab its measures which match our conditions?
      for (off_t k = 0; (k < average->Nmeasure); k++) {
	if (measure[k].averef != Ncat) {
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

  FREE (idxValue);

  // free measure stack stuff
  FREE (mvalues);
  dbFreeFields(mfields, Nmfields);
  dbFreeStack(mstack, Nmstack);
  FREE (mstack);

  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);

  free (output);
  return (TRUE);

 escape:
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  dvo_catalog_free (&catalog);

  if (invec) FreeVectorArray (invec, Ninvec);
  return (FALSE);


 help:
  gprint (GP_ERR, "USAGE: avperiodomatch (output) (RA) (DEC) (RADIUS) -where-measure (expression)\n");
  gprint (GP_ERR, "  NOTE: the optional -where-measure (expression) MUST come after the required arguments\n");
  gprint (GP_ERR, "   RADIUS is in arcseconds\n");
  return (FALSE);
}
