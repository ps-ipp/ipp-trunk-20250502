# include "relastro.h"
# define MAX_TRANGE 180.0
# define MIN_PS1_DET 3
# define MAX_CHISQ_PM 1000000.0
# define MIN_NPOS_FRAC 1.0

# define NEXT_I { if (Ngroup < 2) slowMoving[ni] = TRUE; newI = TRUE; i++; continue; }
# define NEXT_J { j++; continue; }

int hpm_objects (SkyRegion *region, Catalog *catalog) {

  off_t i, j, J, ni, nj, *N1, k, m;
  int *slowMoving;
  double *X1, *Y1;
  double dX, dY, dR, RADIUS2;
  Coords tcoords;
  Catalog catalogOut, testcat;

  int Nsecfilt;
  char filename[1024];

  // we need at least 2 objects if we are going to match anything...
  if (catalog[0].Naverage < 2) return (TRUE);
  if (VERBOSE) fprintf (stderr, "checking "OFF_T_FMT" objects\n",  catalog[0].Naverage);

  // we save the best objects in an hpm dvodb
  snprintf (filename, 1024, "%s/%s.cpt", HIGH_SPEED_DIR, region[0].name);

  dvo_catalog_init (&catalogOut, TRUE); /* init new catalog */
  catalogOut.filename = strcreate(filename);

  Nsecfilt = GetPhotcodeNsecfilt();
  catalogOut.filename = filename;
  catalogOut.Nsecfilt = Nsecfilt;
  catalogOut.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT; // load all data
  
  catalogOut.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
  catalogOut.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data

  if (!dvo_catalog_open (&catalogOut, region, VERBOSE2,"w")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n",
	     filename);
    exit (2);
  }

  off_t NAVERAGE = 1000;
  off_t NMEASURE = 10000;
  REALLOCATE (catalogOut.average, Average, NAVERAGE);
  REALLOCATE (catalogOut.measure, Measure, NMEASURE);
  REALLOCATE (catalogOut.secfilt, SecFilt, NAVERAGE*Nsecfilt);
  catalogOut.Naverage = 0;
  catalogOut.Nmeasure = 0;

  // testcat is used to determine the fit for a single object group
  // objects which do not have a high-quality testcat fit are not kept
  dvo_catalog_init (&testcat, TRUE);
  testcat.Naverage = 1; // this is fixed -- only one obj in testcat
  testcat.Nsecfilt = Nsecfilt;
  off_t NMEASURE_TESTCAT = 1000;
  ALLOCATE (testcat.average, Average, 1);
  ALLOCATE (testcat.measure, Measure, NMEASURE_TESTCAT);
  ALLOCATE (testcat.secfilt, SecFilt, Nsecfilt);

  // mask with which to mark objects to be ignored
  ALLOCATE (slowMoving, int, catalog[0].Naverage);
  memset (slowMoving, 0, catalog[0].Naverage*sizeof(int));
  off_t Nslow = 0;

  time_t T2000 = ohana_date_to_sec ("2000/01/01");

  // mark (exclude) objects which do not meet the selection criterion
  for (i = 0; i < catalog[0].Naverage; i++) {

    int XVERB = FALSE;
    XVERB |= (catalog[0].average[i].objID == OBJ_ID_SRC) && (catalog[0].average[i].catID == CAT_ID_SRC);
    XVERB |= (catalog[0].average[i].objID == OBJ_ID_DST) && (catalog[0].average[i].catID == CAT_ID_DST);
    if (XVERB) {
      fprintf (stderr, "test object\n");
    }

    // count the PS1 detections via explicit photcode ranges?
    // XXX this is a total hard-wired hack...
    int Nps1 = 0;
    double Tmin = +100.0*365.0;  // +/- 100 years
    double Tmax = -100.0*365.0;  
    m = catalog[0].average[i].measureOffset;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {
      if (catalog[0].measure[m+j].photcode < 10000) continue;
      if (catalog[0].measure[m+j].photcode > 10600) continue;
      double To = (catalog[0].measure[m+j].t - T2000) / 86400.0 ; // time in days relative to J2000 in years
      Tmin = MIN(Tmin, To);
      Tmax = MAX(Tmax, To);
      Nps1 ++;
    }
    if (Nps1 < MIN_PS1_DET) {
      slowMoving[i] = TRUE;
      Nslow ++;
      continue;
    }      
    // selection criteria : (Nps1 > 2) && (Trange < 180)
    double Trange = Tmax - Tmin;
    if (Trange > MAX_TRANGE) {
      slowMoving[i] = TRUE;
      Nslow ++;
      continue;
    }
  }

  fprintf (stderr, OFF_T_FMT" slow, "OFF_T_FMT" total objects; "OFF_T_FMT" possible fast\n",  Nslow,  catalog[0].Naverage,  catalog[0].Naverage - Nslow);
  if (catalog[0].Naverage == Nslow) {
    fprintf (stderr, "no possible fast objects, skipping this catalog\n");

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalogOut, VERBOSE2)) { fprintf (stderr, "ERROR: failed to save %s\n", catalogOut.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalogOut)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalogOut.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&catalogOut);
    free (slowMoving);
    return TRUE;
  }
    
  // double loop over unmarked objects (sorted in RA / X)
  // record all pairs within the desired match distance

  // create tmp local positional index
  ALLOCATE (X1, double, catalog[0].Naverage);
  ALLOCATE (Y1, double, catalog[0].Naverage);
  ALLOCATE (N1, off_t,  catalog[0].Naverage);

  // define a local projection
  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  if (region[0].Dmax < 90) {
    tcoords.crval2 = 0.5*(region[0].Dmin + region[0].Dmax);
  } else {
    tcoords.crval2 = 90.0;
  }
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;
  strcpy (tcoords.ctype, "DEC--ARC");

  /* build spatial index (RA sort) referencing input array sequence */
  for (i = 0; i < catalog[0].Naverage; i++) {
    int status = RD_to_XY (&X1[i], &Y1[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N1[i] = i;
    assert (status);
  }
  sort_coords_index (X1, Y1, N1, catalog[0].Naverage);

  RADIUS2 = SQ(RADIUS);

  // group is a list of objects that are within a clump.  this set will be tested
  // via a clipped fit to the measurements after the group is identified
  int Ngroup = 0;
  int NGROUP = 100;
  int *group = NULL;
  ALLOCATE (group, int, NGROUP);

  // in the loop below, we need to do a bunch of things when we go to the next main object
  int newI = TRUE;

  // mark (skip) objects with both sets of target photcodes
  // the loop below is attempting to find associations of multiple objects which have 
  // passed the cuts above.  the index i is following the primary object of interest
  // the index j is used to explore possible near neighbors.  
  // When we go to the next object 'i', Nmatch is reset 
  for (i = j = 0; (i < catalog[0].Naverage) && (j < catalog[0].Naverage);) {

    ni = N1[i];
    nj = N1[j];

    if (newI) {
      Ngroup = 1;
      group[0] = ni;
      newI = FALSE;
    }

    int XVERB = FALSE;
    XVERB |= (catalog[0].average[ni].objID == OBJ_ID_SRC) && (catalog[0].average[ni].catID == CAT_ID_SRC);
    XVERB |= (catalog[0].average[nj].objID == OBJ_ID_DST) && (catalog[0].average[nj].catID == CAT_ID_DST);
    if (XVERB) {
      fprintf (stderr, "test object %d or %d\n", (int) ni, (int) nj);
    }

    if (slowMoving[ni]) NEXT_I;
    if (slowMoving[nj]) NEXT_J;

    if (!finite(X1[i]) || !finite(Y1[i])) NEXT_I;
    if (!finite(X1[j]) || !finite(Y1[j])) NEXT_J;

    // look for pairs that are within the maximum separation
    dX = X1[i] - X1[j];
    if (XVERB) {
      fprintf (stderr, "%d %d : %f\n", (int) ni, (int) nj, dX);
    }

    if (dX <= -1.02*RADIUS) NEXT_I; // negative dX: i is too small
    if (dX >= +1.02*RADIUS) NEXT_J; // positive dX: j is too small

    // within match range; look for valid matches & accumulate the group
    for (J = j; (dX > -1.02*RADIUS) && (J < catalog[0].Naverage); J++) {     
      if (J == i) continue;  // avoid auto-matches
      nj = N1[J];

      dX = X1[i] - X1[J];

      if (slowMoving[nj]) continue;

      XVERB  = (catalog[0].average[ni].objID == OBJ_ID_SRC) && (catalog[0].average[ni].catID == CAT_ID_SRC);
      XVERB |= (catalog[0].average[nj].objID == OBJ_ID_DST) && (catalog[0].average[nj].catID == CAT_ID_DST);
      if (XVERB) {
	fprintf (stderr, "test object pt2 %d or %d : %f\n", (int) ni, (int) nj, dX);
	fprintf (stderr, ".\n");
      }

      dY = Y1[i] - Y1[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;

      /*** a match is found ***/
      group[Ngroup] = nj;
      Ngroup ++;
      CHECK_REALLOCATE (group, int, NGROUP, Ngroup, 100);
    }

    if (Ngroup < 2) NEXT_I;

    // we now have spatially associated group of objects.  now we need to see if the set of
    // measurements can be fitted reasonably with a proper motion (& parallax?)

    // the mean object will start with info from the primary object
    // remember: testcat.Naverage = 1 -- does not change
    ni = group[0];
    int Nmatchmeas = 0;
    testcat.average[0] = catalog[0].average[ni];
    testcat.average[0].measureOffset = 0;
    for (J = 0; J < Ngroup; J++) {
      nj = group[J];
      m = catalog[0].average[nj].measureOffset;
      for (k = 0; k < catalog[0].average[nj].Nmeasure; k++) {
	testcat.measure[Nmatchmeas] = catalog[0].measure[m+k];
	// DROP: was needed when dR,dD were relative to average.R,D
	// testcat.measure[Nmatchmeas].R = catalog[0].measure[m+k].R;
	// testcat.measure[Nmatchmeas].D = catalog[0].measure[m+k].D;
	testcat.measure[Nmatchmeas].averef = 0;
	Nmatchmeas++;
	CHECK_REALLOCATE (testcat.measure, Measure, NMEASURE_TESTCAT, Nmatchmeas, 1000);
      }
    }
    testcat.average[0].Nmeasure = Nmatchmeas;
    testcat.Nmeasure = Nmatchmeas;
    populate_tiny_values (&testcat, DVO_TV_MEASURE);

    // we have now accumulated the measurements for this group, let's try a fit
    // this needs to be a (fairly robust) clipped fit or we will have a hard time
    // distinguishing a bad fit from a fit with 1 or 2 bad points
    FIT_MODE = FIT_PM_ONLY;
    UpdateObjects (&testcat, 1, 0);

    free_tiny_values(&testcat);

    // logic for keeping the fit (anything else?)
    int good = FALSE;
    good |= (testcat.average[0].ChiSqPM < MAX_CHISQ_PM);
    good |= (testcat.average[0].Npos > MIN_NPOS_FRAC * testcat.average[0].Nmeasure);
    good &= ((testcat.average[0].flags & ID_OBJ_FIT_PM) > 0);

    if (good) {
      // save the new object on catalogOut
      // (note: there is only one object in testcat and thus measureOffset = 0
      catalogOut.average[catalogOut.Naverage] = testcat.average[0];
      catalogOut.average[catalogOut.Naverage].measureOffset = catalogOut.Nmeasure;
      for (k = 0; k < testcat.average[0].Nmeasure; k++) {
	catalogOut.measure[catalogOut.Nmeasure] = testcat.measure[k];
	catalogOut.measure[catalogOut.Nmeasure].averef = catalogOut.Naverage;
	catalogOut.Nmeasure ++;
	CHECK_REALLOCATE (catalogOut.measure, Measure, NMEASURE, catalogOut.Nmeasure, 100);
      }
      catalogOut.Naverage ++;
      CHECK_REALLOCATE (catalogOut.average, Average, NAVERAGE, catalogOut.Naverage, 100);
    }
    NEXT_I;
  }
  catalogOut.Nsecfilt = Nsecfilt;
  catalogOut.Nsecfilt_mem = Nsecfilt * catalogOut.Naverage;

  populate_tiny_values (&catalogOut, DVO_TV_MEASURE);
  UpdateObjects (&catalogOut, 1, 0);
  free_tiny_values(&catalogOut);

  fprintf (stderr, "found "OFF_T_FMT" matches with "OFF_T_FMT" detections\n", catalogOut.Naverage, catalogOut.Nmeasure);
  dvo_catalog_save (&catalogOut, VERBOSE2);
  dvo_catalog_unlock (&catalogOut);
  dvo_catalog_free (&catalogOut);
  free (slowMoving);
  free (X1);
  free (Y1);
  free (N1);

  return TRUE;
}
