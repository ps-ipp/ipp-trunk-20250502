# include "dvoshell.h"
int field_needs_images (dbField *field);

int mmextract (int argc, char **argv) {
  
  off_t i, j, k, m;
  int n, N, Npts, NPTS, last, next, state;
  int Nfields, Nreturn, Nreturn_base, Nstack1, Nstack2;
  int Nwhere, Iwhere, Nmatch, Imatch, NTABLE, Nt1, Nt2, n1, n2;
  int Nsecfilt, VERBOSE, loadImages;
  char **cstack1, **cstack2, name1[1024], name2[1024];
  dbValue *values, **table1, **table2;

  Catalog catalog;
  SkyList *skylist;
  Vector **vec;
  dbField *fields;
  dbStack *stack1;
  dbStack *stack2;
  SkyRegionSelection *selection;

  /* defaults */
  vec = NULL;
  stack1 = NULL;
  stack2 = NULL;
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

  dvo_catalog_init (&catalog, TRUE);

  /* load photcode information */
  if (!InitPhotcodes ()) goto escape;
  Nsecfilt = GetPhotcodeNsecfilt ();
  
  // init locally static variables (time refs)
  dbExtractMeasuresInit(HOST_ID);

  // parse skyregion options
  if ((selection = SetRegionSelection (&argc, argv)) == NULL) goto escape;

  // command-line is of the form: avextract field,field, field [where (field op value)...]
  // mmextract [-noauto] field,field field where conditions matched to conditions
  // mmextract ra,dec,mag where photcode equiv g matched to photcode == 2MASS_J
  // mmextract -noauto ra,dec,mag where photcode equiv g matched to photcode == 2MASS_J
  // objectID matched is implied
  // -noauto means imageID is not matched

  // parse the fields to be extracted and returned : last points to end, or first 'where' or 'matched'
  fields = dbCmdlineFields (argc, argv, DVO_TABLE_MEASURE, &last, &Nfields);
  if (fields == NULL) goto escape;
  if (Nfields == 0) {
    FreeSkyRegionSelection (selection);
    dbFreeFields (fields, Nfields);
    dvo_catalog_free (&catalog);
    goto help;
  }

  // examine line for 'where' and 'match to'.  neither is required, but order is fixed
  state = dbCmdlineConditions (argc, argv, last, &next);
  if (state == DVO_DB_CMDLINE_ERROR) goto escape;

  if (state == DVO_DB_CMDLINE_IS_END) {
      Nwhere = Nmatch = 0;
      Iwhere = Imatch = 0;
  }

  if (state == DVO_DB_CMDLINE_IS_MATCH) {
      Nwhere = 0;
      Iwhere = 0;
      Imatch = next;
      Nmatch = argc - next;
  }

  if (state == DVO_DB_CMDLINE_IS_WHERE) {
      Iwhere = next;
      // find the end or the 'match to'
      for (last = next; (last < argc) && strcasecmp (argv[last], "match"); last++);
      state = dbCmdlineConditions (argc, argv, last, &next);
      if (state == DVO_DB_CMDLINE_ERROR) goto escape;
      if (state == DVO_DB_CMDLINE_IS_WHERE) goto escape;
      if (state == DVO_DB_CMDLINE_IS_END) {
	  Nwhere = argc - Iwhere;
	  Imatch = Nmatch = 0;
      } else {
	  Nwhere = last - Iwhere;
	  Imatch = next;
	  Nmatch = argc - Imatch;
      }
  }

  // parse the 'where' and 'matched to' segments of the line as boolean math expressions
  unsigned int Ncstack1, Ncstack2;
  cstack1 = isolate_elements (Nwhere, &argv[Iwhere], &Ncstack1);
  cstack2 = isolate_elements (Nmatch, &argv[Imatch], &Ncstack2);
  
  // construct the db Boolean math stack (frees cstack)
  stack1 = dbRPN (Ncstack1, cstack1, &Nstack1);
  if (Ncstack1 && !Nstack1) {
    print_error ();
    goto escape;
  }

  // construct the db Boolean math stack (frees cstack)
  stack2 = dbRPN (Ncstack2, cstack2, &Nstack2);
  if (Ncstack2 && !Nstack2) {
    print_error ();
    goto escape;
  }

  // XXX disallow skyregion limits in the matched to expression?

  // add the skyregion limits to the where statement (or create)
  dbAstroRegionLimits (&stack1, &Nstack1, selection, DVO_TABLE_MEASURE);
  dbAstroRegionLimits (&stack2, &Nstack2, selection, DVO_TABLE_MEASURE);

  // parse stack elements into fields and scalars as needed
  Nreturn_base = Nfields;
  Nreturn = 2*Nfields; // we are returning fieldi_1, fieldi_2 for the selected fields

  if (!dbCheckStack (stack1, Nstack1, DVO_TABLE_MEASURE, &fields, &Nfields)) goto escape;
  if (!dbCheckStack (stack2, Nstack2, DVO_TABLE_MEASURE, &fields, &Nfields)) goto escape;
  // XXX handle errors

  /* load region corresponding to selection above */
  if ((skylist = SelectRegions (selection)) == NULL) goto escape;

  // load image data if needed (for fields listed below)
  loadImages = FALSE;
  for (i = 0; !loadImages && (i < Nfields); i++) {
    loadImages = field_needs_images (&fields[i]);
  }
  if (loadImages && !SetImageSelection (TRUE, selection)) goto escape;

  /* create storage vector */
  ALLOCATE (values, dbValue, Nfields);
  ALLOCATE (vec, Vector *, Nreturn);

  for (i = 0; i < Nreturn_base; i++) {
    if (ISNUM(fields[i].name[0])) {
      sprintf (name1, "v_%s_1", fields[i].name);
      sprintf (name2, "v_%s_2", fields[i].name);
    } else {
      sprintf (name1, "%s_1", fields[i].name);
      sprintf (name2, "%s_2", fields[i].name);
    }
    if ((vec[2*i+0] = SelectVector (name1, ANYVECTOR, TRUE)) == NULL) goto escape;
    if ((vec[2*i+1] = SelectVector (name2, ANYVECTOR, TRUE)) == NULL) goto escape;
    ResetVector (vec[2*i+0], fields[i].type, fields[i].type);
    ResetVector (vec[2*i+1], fields[i].type, fields[i].type);
  }

  Npts = 0;
  NPTS = 1;

  // we save the selected measures for each average to temporary tables 1 and 2
  // XXX need to deal with the INT / DBL difference here...
  NTABLE = 100;
  ALLOCATE (table1, dbValue *, Nreturn_base);
  ALLOCATE (table2, dbValue *, Nreturn_base);
  for (i = 0; i < Nreturn_base; i++) {
    ALLOCATE (table1[i], dbValue, NTABLE);
    ALLOCATE (table2[i], dbValue, NTABLE);
  }

  // int needLensing = dbFieldNeedLensing (fields, Nfields);
  int needStarpar = dbFieldNeedStarpar (fields, Nfields, FALSE);

  // grab data from all selected sky regions
  struct sigaction *old_sigaction = SetInterrupt();

  for (i = 0; (i < skylist[0].Nregions) && !interrupt; i++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[i];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = Nsecfilt;

    if (VERBOSE) gprint (GP_ERR, "trying %s ("OFF_T_FMT" of "OFF_T_FMT")\n", catalog.filename,  i,  skylist[0].Nregions);
      
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      gprint (GP_ERR, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    /* XXX need to call dvo_catalog_chipcoords here passing the loaded images */

    for (j = 0; (j < catalog.Naverage) && !interrupt; j++) {
      m = catalog.average[j].measureOffset;
      dbExtractMeasuresInitAve (); // reset counters for saved fields 

      // XXX check that we have space to keep all Nmeasure
      if (NTABLE < catalog.average[j].Nmeasure) {
	NTABLE = catalog.average[j].Nmeasure;
	for (n = 0; n < Nreturn_base; n++) {
	  REALLOCATE (table1[n], dbValue, NTABLE);
	  REALLOCATE (table2[n], dbValue, NTABLE);
	}
      }

      // extract the matching measures for this object into the temp tables
      Nt1 = Nt2 = 0;
      for (k = 0; (k < catalog.average[j].Nmeasure); k++, m++) {

	// extract the relevant values for this measurement 
	dbExtractMeasuresInitMeas (); // reset counters for saved fields 

	Average *average = &catalog.average[j];

	int Nstarpar = average->starparOffset;
	StarPar *starpar = needStarpar ? &catalog.starpar[Nstarpar] : NULL;

	// int Nlensing = average->lensobjOffset;
	// Lensobj *lensobj = needLensobj ? &catalog.lensobj[m] : NULL;

	int Nsec = j*Nsecfilt;
	SecFilt *secfilt = &catalog.secfilt[Nsec];

	for (n = 0; n < Nfields; n++) {
	  // values needs to be a pointer to a type with FLT and INT (with a union, we would save a bit of memory...)
	  values[n] = dbExtractMeasures (average, secfilt, &catalog.measure[m], NULL, starpar, &fields[n]);
	}
	// fprintf (stderr, "object: ave: %f, cat: %f, averef %d\n", fields[n].name, values[2], values[3], catalog.measure[m].averef);

	// test the first conditional statement
	if (dbBooleanCond (stack1, Nstack1, values)) {
	  for (n = 0; n < Nreturn_base; n++) {
	    table1[n][Nt1] = values[n];
	    // fprintf (stderr, "keep : field: %s, value: %f\n", fields[n].name, values[n]);
	  }
	  Nt1 ++;
	}
	// test the second conditional statement
	if (dbBooleanCond (stack2, Nstack2, values)) {
	  for (n = 0; n < Nreturn_base; n++) {
	    table2[n][Nt2] = values[n];
	    // fprintf (stderr, "keep : field: %s, value: %f\n", fields[n].name, values[n]);
	  }
	  Nt2 ++;
	}
      }

      // XXX now do the join :: need to filter against automatch if -noauto is selected (record the index value k to test)
      for (n1 = 0; n1 < Nt1; n1++) {
	for (n2 = 0; n2 < Nt2; n2++) {
	  for (n = 0; n < Nreturn_base; n++) {
	    if (vec[2*n+0][0].type == OPIHI_FLT) {
	      vec[2*n+0][0].elements.Flt[Npts] = table1[n][n1].Flt;
	      vec[2*n+1][0].elements.Flt[Npts] = table2[n][n2].Flt;
	    } else {
	      vec[2*n+0][0].elements.Int[Npts] = table1[n][n1].Int;
	      vec[2*n+1][0].elements.Int[Npts] = table2[n][n2].Int;
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
      }
    }

    dvo_catalog_free (&catalog);
    // dbStackAllocPrint ();
    // dbStackAllocReset ();
    // dbStackFreePrint ();
    // dbStackFreeReset ();
  }
  ClearInterrupt (old_sigaction);

  // free excess memory
  for (n = 0; n < Nreturn; n++) {
    vec[n][0].Nelements = Npts;
    if (vec[n][0].type == OPIHI_FLT) {
      REALLOCATE (vec[n][0].elements.Flt, opihi_flt, MAX(1,Npts));
    } else {
      REALLOCATE (vec[n][0].elements.Int, opihi_int, MAX(1,Npts));
    }
  }

  for (n = 0; n < Nreturn_base; n++) {
    free (table1[n]);
    free (table2[n]);
  }
  free (table1);
  free (table2);

  free (values);

  dbFreeFields (fields, Nfields);
  dbFreeStack (stack1, Nstack1);
  dbFreeStack (stack2, Nstack2);
  free (stack1);
  free (stack2);
  FreeImageSelection ();
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  return (TRUE);

escape:
  if (vec) free (vec);
  if (values) free (values);
  dbFreeFields (fields, Nfields);
  dbFreeStack (stack1, Nstack1);
  dbFreeStack (stack2, Nstack2);
  free (stack1);
  free (stack2);
  FreeImageSelection ();
  SkyListFree (skylist);
  FreeSkyRegionSelection (selection);
  dvo_catalog_free (&catalog);
  return (FALSE);

 help:
  gprint (GP_ERR, "USAGE: mmextract field[,field,field...] where (expression) match to (expression)\n");
  gprint (GP_ERR, "  pairs of fields are returned: the first set are restricted by the 'where' expression, the second by the 'match to' expression\n");

  if ((argc > N + 1) && !strcasecmp (argv[N+1], "fields")) {
    gprint (GP_ERR, " USAGE: avextract field[,field,field...] where (expression)\n");
    gprint (GP_ERR, "  RA : right ascension (J2000) for detection\n");
    gprint (GP_ERR, "  DEC : declination for detection\n");
    gprint (GP_ERR, "  RA:ave : average right ascension (J2000) for object\n");
    gprint (GP_ERR, "  DEC:ave : average declination for object\n");
    gprint (GP_ERR, "  RA:err : ra scatter \n");
    gprint (GP_ERR, "  DEC:err : dec scatter\n");
    gprint (GP_ERR, "  uRA : proper motion in ra\n");
    gprint (GP_ERR, "  uDEC : proper motion in dec\n");
    gprint (GP_ERR, "  duRA : proper motion error in ra\n");
    gprint (GP_ERR, "  duDEC : proper motion error in dec\n");
    gprint (GP_ERR, "  PAR : parallax\n");
    gprint (GP_ERR, "  dPAR : parallax error \n");
    gprint (GP_ERR, "  nmeas : number of measurements\n");
    gprint (GP_ERR, "  nmiss : number of non-detections\n");
    gprint (GP_ERR, "  xp : positional chi-square\n");
    gprint (GP_ERR, "  objflag : object flags\n");
    gprint (GP_ERR, "  photcode:ave : average magnitude for photcode (or equivalent)\n");
    gprint (GP_ERR, "  photcode:ref : reference magnitude system for photcode (or equivalent)\n");
    gprint (GP_ERR, "  photcode:inst : instrumental magnitude for photcode\n");
    gprint (GP_ERR, "  photcode:cat :  catalog magnitude for photcode\n");
    gprint (GP_ERR, "  photcode:sys :  system magnitude for photcode\n");
    gprint (GP_ERR, "  photcode:rel :  relative magnitude for photcode\n");
    gprint (GP_ERR, "  photcode:cal :  calibrated magnitude for photcode \n");
    gprint (GP_ERR, "  photcode:err : magnitude error for photcode\n");
    gprint (GP_ERR, "  photcode:chisq : raw chi-square of magnitude fit\n");
    gprint (GP_ERR, "  photcode:ncode : number of measurements in photcode\n");
    gprint (GP_ERR, "  photcode:nphot : number of measurements used for average magnitude\n");
    gprint (GP_ERR, "  airmass : airmass of detection\n");
    gprint (GP_ERR, "  exptime : exposure time\n");
    gprint (GP_ERR, "  photcode : photcode \n");
    gprint (GP_ERR, "  time : time of exposure\n");
    gprint (GP_ERR, "  dR : ra offset\n");
    gprint (GP_ERR, "  dD : dec offset\n");
    gprint (GP_ERR, "  fwhm : fwhm (average)\n");
    gprint (GP_ERR, "  fwhm_maj : fwhm (major axis)\n");
    gprint (GP_ERR, "  fwhm_min : fwhm (minor axis)\n");
    gprint (GP_ERR, "  theta : position angle\n");
    gprint (GP_ERR, "  flags : detection flags\n");
    gprint (GP_ERR, "  xccd : ccd x position\n");
    gprint (GP_ERR, "  yccd : ccd y position\n");
    gprint (GP_ERR, "  xmosaic : mosaic x position\n");
    gprint (GP_ERR, "  ymosaic : mosaic y position\n");
    gprint (GP_ERR, "  xchip : chip x position\n");
    gprint (GP_ERR, "  ychip : chip y position\n");
    gprint (GP_ERR, "  xfpa : fpa x position\n");
    gprint (GP_ERR, "  yfpa : fpa y position\n");
    return (FALSE);
  }
  gprint (GP_ERR, " mmextract --help fields : for a complete listing of allowed fields\n");
  return (FALSE);
}
