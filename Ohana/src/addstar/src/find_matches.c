# include "addstar.h"

int find_matches (SkyRegion *region, Catalog *srccat, Catalog *tgtcat, AddstarClientOptions options) {

  off_t i, j, J, Nstars, mSrc;
  double RADIUS, RADIUS2;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2;
  off_t Nave, NAVE, Nmeas, NMEAS, Nmatch;
  unsigned int objID, catID;
  Coords tcoords;
  int Nsecfilt, Nsec;

  /* photcode data - must by of type DEP; options.photcode is equiv photcode for all input
     images this function requires incoming stars to have the same photcode.equiv value.  if
     this value is not a valid photcode (ie, 0), then no modification is made to the average 
     magnitudes (Nsec will be -1) */

  Nsecfilt = GetPhotcodeNsecfilt ();
  Nsec     = GetPhotcodeNsec (options.photcode);

  /** allocate local arrays (stars) **/
  ALLOCATE (X1, double, srccat->Naverage);
  ALLOCATE (Y1, double, srccat->Naverage);
  ALLOCATE (N1, off_t,  srccat->Naverage);

  myAbort ("figure out if found_t needs to be tracking Naverage or Nmeasure"); 
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
   * substantially far from the projection pole.  We use the center of the region (tgtcat)
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
    int status = RD_to_XY (&X1[Nstars], &Y1[Nstars], srccat->average[i].R, srccat->average[i].D, &tcoords);
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

  /* set up pointers for linked list of measurements */
  if (tgtcat[0].sorted && (tgtcat[0].Nmeasure == tgtcat[0].Nmeasure_disk)) {
    // this version is only valid if we have done a full catalog load, and if the catalog
    // is sorted while processed
    next_meas = init_measure_links (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas);
  } else {
    next_meas = build_measure_links (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas);
  }    

  /* choose a radius for matches (defined in args.c or ImageOptions.c) */
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

    /* within match range; look for matches */
    for (J = j; (dX > -1.02*RADIUS) && (J < Nave); J++) {
      dX = X1[i] - X2[J];
      dY = Y1[i] - Y2[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;

      /*** a match is found, add to average, measure ***/
      Nmatch ++;
      off_t n = N2[J];
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
	/* add to end of measurement list */
	add_meas_link (&tgtcat[0].average[n], next_meas, Nmeas, NMEAS);
	
	/** add measurements for this star **/

	// set the new measurements
	tgtcat[0].measure[Nmeas]          = srccat->measure[mSrc + mOff];
	
	// the following measure elements cannot be set until here:
	tgtcat[0].measure[Nmeas].dbFlags  = 0;
	tgtcat[0].measure[Nmeas].averef   = n; // this must be an absolute sequence number, if partial average is loaded 
	tgtcat[0].measure[Nmeas].objID    = tgtcat[0].average[n].objID;
	tgtcat[0].measure[Nmeas].catID    = tgtcat[0].catID;

	float dRoff = dvoOffsetR(&tgtcat[0].measure[Nmeas], &tgtcat[0].average[n]);

	// rationalize R:
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
		     X1[i], X2[J], 
		     Y1[i], Y2[J]);
	  }
	}

	/* adds the measurement to the calibration if appropriate color terms are found */
	/* we call this before (optionally) setting the average magnitude to avoid auto-correlations */
	if (options.calibrate) {
	  AddToCalibration (&tgtcat[0].average[n], &tgtcat[0].secfilt[n*Nsecfilt], tgtcat[0].measure, &tgtcat[0].measure[Nmeas], next_meas, N);
	}

	/* set the average magnitude if not already set and if photcode.equiv is not 0 */
	/* in UPDATE mode, this value is not saved; use relphot to recalculate */
	if (Nsec > -1) { 
	  if (isnan(tgtcat[0].secfilt[n*Nsecfilt+Nsec].MpsfChp)) {
	    tgtcat[0].secfilt[n*Nsecfilt+Nsec].MpsfChp = PhotCat (&tgtcat[0].measure[Nmeas], MAG_CLASS_PSF);
	  }
	}

	myAbort ("fix logic on multiple detections");

	/*** flag multiple stars */
	/* this image star matches more than one tgtcat star */
	if (srccat->found_t[N] > -1) {
	  tgtcat[0].measure[srccat->found_t[N]].dbFlags |= ID_MEAS_BLEND_MEAS;
	  tgtcat[0].measure[Nmeas].dbFlags |= ID_MEAS_BLEND_MEAS;
	} 
	if (srccat->found_t[N] == -2) { /* this image star matches a tgtcat star on a neighboring tgtcat */
	  tgtcat[0].measure[Nmeas].dbFlags |= ID_MEAS_BLEND_MEAS_X;
	} 
	if (srccat->found_t[N] == -1) { /* this image star matches only this tgtcat star */
	  srccat->found_t[N] = Nmeas;  /* save first match, in case coincidences are found */
	}
	/* this tgtcat star matches more than one image star */
	if (tgtcat[0].found_t[n] > -1) {
	  tgtcat[0].measure[tgtcat[0].found_t[n]].dbFlags |= ID_MEAS_BLEND_OBJ;
	  tgtcat[0].measure[Nmeas].dbFlags |= ID_MEAS_BLEND_OBJ;
	} else {
	  tgtcat[0].found_t[n] = Nmeas;
	}
	tgtcat[0].average[n].Nmeasure ++;
	Nmeas ++;
      }

      if (!options.update) {
	/* in UPDATE mode, newly calculated coordinates are not saved */
	update_coords (&tgtcat[0].average[n], &tgtcat[0].measure[0], next_meas);
      }
    }
    i++;
  }

  /* incorporate unmatched image stars, if this star is in field of this tgtcat */
  /* these new entries are all written out in UPDATE mode */ 
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
      if (tgtcat[0].secfilt) {
	// we only update the secfilt table if it has been allocated for output
	REALLOCATE (tgtcat[0].secfilt, SecFilt, NAVE*tgtcat[0].Nsecfilt);
      }
    }

    if (srccat->found_t[i] >= 0) continue;
    if (!IN_REGION (srccat->average[i].R, srccat->average[i].D)) continue;

    dvo_average_init (&tgtcat[0].average[Nave]);
    tgtcat[0].average[Nave].R         	   = srccat->average[i].R;
    tgtcat[0].average[Nave].D         	   = srccat->average[i].D;

    tgtcat[0].average[Nave].Nmeasure       = srccat->average[i].Nmeasure;
    tgtcat[0].average[Nave].measureOffset  = Nmeas;
    tgtcat[0].average[Nave].objID     	   = objID;
    tgtcat[0].average[Nave].catID     	   = catID;

    if (PSPS_ID) {
      tgtcat[0].average[Nave].extID = CreatePSPSObjectID(tgtcat[0].average[Nave].R, tgtcat[0].average[Nave].D);
    }

    objID ++;

    for (j = 0; tgtcat[0].secfilt && (j < Nsecfilt); j++) {
      dvo_secfilt_init (&tgtcat[0].secfilt[Nave*Nsecfilt+j], SECFILT_RESET_ALL);
    }

    off_t mOff = srccat->average[i].measureOffset;
    for (mSrc = 0; mSrc < srccat->average[i].Nmeasure; mSrc++) {
      // supply the measurments from this detection
      dvo_measure_init (&tgtcat[0].measure[Nmeas]);
      tgtcat[0].measure[Nmeas] = srccat->measure[mSrc + mOff];

      // the following measure elements cannot be set until here:
      tgtcat[0].measure[Nmeas].dbFlags   = 0;
      tgtcat[0].measure[Nmeas].averef    = Nave; // XXX EAM : must be absolute Nave if partial read
      tgtcat[0].measure[Nmeas].objID     = tgtcat[0].average[Nave].objID;
      tgtcat[0].measure[Nmeas].catID     = tgtcat[0].catID;

      /* set the average magnitude if not already set and the photcode.equiv is not 0 */
      /* in UPDATE mode, this value is not saved; use relphot to recalculate */
      if (Nsec > -1) { 
	tgtcat[0].secfilt[Nave*Nsecfilt+Nsec].MpsfChp = PhotCat (&tgtcat[0].measure[Nmeas], MAG_CLASS_PSF);
      }

      /* next[Nmeas] should always be -1 in this context (it is always the only
	 measurement for the star) */
      srccat->found_t[i] = Nmeas;
      Nmeas ++;

      // update the next_meas pointer for this entry (last one for this star is -1)
      next_meas[Nmeas-1] = (mSrc < srccat->average[i].Nmeasure - 1) ? Nmeas : -1;
    }
    Nave ++;
  }
      
  REALLOCATE (tgtcat[0].average, Average, Nave);
  REALLOCATE (tgtcat[0].measure, Measure, Nmeas);
 
  if (options.nosort) {
    tgtcat[0].sorted = FALSE;
  } else {
    tgtcat[0].sorted = TRUE;
    tgtcat[0].measure = sort_measure (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas, next_meas);
  }

  /* note stars which have been found in this tgtcat */
  for (i = 0; i < srccat->Nmeasure; i++) {
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

/* 
   notes:
   
   for finding if a tgtcat star is in an image or an image star is in the tgtcat:
   
   tgtcats have boundaries defined by RA and DEC, but they may curve in projection
   images have boundaries which are lines in pixels coords, but curve in RA and DEC
   
   tgtcat[0].found[Ncat] but stars[Nstar].found
   
*/
