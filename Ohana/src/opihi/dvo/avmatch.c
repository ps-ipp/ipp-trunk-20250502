# include "dvoshell.h"

/* This function uses the 'find_match' algorithm to select the objects of interest.
   Each entry in the match vectors (RA, DEC, RADIUS) yields a result value -- if no
   source matches, the resulting fields are all NAN.

   * choose the sky regions based on the provided RA,DEC points
   * loop over the catalogs
   * within a catalog, use the find_match code to find the matching coordinates
   * use dbExtractAverages to get the fields for the matched entry 
 */

int avmatch (int argc, char **argv) {
  
  off_t i, j, n, m, *index;
  int N, Ncat, Npts, NPTS, last, Nfields, Nsecfilt, Ninvec;
  int VERBOSE;
  char name[1024], *found;
  float RADIUS;

  Catalog catalog;

  Vector **vec, **invec, *RAvec, *DECvec;
  dbField *fields;
  dbValue *values;
  SkyList *skylist;

  /* defaults */
  vec = NULL;
  invec = NULL;
  fields = NULL;
  values = NULL;
  skylist = NULL;
  Ninvec = 0;

  if ((N = get_argument (argc, argv, "-h"))) goto help;
  if ((N = get_argument (argc, argv, "--help"))) goto help;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  int PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    remove_argument (N, &argc, argv);
    PARALLEL = TRUE;
  }

  // dump results directly to fits file (esp for parallel dvo)
  char *CoordsFile = NULL;
  if ((N = get_argument (argc, argv, "-coords"))) {
    remove_argument (N, &argc, argv);
    CoordsFile = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (!CoordsFile && (argc < 5)) goto help;
  if ( CoordsFile && (argc < 3)) goto help;

  dvo_catalog_init (&catalog, TRUE);

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();

  // init locally static variables (time refs)
  dbExtractAveragesInit (); 

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
    if ((RAvec  = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) goto help;
    if ((DECvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) goto help;
    // strip off RA & DEC
    remove_argument (1, &argc, argv);
    remove_argument (1, &argc, argv);
  }

  /* load regions which contain all supplied RA,DEC coordinates */
  if ((skylist = SelectRegionsByCoordVectors (RAvec, DECvec)) == NULL) goto escape;

  // this does all the work of re-packaging the command, calling it on the remote machines, then loading in the results
  if (PARALLEL && !HOST_ID) {

    // We need to copy the args to a temp array and modify them so that we send the
    // correct set to the remote client.  The args list looks like this:
    // if (!CoordsFile) : avmatch (RADIUS) field, ... [we removed RA & DEC above]
    // if ( CoordsFile) : avmatch (RADIUS) field, ... [because we stripped off the -coords filename elements]

    // allocate the temp array and copy all but (RA) (DEC)
    int targc = 0;
    char **targv = NULL;
    ALLOCATE (targv, char *, argc + 2);
    for (i = 0; i < argc; i++) {
      targv[targc] = strcreate (argv[i]);
      targc ++;
    }

    // if not specified, create the coords.fits input file
    // NOTE: RAvec, DECvec were set above
    if (!CoordsFile) {
      ALLOCATE (vec, Vector *, 2);
      vec[0] = RAvec;
      vec[1] = DECvec;

      CoordsFile = abspath("coords.fits", 1024);
      int status = WriteVectorTableFITS (CoordsFile, "COORDS", NULL, vec, 2, FALSE, FALSE, NULL, 0);
      if (!status) goto escape;
    }

    // add the coords file to the args list
    targv[targc+0] = strcreate ("-coords");
    targv[targc+1] = CoordsFile; // this gets freed with targv
    targc += 2;
    
    // I need to pass the RA & DEC vectors to the remote clients...
    int status = HostTableParallelOps (skylist, targc, targv, RESULT_FILE, TRUE, RAvec->Nelements, VERBOSE);
    if (vec) free (vec);
    
    // free up targv
    for (i = 0; i < targc; i++) {
      free (targv[i]);
    }
    free (targv);

    return status;
  }

  RADIUS = atof (argv[1]);
  remove_argument (1, &argc, argv);

  // parse the fields to be extracted and returned
  fields = dbCmdlineFields (argc, argv, DVO_TABLE_AVERAGE, &last, &Nfields);
  if (fields == NULL) goto help;
  if ((Nfields == 0) || (last != argc)) {
    dbFreeFields (fields, Nfields);
    dvo_catalog_free (&catalog);
    goto help;
  }

  // check the requested fields
  int needMeasure = dbFieldNeedMeasure (fields, Nfields);
  int needLensobj = dbFieldNeedLensobj (fields, Nfields);
  int needStarpar = dbFieldNeedStarpar (fields, Nfields, TRUE);

  /* create output storage vectors */
  NPTS = RAvec->Nelements;
  ALLOCATE (values, dbValue, Nfields);
  ALLOCATE (vec, Vector *, Nfields);
  for (i = 0; i < Nfields; i++) {
    if (ISNUM(fields[i].name[0])) {
      sprintf (name, "v_%s", fields[i].name);
    } else {
      sprintf (name, "%s", fields[i].name);
    }
    if ((vec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) goto escape;
    ResetVector (vec[i], fields[i].type, NPTS);
    for (n = 0; n < vec[i][0].Nelements; n++) {
      if (vec[i][0].type == OPIHI_FLT) {
	vec[i][0].elements.Flt[n] = NAN;
      } else {
	vec[i][0].elements.Int[n] = 0; // or NAN_INT?
      }
    }
  }
  ALLOCATE (index, off_t, NPTS);
  ALLOCATE (found, char, NPTS);
  memset (found, 0, NPTS*sizeof(char));

  // grab data from all selected sky regions
  struct sigaction *old_sigaction = SetInterrupt();
  for (i = 0; (i < skylist[0].Nregions) && !interrupt; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    /* lock, load, unlock catalog */
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = HOST_ID ? hostfile : skylist[0].filename[i];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
    catalog.catflags |= needMeasure ? DVO_LOAD_MEASURE : DVO_SKIP_MEASURE;
    catalog.catflags |= needLensobj ? DVO_LOAD_LENSOBJ : DVO_SKIP_LENSOBJ;
    catalog.catflags |= needStarpar ? DVO_LOAD_STARPAR : DVO_SKIP_STARPAR;
    catalog.Nsecfilt = 0;

    if (VERBOSE) gprint (GP_ERR, "trying %s ("OFF_T_FMT" of "OFF_T_FMT")\n", catalog.filename,  i,  skylist[0].Nregions);
      
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      gprint (GP_ERR, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);  /// we can unlock here since this is read-only (do not block other access)
    if (catalog.Naverage == 0) {
      dvo_catalog_free (&catalog);
      continue;
    }

    find_matches_by_vectors_closest (skylist[0].regions[i], &catalog, RAvec, DECvec, RADIUS, index);

    for (j = 0; (j < NPTS) && !interrupt; j++) {
      Ncat = index[j];
      Npts = j;

      if (Ncat == -1) continue; // this point is not in this catalog file
      if (Ncat == -2) continue; // no matches to this point

      m = Ncat;
      Average *average = &catalog.average[m];

      m = average->measureOffset;
      Measure *measure = needMeasure ? &catalog.measure[m] : NULL;

      m = average->lensobjOffset;
      Lensobj *lensobj = needLensobj ? &catalog.lensobj[m] : NULL;

      m = average->starparOffset;
      StarPar *starpar = needStarpar ? &catalog.starpar[m] : NULL;

      m = Ncat*Nsecfilt;
      SecFilt *secfilt = &catalog.secfilt[m];

      // reset counters for saved fields, extract fields
      dbExtractAveragesInitAve (); 
      for (n = 0; n < Nfields; n++) {
	values[n] = dbExtractAverages (average, secfilt, measure, lensobj, starpar, NULL, &fields[n]);
      }

      // XXX if we are allowed to return more rows than the supplied RA,DEC we will need to create an output RA,DEC
      for (n = 0; n < Nfields; n++) {
	if (vec[n][0].type == OPIHI_FLT) {
	  vec[n][0].elements.Flt[Npts] = values[n].Flt;
	} else {
	  vec[n][0].elements.Int[Npts] = values[n].Int;
	}
      }
      found[Npts] = TRUE;
    }
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  // write vectors to a table (this is used by parallel dvo operations, but can be used elsewhere)
  // only write the fields which were in a valid catalog
  if (RESULT_FILE) {
    // extend the array by one to hold index array
    REALLOCATE (vec, Vector *, Nfields + 1);
    vec[Nfields] = InitVector();
    strcpy (vec[Nfields]->name, "index");
    ResetVector (vec[Nfields], OPIHI_INT, NPTS);
    Vector *idxVec = vec[Nfields];

    // only write out the rows which were found
    Npts = 0;
    for (i = 0; i < NPTS; i++) {
      if (!found[i]) continue;
      idxVec->elements.Int[Npts] = i;
      Npts ++;
    }
    int Nfound = Npts;
    idxVec->Nelements = Nfound;

    fprintf (stderr, "found %d of %d pts\n", Nfound, NPTS);

    for (i = 0; i < Nfields; i++) {
      if (vec[i][0].type == OPIHI_FLT) {
	opihi_flt *tmp = NULL;
	ALLOCATE (tmp, opihi_flt, Nfound);
	Npts = 0;
	for (j = 0; j < NPTS; j++) {
	  if (!found[j]) continue;
	  tmp[Npts] = vec[i][0].elements.Flt[j];
	  Npts++;
	}
	free (vec[i][0].elements.Flt);
	vec[i][0].elements.Flt = tmp;
      } else {
	opihi_int *tmp = NULL;
	ALLOCATE (tmp, opihi_int, Nfound);
	Npts = 0;
	for (j = 0; j < NPTS; j++) {
	  if (!found[j]) continue;
	  tmp[Npts] = vec[i][0].elements.Int[j];
	  Npts++;
	}
	free (vec[i][0].elements.Int);
	vec[i][0].elements.Int = tmp;
      }
      vec[i][0].Nelements = Nfound;
    }
    int status = WriteVectorTableFITS (RESULT_FILE, "RESULT", NULL, vec, Nfields + 1, FALSE, FALSE, NULL, 0);
    free (vec[Nfields]->elements.Int);
    free (vec[Nfields]);
    if (!status) goto escape;
  }

  if (vec) free (vec);
  if (values) free (values);
  if (invec) FreeVectorArray (invec, Ninvec);
  dbFreeFields (fields, Nfields);
  SkyListFree (skylist);
  return (TRUE);

 escape:
  if (vec) free (vec);
  if (values) free (values);
  if (invec) FreeVectorArray (invec, Ninvec);
  dbFreeFields (fields, Nfields);
  SkyListFree (skylist);
  return (FALSE);

 help:
  gprint (GP_ERR, "USAGE: avmatch (RA) (DEC) (RADIUS) field[,field,field...]\n");
  gprint (GP_ERR, "   OR: avmatch -coords (filename.fits) (RADIUS) field[,field,field...]\n");
  gprint (GP_ERR, "   RADIUS is in arcseconds\n");

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

    // gprint (GP_ERR, "  type : dophot type (unused)\n");
    // gprint (GP_ERR, "  typefrac : dophot type fraction (unused)\n");
    return (FALSE);
  }
  gprint (GP_ERR, " avextract --help fields : for a complete listing of allowed fields\n");
  return (FALSE);
}
