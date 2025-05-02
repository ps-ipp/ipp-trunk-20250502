# include "dvoshell.h"
int field_needs_images (dbField *field);

/* This function uses the 'find_match' algorithm to select the objects of interest.
   All measurements for matched objects result in an element in the output vectors.
   if -index (index) is supplied, the index of the input vectors are stored to match
   the output vectors.
 */

int mmatch (int argc, char **argv) {
  
  off_t i, j, k, n, m, *index;
  int N, Ncat, Npts, NPTS, last, Nfields, Nsecfilt, Ninvec;
  int VERBOSE;
  char name[1024];
  float RADIUS;

  Catalog catalog;

  Vector **vec, **invec, *RAvec, *DECvec, *IDXvec;
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

  // load info about the images from a reduced-size file
  char *imageMetadataFile = FALSE;
  if ((N = get_argument (argc, argv, "-image-metadata"))) {
    remove_argument (N, &argc, argv);
    imageMetadataFile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    remove_argument (N, &argc, argv);
    PARALLEL = TRUE;
  }

  // read RA,DEC coords from a fits file (esp for parallel dvo)
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
  // HOST_ID tells library if operation is on remote client or not
  dbExtractMeasuresInit(HOST_ID);

  // parse the fields to be extracted and returned
  // this is a syntax check that we can perform before reading the input vectors
  // NOTE: This block is missing in avmatch / avselect
  int first = CoordsFile ? 1 : 3;
  fields = dbCmdlineFields (argc-first, &argv[first], DVO_TABLE_MEASURE, &last, &Nfields);
  if (fields == NULL) goto help;
  if ((Nfields == 0) || (last != argc - first)) {
    dbFreeFields (fields, Nfields);
    dvo_catalog_free (&catalog);
    goto help;
  }

  // load image data if needed (for fields listed below)
  int loadImages = FALSE;
  for (i = 0; !loadImages && (i < Nfields); i++) {
    loadImages = field_needs_images (&fields[i]);
  }

  // this does all the work of re-packaging the command, calling it on the remote machines, then loading in the results
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

  if (PARALLEL && !HOST_ID) {

    // We need to copy the args to a temp array and modify them so that we send the
    // correct set to the remote client.  The args list looks like this:
    // if (!CoordsFile) : mmatch (RADIUS) field, ... [we removed RA & DEC above]
    // if ( CoordsFile) : mmatch (RADIUS) field, ... [because we stripped off the -coords filename elements]

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

      // XXX this is now set for both cases...
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

    // NOTE: image metadata is only needed to mmatch (not avmatch) since measures are tied to images, not averages
    if (loadImages) {
      Image *image;
      off_t Nimage;
      if ((image = LoadImagesDVO (&Nimage)) == NULL) goto escape;

      char *filename = abspath("image.metadata.fits", DVO_MAX_PATH);
      ImageMetadataSave (filename, image, Nimage);

      REALLOCATE (targv, char *, targc + 2);
      targv[targc+0] = strcreate ("-image-metadata");
      targv[targc+1] = strcreate (filename);
      targc += 2;
    }

    // call the remote client
    int status = HostTableParallelOps (skylist, targc, targv, RESULT_FILE, TRUE, 0, VERBOSE);
    if (vec) free (vec);
    
    // free up targv
    for (i = 0; i < targc; i++) {
      free (targv[i]);
    }
    free (targv);

    return status;
  } // END of remote call section

  RADIUS = atof (argv[1]);
  remove_argument (1, &argc, argv);

  // use the whole sky (since we select random points around the sky)
  SkyRegionSelection selection;
  selection.useDisplay = FALSE;
  selection.useSkyregion = FALSE;

  if (loadImages) {
    if (HOST_ID) {
      if (!SetImageMetadataSelection (imageMetadataFile)) goto escape;
    } else {
      if (!SetImageSelection (TRUE, &selection)) goto escape;
    }
  }


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

  // vectors to track recovered values
  int Nelem = RAvec->Nelements;
  ALLOCATE (index, off_t, Nelem);

  // int needLensing = dbFieldNeedLensing (fields, Nfields);
  int needStarpar = dbFieldNeedStarpar (fields, Nfields, FALSE);

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
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE;
    catalog.catflags |= needStarpar ? DVO_LOAD_STARPAR : DVO_SKIP_STARPAR;
    catalog.Nsecfilt = Nsecfilt;

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

    // returns the matches to AVERAGE objects; what do we do for each MEASURE?
    find_matches_by_vectors_closest (skylist[0].regions[i], &catalog, RAvec, DECvec, RADIUS, index);

    for (j = 0; (j < Nelem) && !interrupt; j++) {
      Ncat = index[j];

      if (Ncat == -1) continue;
      if (Ncat == -2) continue;

      m = catalog.average[Ncat].measureOffset;

      dbExtractMeasuresInitAve (); // reset counters for saved fields (costs very little)

      for (k = 0; (k < catalog.average[Ncat].Nmeasure); k++, m++) {
	if (catalog.measure[m].averef != Ncat) {
	  gprint (GP_ERR, "ERROR: inconsistent measure->average link.  Unsorted database?\n");
	  goto escape;
	}

	// extract the relevant values for this measurement
	dbExtractMeasuresInitMeas (); // reset counters for saved fields  (costs very little

	Average *average = &catalog.average[Ncat];

	int Nstarpar = average->starparOffset;
	StarPar *starpar = needStarpar ? &catalog.starpar[Nstarpar] : NULL;

	// int Nlensing = average->lensobjOffset;
	// Lensobj *lensobj = needLensobj ? &catalog.lensobj[m] : NULL;

	int Nsec = Ncat*Nsecfilt;
	SecFilt *secfilt = &catalog.secfilt[Nsec];

	for (n = 0; n < Nfields; n++) {
	  values[n] = dbExtractMeasures (average, secfilt, &catalog.measure[m], NULL, starpar, &fields[n]);
	}

	// XXX if we are allowed to return more rows than the supplied RA,DEC we will need to create an output RA,DEC
	for (n = 0; n < Nfields; n++) {
	  if (vec[n][0].type == OPIHI_FLT) {
	    vec[n][0].elements.Flt[Npts] = values[n].Flt;
	  } else {
	    vec[n][0].elements.Int[Npts] = values[n].Int;
	  }
	}
	if (IDXvec) {
	  IDXvec[0].elements.Int[Npts] = j;
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
	}
      }
    }

    dvo_catalog_free (&catalog);
  }
  ClearInterrupt (old_sigaction);

  // XXXX ???? this seems odd
  for (n = 0; n < Nfields; n++) {
    ResetVector (vec[n], fields[n].type, Npts);
  }
  if (IDXvec) {
    ResetVector (IDXvec, IDXvec->type, Npts);
  }

  // write vectors to a table (this is used by parallel dvo operations, but can be used elsewhere)
  // only write the fields which were in a valid catalog
  if (RESULT_FILE) {
    if (IDXvec) {
      // extend the array by one to hold index array
      Nfields ++;
      REALLOCATE (vec, Vector *, Nfields);
      vec[Nfields-1] = IDXvec;
    }
    int status = WriteVectorTableFITS (RESULT_FILE, "RESULT", NULL, vec, Nfields, FALSE, FALSE, NULL, 0);
    if (!status) goto escape;
  }

  if (vec) free (vec);
  if (values) free (values);
  if (invec) FreeVectorArray (invec, Ninvec);
  dbFreeFields (fields, Nfields);
  FreeImageSelection ();
  FreeImageMetadataSelection ();
  SkyListFree (skylist);
  return (TRUE);

 escape:
  if (vec) free (vec);
  if (values) free (values);
  if (invec) FreeVectorArray (invec, Ninvec);
  dbFreeFields (fields, Nfields);
  FreeImageSelection ();
  FreeImageMetadataSelection ();
  SkyListFree (skylist);
  return (FALSE);

 help:
  gprint (GP_ERR, "USAGE: mmatch (RA) (DEC) (RADIUS) field[,field,field...] [-index index]\n");
  gprint (GP_ERR, "   OR: mmatch -coords (filename.fits) (RADIUS) field[,field,field...] [-index index]\n");

  if ((argc > N + 1) && !strcasecmp (argv[N+1], "fields")) {
    gprint (GP_ERR, "  RA : right ascension (J2000) [degrees]\n");
    gprint (GP_ERR, "  DEC : declination [degrees]\n");
    return (FALSE);
  }
  gprint (GP_ERR, " mmatch --help fields : for a complete listing of allowed fields\n");
  return (FALSE);
}
