# include "addstar.h"
# include "loadICRF.h"

// we find matching stars by (R,D).  we add the ICRF to the array of ICRFs with appropriate links.  
int find_matches_ICRF (SkyRegion *region, ICRF_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options) {
  
  off_t i, j, n, N, J, Jmin;
  double RADIUS, RADIUS2, Rmin;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2;
  off_t Nave, NAVE, Nmeas, NMEAS, Nmatch;

  /** allocate local arrays (stars) **/
  ALLOCATE (X1, double, Nstars);
  ALLOCATE (Y1, double, Nstars);
  ALLOCATE (N1, off_t,  Nstars);

  /** allocate local arrays (catalog) **/
  NAVE = Nave = catalog[0].Naverage;
  ALLOCATE (X2, double, NAVE);
  ALLOCATE (Y2, double, NAVE);
  ALLOCATE (N2, off_t,  NAVE);

  /* internal counters */
  Nmatch = 0;

  // current max obj ID for this catalog
  unsigned int catID = catalog[0].catID;

  int Nsecfilt = catalog[0].Nsecfilt;

  NMEAS = Nmeas = catalog[0].Nmeasure;

  /* project onto rectilinear grid with 1 arcsec pixels. the choice of ARC projection has
   * the advantage that every point in R,D has a mapping to a unique X,Y.  However, note
   * that not all possible X,Y points map back to R,D and the local plate scale changes
   * far from the projection pole. We use the center of the region (catalog) for crval1,2.
   */
  Coords tcoords;
  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  tcoords.crval2 = (region[0].Dmax < 90) ? 0.5*(region[0].Dmin + region[0].Dmax) : 90.0;
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  /* build spatial index (RA sort) referencing input array sequence */
  for (i = 0; i < Nstars; i++) {
    RD_to_XY (&X1[i], &Y1[i], stars[i].R, stars[i].D, &tcoords);
    N1[i] = i;
  }
  sort_coords_index (X1, Y1, N1, Nstars);

  /* build spatial index (RA sort) */
  for (i = 0; i < Nave; i++) {
    RD_to_XY (&X2[i], &Y2[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N2[i] = i;
  }
  sort_coords_index (X2, Y2, N2, Nave);

  /* choose a radius for matches */
  RADIUS = options->radius; /* provided by config */
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

    if (stars[N1[i]].found) {
      /* this star has already been assigned to an object in this or another catalog */
      i++;
      continue;
    }

    /* this block will match a given detection to the closest object within range of that
       detection.  Note that this matches ALL detections within range of the single object
       to that same object.  In the case of ICRF, this is what I want because I am
       attaching each radio band as a separate measurement, with a single concatenated
       list.  This is not super efficient, but ICRF is a small list
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

    /*** a match is found, add to measure with links to average ***/
    Nmatch ++;
    n = N2[Jmin];
    N = N1[i];

    /* make sure there is space for next entry */
    if (Nmeas >= NMEAS) {
      NMEAS = Nmeas + 1000;
      REALLOCATE (catalog[0].measure, Measure, NMEAS);
    }

    /** add new measurement for this star **/

    // set the new measurements
    catalog[0].measure[Nmeas]          = stars[N].measure;

    // measure now carries R,D (not dR,dD) 
    catalog[0].measure[Nmeas].averef   = n;
    catalog[0].measure[Nmeas].objID    = catalog[0].average[n].objID;
    catalog[0].measure[Nmeas].catID    = catID;

    catalog[0].average[n].flags |= ID_OBJ_ICRF_QSO;

    stars[N].found = TRUE;
    catalog[0].average[n].Nmeasure ++;
    Nmeas ++;
    i++;
  }

  // list missed QSOs, do not insert
  for (i = 0; i < Nstars; i++) {
    if (stars[i].found) continue;
    if (!IN_REGION (stars[i].R, stars[i].D)) continue;
    fprintf (stderr, "missed ICRF QSO @ %10.6f %10.6f\n", stars[i].R, stars[i].D);
  }

  REALLOCATE (catalog[0].average, Average, Nave);
  REALLOCATE (catalog[0].measure, Measure, Nmeas);
 
  catalog[0].sorted = FALSE;

  /* check if the catalog has changed?  if no change, no need to write */
  // catalog[0].objID     = objID; -- I am not adding new entries, so objID does not change
  catalog[0].Naverage  = Nave;
  catalog[0].Nmeasure  = Nmeas;
  catalog[0].Nsecfilt_mem = Nave*Nsecfilt;
  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nmeas: %d "OFF_T_FMT" "OFF_T_FMT" ("OFF_T_FMT" matches)\n",  Nstars, Nave, Nmeas, Nmatch);

  free (X1);
  free (Y1);
  free (N1);
  free (N2);
  free (X2);
  free (Y2);

  return (Nmatch);
}
