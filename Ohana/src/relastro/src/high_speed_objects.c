# include "relastro.h"

# define NEXT_I { i++; continue; }
# define NEXT_J { j++; continue; }

int high_speed_objects (SkyRegion *region, Catalog *catalog) {

  off_t i, j, m, J, ni, nj, *N1;
  off_t Nslow, Ninvalid, NgroupA, NgroupB, NgroupAbad, NgroupBbad;
  off_t l, i1, NAVERAGE, NMEASURE;
  int *slowMoving, *groupA, *groupB, status, Nmatch, Nmatchmeas, Nepoch, nv[2], Nmatchmeasobj;
  int foundA, foundB, XVERB;
  double *X1, *Y1;
  double dX, dY, dR, RADIUS2;
  Coords tcoords;
  Catalog catalogOut;

  int Nsecfilt;
  char filename[1024];

  snprintf (filename, 1024, "%s/%s.cpt", HIGH_SPEED_DIR, region[0].name);
  fprintf (stderr, "%s\n",filename);

  dvo_catalog_init (&catalogOut, TRUE); /*initialise new catalogue*/
  catalogOut.filename = strcreate(filename);

  SIGMA_LIM = 0.0;

  Nsecfilt = GetPhotcodeNsecfilt();
  // off_t Naverage = catalog[0].Naverage;
  // off_t Nmeasure = catalog[0].Nmeasure;
  catalogOut.filename = filename; // based on the input name, need to keep everything below the catdir portion
  catalogOut.Nsecfilt = Nsecfilt;
  catalogOut.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT; // load all data
  
  catalogOut.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
  catalogOut.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data

  if (!dvo_catalog_open (&catalogOut, region, VERBOSE2,"w")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n",
	     filename);
    exit (2);
  }

  NAVERAGE = 1000;
  NMEASURE = 10000;
  REALLOCATE (catalogOut.average, Average, NAVERAGE);
  REALLOCATE (catalogOut.measure, Measure, NMEASURE);
  REALLOCATE (catalogOut.secfilt, SecFilt, NAVERAGE*Nsecfilt);

  // high-speed between different surveys (easier case):
  // we have two sets of photcodes (A) and (B) and are looking for objects
  // with detections in only (A) and separately only (B).

  // we need at least 2 objects if we are going to match anything...
  if (catalog[0].Naverage < 2) return (TRUE);

  // mask with which to mark objects to be ignored
  ALLOCATE (slowMoving, int, catalog[0].Naverage);
  memset (slowMoving, 0, catalog[0].Naverage*sizeof(int));

  // record to which photcode group the object belongs:
  NgroupA = 0;
  ALLOCATE (groupA, int, catalog[0].Naverage);
  memset (groupA, 0, catalog[0].Naverage*sizeof(int));

  NgroupB = 0;
  ALLOCATE (groupB, int, catalog[0].Naverage);
  memset (groupB, 0, catalog[0].Naverage*sizeof(int));

  if (VERBOSE) fprintf (stderr, "checking "OFF_T_FMT" objects\n",  catalog[0].Naverage);
  Nslow = 0;
  Ninvalid = 0;
  NgroupAbad = 0;
  NgroupBbad = 0;

  // mark (exclude) objects with both sets of target photcodes
  for (i = 0; i < catalog[0].Naverage; i++) {

    XVERB  = (catalog[0].average[i].objID == OBJ_ID_SRC) && (catalog[0].average[i].catID == CAT_ID_SRC);
    XVERB |= (catalog[0].average[i].objID == OBJ_ID_DST) && (catalog[0].average[i].catID == CAT_ID_DST);
    if (XVERB) {
      fprintf (stderr, "test object\n");
    }

    // do any of the measures for this object match group A?
    m = catalog[0].average[i].measureOffset;
    foundA = FALSE;
    for (j = 0; !foundA && (j < catalog[0].average[i].Nmeasure); j++, m++) {

      if (MeasMatchesPhotcode(&catalog[0].measure[m], photcodesGroupA, NphotcodesGroupA)) {
	foundA = TRUE;
      }
    }

    // do any of the measures for this object match group B?
    m = catalog[0].average[i].measureOffset;
    foundB = FALSE;
    for (j = 0; !foundB && (j < catalog[0].average[i].Nmeasure); j++, m++) {
          
      if (MeasMatchesPhotcode(&catalog[0].measure[m], photcodesGroupB, NphotcodesGroupB)) {
	foundB = TRUE;
      }
    }

    // object found in both - mark as slow moving
    if (foundA && foundB) {
      slowMoving[i] = TRUE;
      Nslow ++;
      continue;
    }

    // Apply additional constraints:
    if (foundA && !foundB) {
        if (applyConstraintsA(catalog, i)) {
            groupA[i] = TRUE;
            NgroupA++;
            continue;
        } else {
            NgroupAbad ++;
            // fprintf (stderr, "skip "OFF_T_FMT" (group A)\n",  i);
        }
	continue;
    } 
    if (foundB && !foundA) {
        if (applyConstraintsB(catalog, i)) {
            groupB[i] = TRUE;
            NgroupB++;
            continue;
        } else {
            NgroupBbad++;
        }
	continue;
    }
    // this object does not have a detection matching the contraints from either gorupA or groupB -- skip as if slow
    slowMoving[i] = TRUE;
    Ninvalid ++;
  }

  fprintf (stderr, OFF_T_FMT" slow, "OFF_T_FMT" invalid, ("OFF_T_FMT" group A, "OFF_T_FMT" group B), "OFF_T_FMT" total objects; "OFF_T_FMT" in group A, "OFF_T_FMT" in group B\n",  Nslow,  Ninvalid, NgroupAbad, NgroupBbad, catalog[0].Naverage,  NgroupA,  NgroupB);
    
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

  /* build spatial index (RA sort) referencing input array sequence */
  for (i = 0; i < catalog[0].Naverage; i++) {
    status = RD_to_XY (&X1[i], &Y1[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N1[i] = i;
    assert (status);
  }
  sort_coords_index (X1, Y1, N1, catalog[0].Naverage);

  Nmatch = 0;
  Nmatchmeas = 0;
  RADIUS2 = SQ(RADIUS);

  // mark (exclude) objects with both sets of target photcodes
  for (i = j = 0; (i < catalog[0].Naverage) && (j < catalog[0].Naverage);) {

    ni = N1[i];
    nj = N1[j];

    XVERB  = (catalog[0].average[ni].objID == OBJ_ID_SRC) && (catalog[0].average[ni].catID == CAT_ID_SRC);
    XVERB |= (catalog[0].average[nj].objID == OBJ_ID_DST) && (catalog[0].average[nj].catID == CAT_ID_DST);
    if (XVERB) {
      fprintf (stderr, "test object %d or %d\n", (int) ni, (int) nj);
    }

    if (slowMoving[ni]) NEXT_I;
    if (slowMoving[nj]) NEXT_J;

    // i => groupA, j => groupB
    if (!groupA[ni]) NEXT_I;
    if (!groupB[nj]) NEXT_J;

    if (!finite(X1[i]) || !finite(Y1[i])) NEXT_I;
    if (!finite(X1[j]) || !finite(Y1[j])) NEXT_J;

    // look for pairs that are within the maximum separation
    dX = X1[i] - X1[j];
    if (XVERB) {
      fprintf (stderr, "%d %d : %f\n", (int) ni, (int) nj, dX);
    }

    if (dX <= -1.02*RADIUS) NEXT_I; // negative dX: i is too small
    if (dX >= +1.02*RADIUS) NEXT_J; // positive dX: j is too small

    // within match range; look for valid matches
    for (J = j; (dX > -1.02*RADIUS) && (J < catalog[0].Naverage); J++) {     
      if (J == i) continue;  // avoid auto-matches

      dX = X1[i] - X1[J];

      nj = N1[J];
      if (!groupB[nj]) continue;

      XVERB  = (catalog[0].average[ni].objID == OBJ_ID_SRC) && (catalog[0].average[ni].catID == CAT_ID_SRC);
      XVERB |= (catalog[0].average[nj].objID == OBJ_ID_DST) && (catalog[0].average[nj].catID == CAT_ID_DST);
      if (XVERB) {
	fprintf (stderr, "test object pt2 %d or %d : %f\n", (int) ni, (int) nj, dX);
	fprintf (stderr, ".\n");
      }

      dY = Y1[i] - Y1[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;

      /*** a match is found (just print it for the moment) ***/
      /*Define a vector of matched indices and set averages in new catalogue*/
      Nepoch=2;
      FIT_MODE = FIT_PM_ONLY;
      nv[0]=ni; /*THESE SHOULD BE CHANGED FOR MULTIPLE EPOCHS AS SHOULD nv*/
      nv[1]=nj;

      catalogOut.average[Nmatch]=catalog[0].average[nv[0]];
      /*Loop over index values and set measurements in new catalogue*/
      Nmatchmeasobj=0;
      catalogOut.average[Nmatch].measureOffset=Nmatchmeas;
      for(l=0;l<Nepoch;l++) {
	  m = catalog[0].average[nv[l]].measureOffset;
	  for(i1=0;i1<catalog[0].average[nv[l]].Nmeasure;i1++) {
	      catalogOut.measure[Nmatchmeas]=catalog[0].measure[m+i1];
	      // DROP no longer necessary to repoint R,D
	      // catalogOut.measure[Nmatchmeas].dR=catalog[0].measure[m+i1].dR+3600.0*(catalog[0].average[nv[0]].R-catalog[0].average[nv[l]].R);
	      // catalogOut.measure[Nmatchmeas].dD=catalog[0].measure[m+i1].dD+3600.0*(catalog[0].average[nv[0]].D-catalog[0].average[nv[l]].D);
	      catalogOut.measure[Nmatchmeas].averef = Nmatch;
	      Nmatchmeasobj++;
	      Nmatchmeas++;

	      if (Nmatchmeas == NMEASURE - 1) {
		NMEASURE += 10000;
		REALLOCATE (catalogOut.measure, Measure, NMEASURE);
	      }
	  }
      }
      catalogOut.average[Nmatch].Nmeasure = Nmatchmeasobj;
      Nmatch ++;

      if (Nmatch == NAVERAGE - 1) {
	NAVERAGE += 1000;
	REALLOCATE (catalogOut.average, Average, NAVERAGE);
	REALLOCATE(catalogOut.secfilt, SecFilt, NAVERAGE*Nsecfilt);
      }
    }
    i++;
  }
  catalogOut.Naverage=Nmatch;
  catalogOut.Nmeasure=Nmatchmeas;
  catalogOut.Nsecfilt=Nsecfilt;
  catalogOut.Nsecfilt_mem=Nmatch*Nsecfilt;

  populate_tiny_values (&catalogOut, DVO_TV_MEASURE);
  UpdateObjects (&catalogOut, 1, 0);
  free_tiny_values(&catalogOut);

  fprintf (stderr, "found %d matches\n", Nmatch);

  SetProtect (TRUE);
  if (!dvo_catalog_save (&catalogOut, VERBOSE2)) { fprintf (stderr, "ERROR: failed to save %s\n", catalogOut.filename); exit (1); }
  if (!dvo_catalog_unlock (&catalogOut)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalogOut.filename); exit (1); }
  SetProtect (FALSE);

  dvo_catalog_free (&catalogOut);
  free (slowMoving);
  free (groupA);
  free (groupB);
  free (X1);
  free (Y1);
  free (N1);

  return TRUE;
}

// return TRUE if measure->photcode match any in the photcodeSet
// (but, return FALSE if the measurement is bad)
int MeasMatchesPhotcode(Measure *measure, PhotCode **photcodeSet, int Nset) {

  int found, k;

  if (!Nset) return TRUE;
  // XXX require a set or not?  assert (Nset > 0);

  if (!finite(measure[0].R)) return FALSE;
  if (!finite(measure[0].D)) return FALSE;
  if (!finite(measure[0].M)) return FALSE;
  
  float dX = GetAstromError (measure, ERROR_MODE_RA);
  if (isnan(dX)) return FALSE;

  float dY = GetAstromError (measure, ERROR_MODE_DEC);
  if (isnan(dY)) return FALSE;

  /* select measurements by photcode, or equiv photcode, if specified */
  found = FALSE;
  for (k = 0; (k < Nset) && !found; k++) {
    if (photcodeSet[k][0].code == measure[0].photcode) found = TRUE;
    if (photcodeSet[k][0].code == GetPhotcodeEquivCodebyCode(measure[0].photcode)) found = TRUE;
  }

  return found;
}
