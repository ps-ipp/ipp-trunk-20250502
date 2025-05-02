# include "addstar.h"
# include "gaia.h"

int find_matches_gaia (SkyRegion *region, Gaia_Stars *stars, int NstarsIn, Catalog *tgtcat, AddstarClientOptions *options) {

  int Nsecfilt = GetPhotcodeNsecfilt ();
  int Nsec     = GetPhotcodeNsec (options->photcode);

  /** allocate local arrays (stars) **/
  ALLOCATE_PTR (X1, double, NstarsIn);
  ALLOCATE_PTR (Y1, double, NstarsIn);
  ALLOCATE_PTR (N1, off_t,  NstarsIn);

  /** allocate local arrays (tgtcat) **/
  off_t NAVE = tgtcat[0].Naverage;
  off_t Nave = tgtcat[0].Naverage;
  ALLOCATE_PTR (X2, double, NAVE);
  ALLOCATE_PTR (Y2, double, NAVE);
  ALLOCATE_PTR (N2, off_t,  NAVE);

  /* for secfilt j and star i, secfilt[i*Nsecfilt+j] */

  /* internal counters */
  off_t Nmatch = 0;
  off_t NMEAS = tgtcat[0].Nmeasure;
  off_t Nmeas = tgtcat[0].Nmeasure;

  off_t *next_meas = NULL;

  // current max obj ID for this tgtcat
  unsigned int objID = tgtcat[0].objID;
  unsigned int catID = tgtcat[0].catID;

  /* project onto rectilinear grid with 1 arcsec pixels. the choice of ARC projection has
   * the advantage that every point in R,D has a mapping to a unique X,Y.  However, note
   * that not all possible X,Y points map back to R,D and the local plate scale changes
   * substantially far from the projection pole. We use the center of the region (tgtcat)
   * for crval1,2.
   */

  Coords tcoords;
  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  if ((region[0].Dmax < 90) && (region[0].Dmin > -90)) {
    tcoords.crval2 = 0.5*(region[0].Dmin + region[0].Dmax);
  } else {
    tcoords.crval2 = (region[0].Dmax >= 90) ? 90.0 : -90.0;
  }
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  /* build spatial index (RA sort) referencing input array sequence */
  off_t Nstars = 0;
  for (off_t i = 0; i < NstarsIn; i++) {
    int status = RD_to_XY (&X1[Nstars], &Y1[Nstars], stars[i].R, stars[i].D, &tcoords);
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
    return 0;
  }
  if (Nstars > 1) sort_coords_index (X1, Y1, N1, Nstars);

  /* build spatial index (RA sort) */
  for (off_t i = 0; i < Nave; i++) {
    RD_to_XY (&X2[i], &Y2[i], tgtcat[0].average[i].R, tgtcat[0].average[i].D, &tcoords);
    N2[i] = i;
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
  double RADIUS = (options->radius == 0) ? 2.0 : options->radius; /* provided by config */
  double RADIUS2 = RADIUS*RADIUS;

  /****************** find matched stars ********************/

  for (off_t i = 0, j = 0; (i < Nstars) && (j < Nave); ) {
    if (!finite(X1[i]) || !finite(Y1[i])) { 
      i++; 
      continue;
    }
    if (!finite(X2[j]) || !finite(Y2[j])) { 
      j++; 
      continue;
    }
    
    /* negative dX: j is too large */
    double dX = X1[i] - X2[j];
    if (dX <= -1.02*RADIUS) {
      i++;
      continue;
    }
    /* positive dX, i is too large */
    if (dX >= 1.02*RADIUS) {
      j++;
      continue;
    }

    // skip this star if already assigned to an object
    if (stars[N1[i]].found) {
        i++;
        continue;
    }

    /* this block will match a given detection to the closest object within range of that detection.
       XXX note that this matches ALL detections within range of the single object to that same object 
       this is bad, but I cannot just go in linear order (ie, mark off each object as they are
       used).  I should make a list of all Nobj * Ndet pairs in range and choose the matches
       based on their separations.  UGH
     */
    
    /* within match range; look for matches */
    off_t Jmin = -1;
    double Rmin = RADIUS2;
    for (off_t J = j; (dX > -1.02*RADIUS) && (J < Nave); J++) {
      /* find closest match for this detection */
      dX = X1[i] - X2[J];
      double dY = Y1[i] - Y2[J];
      double dR = dX*dX + dY*dY;
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
    if (Nmeas >= NMEAS - 1) {
      NMEAS = Nmeas + 1000;
      REALLOCATE (next_meas, off_t, NMEAS);
      REALLOCATE (tgtcat[0].measure, Measure, NMEAS);
    }

    /** add measurement for this star **/

    /* add to end of measurement list */
    add_meas_link (&tgtcat[0].average[n], next_meas, Nmeas, NMEAS);

    // set the new measurements
    tgtcat[0].measure[Nmeas]          = stars[N].measure;

    // measure now carries R,D (not dR,dD) 
    tgtcat[0].measure[Nmeas].dbFlags  = 0;
    tgtcat[0].measure[Nmeas].averef   = n;
    tgtcat[0].measure[Nmeas].objID    = tgtcat[0].average[n].objID;
    tgtcat[0].measure[Nmeas].catID    = tgtcat[0].catID;
    
    float dRoff = dvoOffsetR(&tgtcat[0].measure[Nmeas], &tgtcat[0].average[n]);

    // rationalize dR
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
		 stars[N].R, stars[N].D,
		 X1[i], X2[Jmin], 
		 Y1[i], Y2[Jmin]);
	// XXX abort on this? -- this is a bad failure...
      }
    }

    /* Nm is updated, but not written out in -update mode (for existing entries)
       Nm is recalculated in build_meas_links if loaded table is not sorted */
    tgtcat[0].average[n].Nmeasure ++;
    Nmeas ++;

    /* if we choose to flag close encounters, see find_matches.c */
    /* if we choose to calculate RA,DEC averages, see update_coords.c */

    stars[N].found = TRUE;
    i++;
  }

  /*************** add unmatched stars *************************/

  /** incorporate unmatched image stars, if this star is in field of this tgtcat **/
  /* these new entries are all written out in UPDATE mode */ 
  for (off_t i = 0; (i < Nstars) && !options->only_match; i++) {
    /* make sure there is space for next entry */
    if (Nmeas >= NMEAS - 1) {
      NMEAS = Nmeas + 1000;
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

    if (stars[i].found) continue;
    if (!IN_REGION (stars[i].R, stars[i].D)) continue;

    dvo_average_init (&tgtcat[0].average[Nave]);
    tgtcat[0].average[Nave].R         	   = stars[i].R;
    tgtcat[0].average[Nave].D         	   = stars[i].D;

    tgtcat[0].average[Nave].Nmeasure  	   = 1;
    tgtcat[0].average[Nave].measureOffset  = Nmeas;
    tgtcat[0].average[Nave].objID     	   = objID;
    tgtcat[0].average[Nave].catID     	   = catID;

    if (PSPS_ID) {
        tgtcat[0].average[Nave].extID = CreatePSPSObjectID(tgtcat[0].average[Nave].R, tgtcat[0].average[Nave].D);
    }

    objID ++;

    // we only update the secfilt table if it has been allocated for output
    for (int j = 0; tgtcat[0].secfilt && (j < Nsecfilt); j++) {
      dvo_secfilt_init (&tgtcat[0].secfilt[Nave*Nsecfilt+j], SECFILT_RESET_ALL);
    }

    // supply the measurements from this detection
    dvo_measure_init (&tgtcat[0].measure[Nmeas]);
    tgtcat[0].measure[Nmeas] = stars[i].measure;

    // the following measure elements cannot be set until here:
    tgtcat[0].measure[Nmeas].dbFlags  = 0;
    tgtcat[0].measure[Nmeas].averef   = Nave;
    tgtcat[0].measure[Nmeas].objID    = tgtcat[0].average[Nave].objID;
    tgtcat[0].measure[Nmeas].catID    = tgtcat[0].catID;

    /* set the average magnitude if not already set and the photcode.equiv is not 0 */
    /* in UPDATE mode, this value is not saved; use relphot to recalculate */
    if (Nsec > -1) { 
      tgtcat[0].secfilt[Nave*Nsecfilt+Nsec].MpsfChp = PhotCat (&tgtcat[0].measure[Nmeas], MAG_CLASS_PSF);
    }

    Nmeas ++;

    // update the next_meas pointer for this entry (last one for this star is -1)
    next_meas[Nmeas-1] = -1;
    stars[i].found = TRUE;
    Nave ++;
  }

  REALLOCATE (tgtcat[0].average, Average, Nave);
  REALLOCATE (tgtcat[0].measure, Measure, Nmeas);
 
  if (options->nosort) {
    tgtcat[0].sorted = FALSE;
  } else {
    tgtcat[0].sorted = TRUE;
    tgtcat[0].measure = sort_measure (tgtcat[0].average, Nave, tgtcat[0].measure, Nmeas, next_meas);
  }

  /* check if the tgtcat has changed?  if no change, no need to write */
  tgtcat[0].objID    = objID; // new max value, save on tgtcat close
  tgtcat[0].Naverage = Nave;
  tgtcat[0].Nmeasure = Nmeas;
  tgtcat[0].Nsecfilt_mem = tgtcat[0].secfilt ? Nave*Nsecfilt : 0;
  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT", ("OFF_T_FMT" matches)\n",  Nstars,  Nave,  Nmeas, Nmatch);

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
