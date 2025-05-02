# include "addstar.h"

int find_matches_closest_refstars (SkyRegion *region, Catalog *srccat, Catalog *tgtcat, AddstarClientOptions options) {

  off_t i, j, J, Jmin, status, mSrc;
  double RADIUS, RADIUS2, Rmin;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2;
  off_t Nave, NAVE, Nmeas, NMEAS, Nstars, Nmatch;
  unsigned int objID, catID;
  Coords tcoords;
  int Nsecfilt;

  /* photcode data -- should not have to modify secfilt / average */
  Nsecfilt = GetPhotcodeNsecfilt ();

  /** allocate local arrays (srccat) **/
  ALLOCATE (X1, double, srccat->Naverage);
  ALLOCATE (Y1, double, srccat->Naverage);
  ALLOCATE (N1, off_t,  srccat->Naverage);

  if (!srccat->found_t) {
    ALLOCATE (srccat->found_t, off_t, srccat->Naverage);
    for (i = 0; i < srccat->Naverage; i++) {
      srccat->found_t[i] = -1;
    }
  }

  /** allocate local arrays (tgtcat) **/
  NAVE = Nave = tgtcat[0].Naverage;
  ALLOCATE (X2, double, NAVE);
  ALLOCATE (Y2, double, NAVE);
  ALLOCATE (N2, off_t,  NAVE);

  if (!tgtcat->found_t) {
    ALLOCATE (tgtcat[0].found_t, off_t, NAVE);
    for (i = 0; i < NAVE; i++) {
      tgtcat->found_t[i] = -1;
    }
  }
  /* for secfilt j and star i, secfilt[i*Nsecfilt+j] */

  /* internal counters */
  Nmatch = 0;
  NMEAS = Nmeas = tgtcat[0].Nmeasure;
  
  off_t *next_meas = NULL;

  // current max obj ID for this tgtcat
  objID = tgtcat[0].objID;
  catID = tgtcat[0].catID;

  /* project onto rectilinear grid with 1 arcsec pixels. the choice of ARC projection has
   * the advantage that every point in R,D has a mapping to a unique X,Y.  However, note
   * that not all possible X,Y points map back to R,D and the local plate scale changes
   * substantially far from the projection pole. We use the center of the region (tgtcat)
   * for crval1,2.
   */
  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  if ((region[0].Dmax < 90) && (region[0].Dmin > -90)) {
    tcoords.crval2 = 0.5*(region[0].Dmin + region[0].Dmax);
  } else {
    tcoords.crval2 = (region[0].Dmax >= 90) ? 90.0 : -90.0;
  }
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;
  
  /* build spatial index (RA sort) referencing input array sequence */
  Nstars = 0;
  for (i = 0; i < srccat->Naverage; i++) {
    status = RD_to_XY (&X1[Nstars], &Y1[Nstars], srccat->average[i].R, srccat->average[i].D, &tcoords);
    if (!status) continue;
    N1[Nstars] = i;
    Nstars ++;
  }
  if (Nstars < 1) {
    if (VERBOSE) fprintf (stderr, "skipping %s, no overlapping stars\n", tgtcat[0].filename);
    free (X1);
    free (Y1);
    free (N1);
    free (X2);
    free (Y2);
    free (N2);
    return (0);
  }
  if (Nstars > 1) sort_coords_index (X1, Y1, N1, Nstars);
  
  /* build spatial index (RA sort) */
  for (i = 0; i < Nave; i++) {
    RD_to_XY (&X2[i], &Y2[i], tgtcat[0].average[i].R, tgtcat[0].average[i].D, &tcoords);
    N2[i] = i;
    tgtcat[0].found_t[N2[i]] = -1;
  }
  if (Nave > 1) sort_coords_index (X2, Y2, N2, Nave);

  /* set up pointers for linked list of measure */
  if (tgtcat[0].sorted && (tgtcat[0].Nmeasure == tgtcat[0].Nmeasure_disk)) {
    // this version is only valid if we have done a full tgtcat load, and if the tgtcat
    // is sorted while processed
    next_meas = init_measure_links (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas);
  } else {
    next_meas = build_measure_links (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas);
  }    

  /* choose a radius for matches */
  if (options.radius == 0) {
    RADIUS = 2.0; /* hardwired default for refstars */
  } else {
    RADIUS = options.radius; /* provided by config */
  }
  RADIUS2 = RADIUS*RADIUS;

  /****************** find matched stars ********************/

  for (i = j = 0; (i < Nstars) && (j < Nave); ) {
    if (!finite(X1[i]) || !finite(Y1[i])) { 
      i++; 
      continue;
    }
    if (!finite(X2[j]) || !finite(Y2[j])) { 
      j++; 
      continue;
    }
    
    /* negative dX: j is too large */
    dX = X1[i] - X2[j];
    if (dX <= -1.02*RADIUS) {
      i++;
      continue;
    }
    /* positive dX, i is too large */
    if (dX >= 1.02*RADIUS) {
      j++;
      continue;
    }

    // XXX check that this is allocated
    if (srccat->found_t[N1[i]] != -1) {
        /* this star has already been assigned to an object in this or another tgtcat */
        i++;
        continue;
    }

    /* within match range; look for matches */
    Jmin = -1;
    Rmin = RADIUS2;
    for (J = j; (dX > -1.02*RADIUS) && (J < Nave); J++) {
      /* find closest match for this detection */
      dX = X1[i] - X2[J];
      dY = Y1[i] - Y2[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;
      if (dR < Rmin) {
	Rmin = dR;
	Jmin  = J;
      }
    }

    /* no match, try next detection */ 
    if (Jmin == -1) {
      i++;
      continue;
    }

    /*** a match is found, add to average, measure ***/
    Nmatch ++;
    off_t n = N2[Jmin];
    off_t N = N1[i];

    /* make sure there is space for next entry */
    if (Nmeas >= NMEAS - srccat->average[N].Nmeasure) {
      NMEAS = Nmeas + 1000 + srccat->average[N].Nmeasure;
      REALLOCATE (next_meas, off_t, NMEAS);
      REALLOCATE (tgtcat[0].measure, Measure, NMEAS);
    }

    /** add measurements for this star **/
    off_t mOff = srccat->average[N].measureOffset;
    for (mSrc = 0; mSrc < srccat->average[N].Nmeasure; mSrc++) {
      /** in replace mode, search for entry and replace values M, dM, R, D */
      // XXX this fails for unsorted tgtcats, right?
      if (options.replace && replace_match (&tgtcat[0].average[n], tgtcat[0].measure, &srccat->measure[mSrc + mOff], &srccat->found_t[N])) continue;

      /* add to end of measurement list */
      add_meas_link (&tgtcat[0].average[n], next_meas, Nmeas, NMEAS);
      
      // set the new measurements
      tgtcat[0].measure[Nmeas]          = srccat->measure[mSrc + mOff];

      // measure now carries R,D (not dR,dD) 
      tgtcat[0].measure[Nmeas].dbFlags  = 0;
      tgtcat[0].measure[Nmeas].averef   = n;
      tgtcat[0].measure[Nmeas].objID    = tgtcat[0].average[n].objID;
      tgtcat[0].measure[Nmeas].catID    = tgtcat[0].catID;

      float dRoff = dvoOffsetR(&tgtcat[0].measure[Nmeas], &tgtcat[0].average[n]);

      // rationalize dR:
      if (dRoff > +180.0*3600.0) {
	// average on high end of boundary, move star up
	tgtcat[0].measure[Nmeas].R += 360.0;
	dRoff -= 360.0*3600.0;
      }
      if (dRoff < -180.0*3600.0) {
	// average on low end of boundary, move star down
	tgtcat[0].measure[Nmeas].R -= 360.0;
	dRoff += 360.0*3600.0;
      }
      if (fabs(dRoff) > 10*RADIUS) {
	// take declination into account and check again.
	double cosD = cos(RAD_DEG*tgtcat[0].average[n].D);
	if (fabs(dRoff*cosD) > 10*RADIUS) {
	  fprintf (stderr, "error: %10.6f,%10.6f vs %10.6f,%10.6f (%f,%f vs %f,%f)\n", 
		   tgtcat[0].average[n].R, tgtcat[0].average[n].D, 
		   srccat->average[N].R, srccat->average[N].D,
		   X1[i], X2[Jmin], 
		   Y1[i], Y2[Jmin]);
	  // XXX abort on this? -- this is a bad failure...
	}
      }

      // the reference data may not have per-object time specified
      if (TIMEREF) {
	tgtcat[0].measure[Nmeas].t        = TIMEREF;
	tgtcat[0].measure[Nmeas].t_msec   = 0;
      }

      // we can choose to accept the proper-motion and parallax from the reference tgtcat
      if (ACCEPT_MOTION) {
	tgtcat[0].average[n].dR         = srccat->average[N].dR;
	tgtcat[0].average[n].dD         = srccat->average[N].dD;
	tgtcat[0].average[n].uR         = srccat->average[N].uR;
	tgtcat[0].average[n].uD         = srccat->average[N].uD;
	tgtcat[0].average[n].duR        = srccat->average[N].duR;
	tgtcat[0].average[n].duD        = srccat->average[N].duD;
	tgtcat[0].average[n].P          = srccat->average[N].P;
	tgtcat[0].average[n].dP         = srccat->average[N].dP;
	tgtcat[0].average[n].Tmean      = srccat->average[N].Tmean;
      }

    /** don't update average / secfilt values for REF photcodes **/

      tgtcat[0].average[n].Nmeasure ++;
      Nmeas ++;
    }

    srccat->found_t[N] = n;
    tgtcat[0].found_t[n] = -1;
    i++;
  }

  /*************** add unmatched stars *************************/

  /* Incorporate unmatched refcat stars.  Skip this step if we want to
     require matches combined with -replace, this lets us keep the
     reference up-to-date with known stars only */

  for (i = 0; (i < Nstars) && !options.only_match; i++) {
    /* make sure there is space for next entry */
    if (Nmeas >= NMEAS - srccat->average[i].Nmeasure) {
      NMEAS = Nmeas + 1000 + srccat->average[i].Nmeasure;
      REALLOCATE (next_meas, off_t, NMEAS);
      REALLOCATE (tgtcat[0].measure, Measure, NMEAS);
    }
    if (Nave >= NAVE) {
      NAVE = Nave + 1000;
      REALLOCATE (tgtcat[0].average, Average, NAVE);
      REALLOCATE (tgtcat[0].found_t, off_t, NAVE);
      if (tgtcat[0].secfilt) {
	// we only update the secfilt table if it has been allocated for output
	REALLOCATE (tgtcat[0].secfilt, SecFilt, NAVE*tgtcat[0].Nsecfilt);
      }
    }

    if (srccat->found_t[i] != -1) continue;
    if (!IN_REGION (srccat->average[i].R, srccat->average[i].D)) continue;

    dvo_average_init (&tgtcat[0].average[Nave]);
    tgtcat[0].average[Nave].R         	   = srccat->average[i].R;
    tgtcat[0].average[Nave].D         	   = srccat->average[i].D;

    tgtcat[0].average[Nave].Nmeasure  	   = srccat->average[i].Nmeasure;
    tgtcat[0].average[Nave].measureOffset  = Nmeas;
    tgtcat[0].average[Nave].objID     	   = objID;
    tgtcat[0].average[Nave].catID     	   = catID;

    if (PSPS_ID) {
        tgtcat[0].average[Nave].extID = CreatePSPSObjectID(tgtcat[0].average[Nave].R, tgtcat[0].average[Nave].D);
    }

    if (ACCEPT_MOTION) {
      tgtcat[0].average[Nave].dR    	   = srccat->average[i].dR;
      tgtcat[0].average[Nave].dD    	   = srccat->average[i].dD;
      tgtcat[0].average[Nave].uR    	   = srccat->average[i].uR;
      tgtcat[0].average[Nave].uD    	   = srccat->average[i].uD;
      tgtcat[0].average[Nave].duR   	   = srccat->average[i].duR;
      tgtcat[0].average[Nave].duD   	   = srccat->average[i].duD;
      tgtcat[0].average[Nave].P     	   = srccat->average[i].P;
      tgtcat[0].average[Nave].dP    	   = srccat->average[i].dP;
      tgtcat[0].average[Nave].Tmean   	   = srccat->average[i].Tmean;
    }

    objID ++;

    for (j = 0; j < Nsecfilt; j++) {
      dvo_secfilt_init (&tgtcat[0].secfilt[Nave*Nsecfilt+j], SECFILT_RESET_ALL);
    }

    off_t mOff = srccat->average[i].measureOffset;
    for (mSrc = 0; mSrc < srccat->average[i].Nmeasure; mSrc++) {
      // supply the measurements from this detection
      dvo_measure_init (&tgtcat[0].measure[Nmeas]);
      tgtcat[0].measure[Nmeas] = srccat->measure[mSrc + mOff];

      // the following measure elements cannot be set until here:
      tgtcat[0].measure[Nmeas].dbFlags  = 0;
      tgtcat[0].measure[Nmeas].averef   = Nave;
      tgtcat[0].measure[Nmeas].objID    = tgtcat[0].average[Nave].objID;
      tgtcat[0].measure[Nmeas].catID    = tgtcat[0].catID;

      if (TIMEREF) {
	tgtcat[0].measure[Nmeas].t      = TIMEREF;
	tgtcat[0].measure[Nmeas].t_msec = 0;
      }

      Nmeas ++;

      // update the next_meas pointer for this entry (last one for this star is -1)
      next_meas[Nmeas-1] = (mSrc < srccat->average[i].Nmeasure - 1) ? Nmeas : -1;
    }
    srccat->found_t[i] = Nave;
    Nave ++;
  }
      
  REALLOCATE (tgtcat[0].average, Average, Nave);
  REALLOCATE (tgtcat[0].measure, Measure, Nmeas);

  // XXX allow for unsorted output?
  tgtcat[0].measure = sort_measure (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas, next_meas);
  tgtcat[0].sorted = TRUE;

  /* note stars which have been found in this tgtcat */
  for (i = 0; i < srccat->Naverage; i++) {
    if (srccat->found_t[i] > -1) {
      srccat->found_t[i] = -2;
    }
  }

  /* check if the tgtcat has changed?  if no change, no need to write */
  tgtcat[0].objID    = objID; // new max value, save on tgtcat close
  tgtcat[0].Naverage = Nave;
  tgtcat[0].Nmeasure = Nmeas;
  tgtcat[0].Nsecfilt_mem = Nave*Nsecfilt;
  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT", ("OFF_T_FMT" matches)\n",  Nstars,  Nave,  Nmeas,  Nmatch);

  free (X1);
  free (Y1);
  free (N1);
  free (X2);
  free (Y2);
  free (N2);
  free (next_meas);

  return (Nmatch);
}

/* differences with find_matches_closest():
 * accepts Stars **stars (vs Stars *stars)
 * NREFSTAR_GROUP vs NSTAR_GROUP
 * does not update lensing fields
 * allows option to replace existing entries
 * allows multiple photcodes

 */

