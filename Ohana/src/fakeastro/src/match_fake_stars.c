# include "fakeastro.h"

int match_fake_stars (Stars *stars, unsigned int NstarsIn, SkyRegion *region, Catalog *catalog) {
  
  off_t i, j, n, N, J, Jmin, status, Nstars;
  double RADIUS2, Rmin;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2;
  off_t Nave, NAVE, Nmeas, NMEAS, Nstarpar, NSTARPAR, Nmatch;
  unsigned int objID, catID;
  Coords tcoords;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  /** allocate local arrays (stars) **/
  ALLOCATE (X1, double, NstarsIn);
  ALLOCATE (Y1, double, NstarsIn);
  ALLOCATE (N1, off_t,  NstarsIn);

  /** allocate local arrays (catalog) **/
  NAVE = Nave = catalog[0].Naverage;
  ALLOCATE (X2, double, NAVE);
  ALLOCATE (Y2, double, NAVE);
  ALLOCATE (N2, off_t,  NAVE);
  ALLOCATE (catalog[0].found_t, off_t, NAVE);
  /* for secfilt j and star i, secfilt[i*Nsecfilt+j] */

  /* internal counters */
  Nmatch = 0;
  NMEAS = Nmeas = catalog[0].Nmeasure;
  NSTARPAR = Nstarpar = catalog[0].Nstarpar;

  // current max obj ID for this catalog
  objID = catalog[0].objID;
  catID = catalog[0].catID;

  /* project onto rectilinear grid with 1 arcsec pixels. the choice of ARC projection has
   * the advantage that every point in R,D has a mapping to a unique X,Y.  However, note
   * that not all possible X,Y points map back to R,D and the local plate scale changes
   * far from the projection pole. We use the center of the region (catalog) for crval1,2.
   */
  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  if (region[0].Dmax < 90) {
    tcoords.crval2 = 0.5*(region[0].Dmin + region[0].Dmax);
  } else {
    tcoords.crval2 = 90.0;
  }
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  /* build spatial index (RA sort) referencing input array sequence */
  Nstars = 0;
  for (i = 0; i < NstarsIn; i++) {
    status = RD_to_XY (&X1[Nstars], &Y1[Nstars], stars[i].average.R, stars[i].average.D, &tcoords);
    if (!status) continue;
    N1[Nstars] = i;
    Nstars ++;
  }
  if (Nstars < 1) {
    if (VERBOSE) fprintf (stderr, "skipping %s, no overlapping stars\n", catalog[0].filename);
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
    RD_to_XY (&X2[i], &Y2[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N2[i] = i;
    catalog[0].found_t[N2[i]] = -1;
  }
  if (Nave > 1) sort_coords_index (X2, Y2, N2, Nave);

  /* RADIUS is global from ConfigInit.c */
  RADIUS2 = RADIUS*RADIUS;

  /** find matched stars **/
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

    if (stars[N1[i]].found != -1) {
        /* this star has already been assigned to an object in this or another catalog */
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
    n = N2[Jmin];
    N = N1[i];

    /* make sure there is space for next entry */
    if (Nmeas >= NMEAS) {
      NMEAS = Nmeas + 1000;
      REALLOCATE (catalog[0].measure, Measure, NMEAS);
    }

    /** add measurements for this star **/

    // set the new measurements
    catalog[0].measure[Nmeas]          = stars[N].measure;

    // measure now carries R,D (not dR,dD) 
    // note that ReadStarsFITS does not set measure.R,D and average.R,D
    catalog[0].measure[Nmeas].R        = stars[N].average.R;
    catalog[0].measure[Nmeas].D        = stars[N].average.D;
    catalog[0].measure[Nmeas].dbFlags  = 0;
    catalog[0].measure[Nmeas].averef   = n;
    catalog[0].measure[Nmeas].objID    = catalog[0].average[n].objID;
    catalog[0].measure[Nmeas].catID    = catalog[0].catID;

    float dRoff = dvoOffsetR(&catalog[0].measure[Nmeas], &catalog[0].average[n]);

    // rationalize dR
    if (dRoff > +180.0*3600.0) {
      // average on high end of boundary, move star up
      catalog[0].measure[Nmeas].R += 360.0;
      dRoff -= 360.0*3600.0;
    }
    if (dRoff < -180.0*3600.0) {
      // average on low end of boundary, move star down
      catalog[0].measure[Nmeas].R -= 360.0;
      dRoff += 360.0*3600.0;
    }
    if (fabs(dRoff) > 10*RADIUS) {
	// take declination into account and check again.
	double cosD = cos(RAD_DEG*catalog[0].average[n].D);
	if (fabs(dRoff*cosD) > 10*RADIUS) {
	    fprintf (stderr, "error: %10.6f,%10.6f vs %10.6f,%10.6f (%f,%f vs %f,%f)\n", 
		     catalog[0].average[n].R, catalog[0].average[n].D, 
		     stars[N].average.R, stars[N].average.D,
		     X1[i], X2[Jmin], 
		     Y1[i], Y2[Jmin]);
	    // XXX abort on this? -- this is a bad failure...
	}
    }

    /* Nm is updated, but not written out in -update mode (for existing entries)
       Nm is recalculated in build_meas_links if loaded table is not sorted */
    stars[N].found = Nmeas;
    catalog[0].found_t[n] = Nmeas;
    catalog[0].average[n].Nmeasure ++;
    Nmeas ++;
    i++;
  }

  /** incorporate unmatched image stars, if this star is in field of this catalog **/
  /* these new entries are all written out in UPDATE mode */ 
  for (i = 0; i < Nstars; i ++) {
    if (stars[i].found != -1) continue;
    if (!IN_REGION (stars[i].average.R, stars[i].average.D)) continue;

    /* make sure there is space for next entry */
    if (Nmeas >= NMEAS - 1) {
      NMEAS = Nmeas + 1000;
      REALLOCATE (catalog[0].measure, Measure, NMEAS);
    }
    /* make sure there is space for next entry */
    if (Nstarpar >= NSTARPAR - 1) {
      NSTARPAR = Nstarpar + 1000;
      REALLOCATE (catalog[0].starpar, StarPar, NSTARPAR);
    }
    if (Nave >= NAVE) {
      NAVE = Nave + 1000;
      REALLOCATE (catalog[0].average, Average, NAVE);
      REALLOCATE (catalog[0].secfilt, SecFilt, NAVE*catalog[0].Nsecfilt);
    }

    dvo_average_init (&catalog[0].average[Nave]);
    catalog[0].average[Nave].R         	   = stars[i].average.R;
    catalog[0].average[Nave].D         	   = stars[i].average.D;

    catalog[0].average[Nave].Nstarpar  	   = 1;
    catalog[0].average[Nave].starparOffset = Nstarpar;
    catalog[0].average[Nave].Nmeasure  	   = 1;
    catalog[0].average[Nave].measureOffset = Nmeas;
    catalog[0].average[Nave].objID     	   = objID;
    catalog[0].average[Nave].catID     	   = catID;

    objID ++;

    for (j = 0; j < Nsecfilt; j++) {
      dvo_secfilt_init (&catalog[0].secfilt[Nave*Nsecfilt+j], SECFILT_RESET_ALL);
    }

    // supply the measurments from this detection
    dvo_measure_init (&catalog[0].measure[Nmeas]);
    catalog[0].measure[Nmeas]           = stars[i].measure;

    // the following measure elements cannot be set until here:
    catalog[0].measure[Nmeas].R        = stars[i].average.R;
    catalog[0].measure[Nmeas].D        = stars[i].average.D;
    catalog[0].measure[Nmeas].dbFlags  = 0;
    catalog[0].measure[Nmeas].averef   = Nave;
    catalog[0].measure[Nmeas].objID    = catalog[0].average[Nave].objID;
    catalog[0].measure[Nmeas].catID    = catalog[0].catID;

    // supply the starpar values from this detection
    dvo_starpar_init (&catalog[0].starpar[Nstarpar]);
    catalog[0].starpar[Nstarpar] = stars[i].starpar;
    catalog[0].starpar[Nstarpar].averef   = Nave;
    catalog[0].starpar[Nstarpar].objID    = catalog[0].average[Nave].objID;
    catalog[0].starpar[Nstarpar].catID    = catalog[0].catID;
    Nstarpar ++;

    /* next[Nmeas] should always be -1 in this context (it is always the only
       measurement for the star) */
    stars[i].found = Nmeas;
    Nmeas ++;
    Nave ++;
  }
      
  REALLOCATE (catalog[0].average, Average, Nave);
  REALLOCATE (catalog[0].measure, Measure, Nmeas);
  REALLOCATE (catalog[0].starpar, StarPar, Nstarpar);
 
  catalog[0].sorted = FALSE;

  /* note stars which have been found in this catalog */
  for (i = 0; i < NstarsIn; i++) {
    if (stars[i].found > -1) {
      stars[i].found = -2;
    }
  }

  /* check if the catalog has changed?  if no change, no need to write */
  catalog[0].objID    = objID; // new max value, save on catalog close
  catalog[0].Naverage = Nave;
  catalog[0].Nmeasure = Nmeas;
  catalog[0].Nstarpar = Nstarpar;
  catalog[0].Nsecfilt_mem = Nave*Nsecfilt;
  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nmeas: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT", ("OFF_T_FMT" matches)\n",  Nstars,  Nave,  Nmeas,  Nmatch);

  free (X1);
  free (Y1);
  free (N1);
  free (N2);
  free (X2);
  free (Y2);

  return (Nmatch);
}
