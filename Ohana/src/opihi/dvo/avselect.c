# include "dvoshell.h"

/* This function uses the 'find_match' algorithm to select the objects of interest.
   Each entry in the match vectors (RA, DEC, RADIUS) yields a result value -- if no
   source matches, the resulting fields are all NAN.

   * choose the sky regions based on the provided RA,DEC points
   * loop over the catalogs
   * within a catalog, use the find_match code to find the matching coordinates
   * use dbExtractAverages to get the fields for the matched entry
 */

int avselect (int argc, char **argv) {

  off_t i, j, n, m;
  int N, Ncat, Npts, NPTS, last, Nfields, Nsecfilt, Ninvec;
  int VERBOSE;
  char name[1024];
  float RADIUS;

  Catalog catalog;

  Vector **vec, **invec, *RAvec, *DECvec, *IDXvec, *RADvec;
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

  IDXvec = NULL;
  if ((N = get_argument (argc, argv, "-index"))) {
    remove_argument (N, &argc, argv);
    IDXvec = SelectVector (argv[N], ANYVECTOR, TRUE);
    if (IDXvec == NULL) goto help;
    remove_argument (N, &argc, argv);
  }

  RADvec = NULL;
  if ((N = get_argument (argc, argv, "-radius"))) {
    remove_argument (N, &argc, argv);
    RADvec = SelectVector (argv[N], ANYVECTOR, TRUE);
    if (RADvec == NULL) goto help;
    remove_argument (N, &argc, argv);
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

  RADIUS = atof (argv[1]);

  /* load regions which contain all supplied RA,DEC coordinates */
  if ((skylist = SelectRegionsByCoordVectorsAndRadius (RAvec, DECvec, RADIUS/3600.0)) == NULL) goto escape;

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

    // if needed, add the index vector to the args list
    if (IDXvec) {
      REALLOCATE (targv, char *, targc + 2);
      targv[targc+0] = strcreate ("-index");
      targv[targc+1] = strcreate (IDXvec[0].name);
      targc += 2;
    }
    if (RADvec) {
      REALLOCATE (targv, char *, targc + 2);
      targv[targc+0] = strcreate ("-radius");
      targv[targc+1] = strcreate (RADvec[0].name);
      targc += 2;
    }

    // I need to pass the RA & DEC vectors to the remote clients...
    int status = HostTableParallelOps (skylist, targc, targv, RESULT_FILE, TRUE, 0, VERBOSE);
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
  // XXX with this block here, we do not get a check on the syntax until after the clients have launched
  // (see mmatch.c for a fix)
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
  Npts = 0;
  NPTS = 1000;
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
  }
  if (IDXvec) {
    ResetVector (IDXvec, OPIHI_INT, NPTS);
  }
  if (RADvec) {
    ResetVector (RADvec, OPIHI_FLT, NPTS);
  }

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

    // find all matches within the radius. index returns the elements of catalog which are matches
    off_t Nresult;
    AvselectResult *result = find_matches_by_vectors_allmatch (skylist[0].regions[i], &catalog, RAvec, DECvec, RADIUS, &Nresult);

    for (j = 0; (j < Nresult) && !interrupt; j++) {
      Ncat = result[j].Ncat;
      Average *average = &catalog.average[Ncat];

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

      // set resulting values
      for (n = 0; n < Nfields; n++) {
        if (vec[n][0].type == OPIHI_FLT) {
          vec[n][0].elements.Flt[Npts] = values[n].Flt;
        } else {
          vec[n][0].elements.Int[Npts] = values[n].Int;
        }
      }
      // set (optional) IDXvec and RADvec
      if (IDXvec) {
        IDXvec[0].elements.Int[Npts] = result[j].Nseq;;
      }
      if (RADvec) {
        RADvec[0].elements.Flt[Npts] = result[j].Roff;;
      }

      // extend length of output vectors
      Npts++;
      if (Npts >= NPTS) {
        NPTS += 2000;
        for (n = 0; n < Nfields; n++) {
          REALLOCATE (vec[n][0].elements.Flt, opihi_flt, NPTS);
        }
        if (IDXvec) {
          REALLOCATE (IDXvec[0].elements.Int, opihi_int, NPTS);
        }
        if (RADvec) {
          REALLOCATE (RADvec[0].elements.Flt, opihi_flt, NPTS);
        }
      }
    }
    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  for (i = 0; i < Nfields; i++) {
    vec[i][0].Nelements = Npts;
  }
  if (IDXvec) {
    IDXvec[0].Nelements = Npts;
  }
  if (RADvec) {
    RADvec[0].Nelements = Npts;
  }

  // write vectors to a table (this is used by parallel dvo operations, but can be used elsewhere)
  // only write the fields which were in a valid catalog
  if (RESULT_FILE) {
    int NfieldsOut = Nfields;
    if (IDXvec) {
      // extend the array by one to hold index array
      NfieldsOut ++;
      REALLOCATE (vec, Vector *, NfieldsOut);
      vec[NfieldsOut-1] = IDXvec;
    }
    if (RADvec) {
      // extend the array by one to hold index array
      NfieldsOut ++;
      REALLOCATE (vec, Vector *, NfieldsOut);
      vec[NfieldsOut-1] = RADvec;
    }
    int status = WriteVectorTableFITS (RESULT_FILE, "RESULT", NULL, vec, NfieldsOut, FALSE, FALSE, NULL, 0);
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
  gprint (GP_ERR, "USAGE: avselect (RA) (DEC) (RADIUS) field[,field,field...] [-index index] [-radius radius]\n");
  gprint (GP_ERR, "   OR: avselect -coords (filename.fits) (RADIUS) field[,field,field...]\n");
  gprint (GP_ERR, "   RADIUS is in arcseconds\n");
  gprint (GP_ERR, "   -index index : return an index into the input RA,DEC vectors (e.g, field[i] matches RA[index[i]])\n");
  gprint (GP_ERR, "   -radius radius : return the distance (in arcseconds) between the returned object and the matched coordinate\n");

  if ((argc > N + 1) && !strcasecmp (argv[N+1], "fields")) {
    gprint (GP_ERR, "  NOTE: the list below is incomplete\n");

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
