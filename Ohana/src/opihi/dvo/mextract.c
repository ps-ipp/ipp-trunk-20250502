# include "dvoshell.h"

int field_needs_images (dbField *field) {
  // the image subset table requires imageID for all fields

    if (!MEASURE_HAS_XCCD) {
      // I'm keeping this code because it gives a way of handling dvo dbs that don't have
      // measure.xccd if we need it
      if (field->ID == MEAS_XCCD) return TRUE; // full astrometry per chip (120 bytes!)
      if (field->ID == MEAS_YCCD) return TRUE; // full astrometry per chip (120 bytes!)
    }
    if (field->ID == MEAS_XMOSAIC) 	   return TRUE; // crval1,2 only
    if (field->ID == MEAS_YMOSAIC) 	   return TRUE; // crval1,2 only
    if (field->ID == MEAS_IMAGE_EXTERN_ID) return TRUE; // externID
    if (field->ID == MEAS_FLAT)            return TRUE; // Mcal
    if (field->ID == MEAS_CENTER_OFFSET)   return TRUE; // 0.5*NX, 0.5*NY
    if (field->ID == MEAS_EXPNAME_AS_INT)  return TRUE; // expname (or as int)
    if (field->ID == MEAS_MEAN_AIRMASS)    return TRUE; // airmass
    return FALSE;
}

void gfits_uncompress_timing ();

int mextract (int argc, char **argv) {
  
  off_t i, j, k, m; // used for counter averages and measures
  int n, N, Npts, NPTS, last, next, state, Nfields, Nreturn, Nstack;
  int Nsecfilt, VERBOSE, loadImages;
  char **cstack, name[1024];
  dbValue *values;

  Catalog catalog;
  SkyList *skylist;
  Vector **vec;
  dbField *fields;
  dbStack *stack;
  SkyRegionSelection *selection;

  /* defaults */
  vec = NULL;
  stack = NULL;
  fields = NULL;
  values = NULL;
  skylist = NULL;
  selection = NULL;

  // fprintf (stderr, "start...");
  if ((N = get_argument (argc, argv, "-h"))) goto help;
  if ((N = get_argument (argc, argv, "--help"))) goto help;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

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

  // this is used to NOT save the results in the results file
  // use this option when mextract is used in a script which does its
  // own job of packaging the results
  int SKIP_RESULTS = FALSE;
  if ((N = get_argument (argc, argv, "-skip-results"))) {
    remove_argument (N, &argc, argv);
    SKIP_RESULTS = TRUE;
  }

  // init here so free in 'escape' block does not crash
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
  // HOST_ID tells library if operation is on remote client or not
  dbExtractMeasuresInit(HOST_ID);

  // command-line is of the form: mextract field,field, field [where (field op value)...]

  // parse the fields to be extracted and returned
  fields = dbCmdlineFields (argc, argv, DVO_TABLE_MEASURE, &last, &Nfields);
  if (fields == NULL) return (FALSE);
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
  unsigned int Ncstack;
  cstack = isolate_elements (argc-next, &argv[next], &Ncstack);
  
  // construct the db Boolean math stack (frees cstack)
  stack = dbRPN (Ncstack, cstack, &Nstack);
  if (Ncstack && !Nstack) {
    print_error ();
    goto escape;
  }

  // add the skyregion limits to the where statement (or create)
  dbAstroRegionLimits (&stack, &Nstack, selection, DVO_TABLE_MEASURE);

  // parse stack elements into fields and scalars as needed
  Nreturn = Nfields; 
  if (!dbCheckStack (stack, Nstack, DVO_TABLE_MEASURE, &fields, &Nfields)) goto escape;
  // XXX handle errors

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  // load image data if needed (for fields listed below)
  loadImages = FALSE;
  for (i = 0; !loadImages && (i < Nfields); i++) {
    loadImages = field_needs_images (&fields[i]);
  }
    
  // this does all the work of re-packaging the command, calling it on the remote machines, then loading in the results
  if (PARALLEL && !HOST_ID) {

    // check for -region and apply to current sky region
    if (!SetSkyRegions (selection)) {
      FreeSkyRegionSelection (selection);
      dbFreeFields (fields, Nfields);
      dvo_catalog_free (&catalog);
      goto help;
    }

    // Image Metadata for remote queries:
    // 1) figure out if we need any image metadata
    // 2) load the images and generate a subset table with just the fields of interest
    // 3) add the input subset filename to the dvo_client command

    // allocate the temp array and copy all but (RA) (DEC)
    int targc = 0;
    char **targv = NULL;
    ALLOCATE (targv, char *, argc);
    for (i = 0; i < argc; i++) {
      targv[targc] = strcreate (argv[i]);
      targc ++;
    }

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

    dbFreeFields (fields, Nfields);
    dbFreeStack (stack, Nstack);
    free (stack);
    FreeSkyRegionSelection (selection);
    dvo_catalog_free (&catalog);

    // free up targv
    for (i = 0; i < targc; i++) {
      free (targv[i]);
    }
    free (targv);

    return status;
  }

  if (loadImages) {
    if (HOST_ID) {
      if (!SetImageMetadataSelection (imageMetadataFile)) goto escape;
    } else {
      if (!SetImageSelection (TRUE, selection)) goto escape;
    }
  }

  /* create storage vector */
  Npts = 0;
  NPTS = 1000;
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

  int needLensing = dbFieldNeedLensing (fields, Nfields);
  int needStarpar = dbFieldNeedStarpar (fields, Nfields, FALSE);

  // the lensing table does not have a good index to/from the measure table.  if we need lensing
  // parameters, we need to generate a lookup table to avoid excess iterations
  mySequenceType *lensingSeq = NULL;
  if (needLensing) {
    lensingSeq = mySequenceAlloc();
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
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.catflags |= needLensing ? DVO_LOAD_LENSING : DVO_SKIP_LENSING;
    catalog.catflags |= needStarpar ? DVO_LOAD_STARPAR : DVO_SKIP_STARPAR;
    catalog.Nsecfilt = Nsecfilt;

    if (VERBOSE) gprint (GP_ERR, "trying %s ("OFF_T_FMT" of "OFF_T_FMT")\n", catalog.filename,  i,  skylist[0].Nregions);
      
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      gprint (GP_ERR, "ERROR: failure to open catalog file %s\n", catalog.filename);
      return FALSE;
    }
    dvo_catalog_unlock (&catalog);

    /* XXX need to call dvo_catalog_chipcoords here passing the loaded images */

    // fprintf (stderr, "done read...");

    for (j = 0; (j < catalog.Naverage) && !interrupt; j++) {
      Average *average = &catalog.average[j];

      m = average->measureOffset;
      if (m > catalog.Nmeasure) {
	gprint (GP_ERR, "ERROR: inconsistent average->measure offset.  Unsorted database?\n");
	goto escape;
      }

      dbExtractMeasuresInitAve (); // reset counters for saved fields (costs very little)

      off_t Lj;
      off_t Loff = average->lensingOffset;

      if (needLensing) {
	mySequenceSetSize (lensingSeq, average->Nlensing);
	
	// generate an index for these lensing entries (based on Lj and imageID)
	for (Lj = 0; Lj < average->Nlensing; Lj++) {
	  mySequenceSetValue (lensingSeq, catalog.lensing[Loff + Lj].imageID, Lj);
	}
	
	mySequenceSort (lensingSeq);
      }

      for (k = 0; (k < average->Nmeasure); k++, m++) {
	if (catalog.measure[m].averef != j) {
	  gprint (GP_ERR, "ERROR: inconsistent measure->average link.  Unsorted database?\n");
	  goto escape;
	}

	// extract the relevant values for this measurement
	dbExtractMeasuresInitMeas (); // reset counters for saved fields  (costs very little

	int Nstarpar = average->starparOffset;
	StarPar *starpar = needStarpar && average->Nstarpar ? &catalog.starpar[Nstarpar] : NULL;

	Lensing *lensing = NULL;

	// a gpc1 analysis specific choice: only look for lensing if photcode is a warp one
	// EAM 2022.02.28 : need to add stack values because we ingested stack lensing entries (radial apertures) 
	if (needLensing && average->Nlensing && (catalog.measure[m].photcode >= 11000) && (catalog.measure[m].photcode <= 12500)) {
	  Lj = mySequenceGetEntry (lensingSeq, catalog.measure[m].imageID);
	  if (Lj >= 0) lensing = &catalog.lensing[Loff + Lj];
	}

	int Nsec = j*Nsecfilt;
	SecFilt *secfilt = &catalog.secfilt[Nsec];

	for (n = 0; n < Nfields; n++) {
	  values[n] = dbExtractMeasures (average, secfilt, &catalog.measure[m], lensing, starpar, &fields[n]);
	}
	// fprintf (stderr, "object: ave: %f, cat: %f, averef %d\n", fields[n].name, values[2], values[3], catalog.measure[m].averef);

	// test the conditional statement
	if (!dbBooleanCond (stack, Nstack, values)) continue;
	for (n = 0; n < Nreturn; n++) {
	  if (vec[n][0].type == OPIHI_FLT) {
	    vec[n][0].elements.Flt[Npts] = values[n].Flt;
	  } else {
	    vec[n][0].elements.Int[Npts] = values[n].Int;
	  }
	  // fprintf (stderr, "keep : field: %s, value: %f\n", fields[n].name, values[n]);
	}
	Npts++;
	if (Npts >= NPTS) {
	  NPTS += 2000;
	  for (n = 0; n < Nreturn; n++) {
	    REALLOCATE (vec[n][0].elements.Flt, opihi_flt, NPTS);
	  }
	}
      }
    }
    dvo_catalog_free (&catalog);
    // dbStackAllocPrint ();
    // dbStackAllocReset ();
    // dbStackFreePrint ();
    // dbStackFreeReset ();
  }
  ClearInterrupt (old_sigaction);
  interrupt = FALSE;

  // fprintf (stderr, "done load...");

  for (n = 0; n < Nreturn; n++) {
    ResetVector (vec[n], fields[n].type, Npts);
  }

  // write vectors to a table (this is used by parallel dvo operations, but can be used elsewhere)
  if (RESULT_FILE && !SKIP_RESULTS) {
    int status = WriteVectorTableFITS (RESULT_FILE, "RESULT", NULL, vec, Nreturn, FALSE, FALSE, NULL, 0);
    if (!status) goto escape;
  }

  if (vec) free (vec);
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  free (stack);
  FreeImageSelection ();
  FreeImageMetadataSelection ();
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);

  gfits_uncompress_timing();

  // fprintf (stderr, "done extr...\n");
  return (TRUE);

escape:
  if (vec) free (vec);
  free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack, Nstack);
  free (stack);
  FreeImageSelection ();
  FreeImageMetadataSelection ();
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  dvo_catalog_free (&catalog);
  return (FALSE);

 help:
  gprint (GP_ERR, "USAGE: mextract field[,field,field...] where (expression)\n");

  if ((argc > N + 1) && !strcasecmp (argv[N+1], "fields")) {
    gprint (GP_ERR, "  RA : right ascension (J2000) for detection [degrees]\n");
    gprint (GP_ERR, "  DEC : declination for detection [degrees]\n");
    gprint (GP_ERR, "  RA:ave : average right ascension (J2000) for object [degrees]\n");
    gprint (GP_ERR, "  DEC:ave : average declination for object [degrees]\n");
    gprint (GP_ERR, "  RA:err : ra scatter [degrees] \n");
    gprint (GP_ERR, "  DEC:err : dec scatter [degrees]\n");
    gprint (GP_ERR, "  GLON : galactic longitude [degrees]\n");
    gprint (GP_ERR, "  GLAT : galactic latitude [degrees]\n");
    gprint (GP_ERR, "  GLON:ave : average galactic longitude [degrees]\n");
    gprint (GP_ERR, "  GLAT:ave : average galactic latitude [degrees]\n");
    gprint (GP_ERR, "  ELON : ecliptic longitude [degrees]\n");
    gprint (GP_ERR, "  ELAT : ecliptic latitude [degrees]\n");
    gprint (GP_ERR, "  ELON:ave : average ecliptic longitude [degrees]\n");
    gprint (GP_ERR, "  ELAT:ave : average ecliptic latitude [degrees]\n");
    gprint (GP_ERR, "  uRA : proper motion in ra [mas/yr]\n");
    gprint (GP_ERR, "  uDEC : proper motion in dec [mas/yr]\n");
    gprint (GP_ERR, "  duRA : proper motion error in ra [mas/yr]\n");
    gprint (GP_ERR, "  duDEC : proper motion error in dec [mas/yr]\n");
    gprint (GP_ERR, "  PAR : parallax\n");
    gprint (GP_ERR, "  dPAR : parallax error \n");

    gprint (GP_ERR, "  dR : ra offset [arcseconds]\n");
    gprint (GP_ERR, "  dD : dec offset [arcseconds]\n");
    gprint (GP_ERR, "  dR:fit : ra offset from fit [arcseconds]\n");
    gprint (GP_ERR, "  dD:fit : dec offset from fit [arcseconds]\n");
    gprint (GP_ERR, "  dR:err : ra offset error [arcseconds]\n");
    gprint (GP_ERR, "  dD:err : dec offset error [arcseconds]\n");

    gprint (GP_ERR, "  ChiSqPos : chi square of position fit \n");
    gprint (GP_ERR, "  ChiSqPM  : chi square of proper-motion fit \n");
    gprint (GP_ERR, "  ChiSqPar : chi square of parallax fit \n");

    gprint (GP_ERR, "  Tmean : mean epoch (reference for proper motion)\n");
    gprint (GP_ERR, "  Trange : range of times used for proper motion/parallax fit\n");


    gprint (GP_ERR, "  Nmeas : number of measurements\n");
    gprint (GP_ERR, "  Nmiss : number of non-detections\n");
    gprint (GP_ERR, "  Npos  : number of measurments used for astrometry\n");

    gprint (GP_ERR, "  objflags  : object flags [alias: obj_flags]\n");
    gprint (GP_ERR, "  secflags  : average photometry flags [aliases: obj_phot_flags, sec_flags, secfilt_flags]\n");
    gprint (GP_ERR, "  photflags : detection flags from image analysis [alias: phot_flags]\n");
    gprint (GP_ERR, "  dbflags : detection flags from database analysis [alias: db_flags]\n");

    gprint (GP_ERR, "  obj_flags : object flags\n");
    gprint (GP_ERR, "  phot_flags : detection flags from image analysis\n");
    gprint (GP_ERR, "  db_flags : detection flags from database analysis\n");

    gprint (GP_ERR, "  airmass : airmass of detection\n");
    gprint (GP_ERR, "  meas_airmass : airmass of exposure\n");
    gprint (GP_ERR, "  alt : altitude of detection\n");
    gprint (GP_ERR, "  az  : azimuth of detection\n");
    gprint (GP_ERR, "  exptime : exposure time [s]\n");
    gprint (GP_ERR, "  photcode : photcode \n");
    gprint (GP_ERR, "  photcode:equiv : equivalent average photcode \n");
    gprint (GP_ERR, "  photcode:c : zero point of photcode \n");
    gprint (GP_ERR, "  photcode:klam : airmass slope of photcode \n");
    gprint (GP_ERR, "  time : time of exposure [Seconds since Jan 1, 1970/\n");
    gprint (GP_ERR, "  fwhm : fwhm (average) of fitted PSF [pixels]\n");
    gprint (GP_ERR, "  fwhm_maj : fwhm (major axis) of fitted PSF [pixels]\n");
    gprint (GP_ERR, "  fwhm_min : fwhm (minor axis) of fitted PSF [pixels]\n");
    gprint (GP_ERR, "  theta : position angle of fitted PSF\n");
    gprint (GP_ERR, "  posangle : position angle of detector at measurement [degrees]\n");
    gprint (GP_ERR, "  platescale : plate scale of detector at measurement [arcsec/pixel] (negative = sky parity)\n");

    gprint (GP_ERR, "  Mxx : second moment in X [pixels^2]\n");
    gprint (GP_ERR, "  Mxy : second moment cross term [pixels^2]\n");
    gprint (GP_ERR, "  Myy : second moment in Y [pixels^2]\n");

    gprint (GP_ERR, "  xccd : ccd x position\n");
    gprint (GP_ERR, "  yccd : ccd y position\n");
    gprint (GP_ERR, "  xoff : ccd x correction\n");
    gprint (GP_ERR, "  yoff : ccd y correction\n");
    gprint (GP_ERR, "  xccd:err : ccd x position error\n");
    gprint (GP_ERR, "  yccd:err : ccd y position error\n");

    gprint (GP_ERR, "  pos_sys_err : systematic position error\n");

    gprint (GP_ERR, "  xmosaic : mosaic x position\n");
    gprint (GP_ERR, "  ymosaic : mosaic y position\n");

    gprint (GP_ERR, "  xchip : chip x position (= ccd position)\n");
    gprint (GP_ERR, "  ychip : chip y position (= ccd position)\n");
    gprint (GP_ERR, "  xfpa : fpa x position (= mosaic position)\n");
    gprint (GP_ERR, "  yfpa : fpa y position (= mosaic position)\n");

    gprint (GP_ERR, "  detID : ID of detection (unique on source image)\n");
    gprint (GP_ERR, "  objID : object ID (32 bit, unique in catalog)\n");
    gprint (GP_ERR, "  catID : catalog ID (32 bit)\n");
    gprint (GP_ERR, "  imageID : ID of source image (32 bit)\n");
    gprint (GP_ERR, "  externID : externID of source image (32 bit)\n");

    gprint (GP_ERR, "  psf_qf : PSF quality factor (psf-weighted mask fraction)\n");
    gprint (GP_ERR, "  psf_qf_perfect : PSF quality factor, perfect mask version (psf-weighted mask fraction)\n");

    gprint (GP_ERR, "  psf_chisq : PSF fit chi square\n");
    gprint (GP_ERR, "  psf_ndof : PSF number of degrees of freedom\n");
    gprint (GP_ERR, "  psf_npix : PSF number of pixels\n");

    gprint (GP_ERR, "  cr_nsigma : Nsigma deviation towards cosmic ray\n");
    gprint (GP_ERR, "  ext_nsigma : Nsigma deviation towards extended source\n");

    gprint (GP_ERR, "  sky : sky model flux at measurement location\n");
    gprint (GP_ERR, "  sky_err : sky model stdev at measurement location\n");

    gprint (GP_ERR, "  Mcal_offset : difference wrt nominal zero point (clouds are positive)\n");
    gprint (GP_ERR, "  flat : flat-field correction (measure.Mcal - image.Mcal)\n");

    gprint (GP_ERR, "  center_offset : distance to image center\n");
    gprint (GP_ERR, "  flux : PSF flux\n");
    gprint (GP_ERR, "  flux_err : PSF flux error\n");
    gprint (GP_ERR, "  flux_psf : PSF flux\n");
    gprint (GP_ERR, "  flux_psf_err : PSF flux error\n");
    gprint (GP_ERR, "  flux_kron : KRON flux\n");
    gprint (GP_ERR, "  flux_kron_err : KRON flux error\n");

    gprint (GP_ERR, "  --- the following fields are selected by giving a photcode with the attached ending\n");
    gprint (GP_ERR, "  <photcode>:ave : average magnitude for photcode (or equivalent)\n");
    gprint (GP_ERR, "  <photcode>:ref : reference magnitude system for photcode (or equivalent)\n");
    gprint (GP_ERR, "  <photcode>:inst : instrumental magnitude for photcode\n");
    gprint (GP_ERR, "  <photcode>:cat :  catalog magnitude for photcode\n");
    gprint (GP_ERR, "  <photcode>:sys :  system magnitude for photcode\n");
    gprint (GP_ERR, "  <photcode>:rel :  relative magnitude for photcode\n");
    gprint (GP_ERR, "  <photcode>:cal :  calibrated magnitude for photcode \n");
    gprint (GP_ERR, "  <photcode>:err :  magnitude error for measurement\n");
    gprint (GP_ERR, "  <photcode>:ap :  catalog aperture magnitude for photcode\n");
    gprint (GP_ERR, "  <photcode>:aper :  catalog aperture magnitude for photcode\n");
    gprint (GP_ERR, "  <photcode>:aveerr : average error (stdev)\n");
    gprint (GP_ERR, "  <photcode>:chisq : raw chi-square of magnitude fit\n");
    gprint (GP_ERR, "  <photcode>:ncode : number of measurements in photcode\n");
    gprint (GP_ERR, "  <photcode>:nphot : number of measurements used for average magnitude\n");

    gprint (GP_ERR, "  <photcode>:aperinst : instrumental aperture magnitude\n");
    gprint (GP_ERR, "  <photcode>:aper_inst : instrumental aperture magnitude\n");
    gprint (GP_ERR, "  <photcode>:kron : kron mag\n");
    gprint (GP_ERR, "  <photcode>:kroninst : instrumental kron mag\n");
    gprint (GP_ERR, "  <photcode>:kron_inst : instrumental kron mag\n");
    gprint (GP_ERR, "  <photcode>:kronerr : kron mag error\n");

    gprint (GP_ERR, "  <photcode>:photflags : photometry flags for measurements\n");
    gprint (GP_ERR, "  <photcode>:flags : photometry flags for measurements\n");
    gprint (GP_ERR, "  <photcode>:fluxpsf : average psf flux\n");
    gprint (GP_ERR, "  <photcode>:fluxpsferr : average psf flux error\n");
    gprint (GP_ERR, "  <photcode>:fluxkron : average kron flux\n");
    gprint (GP_ERR, "  <photcode>:fluxkronerr : average kron flux error\n");

    return (FALSE);
  }
  gprint (GP_ERR, " mextract --help fields : for a complete listing of allowed fields\n");
  return (FALSE);
}
