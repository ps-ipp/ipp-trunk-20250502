# include "addstar.h"
# include "setobjflags.h"

// we find matching stars by (R,D).  we add the supplied bit to the average.flags for that star
int find_matches_setobjflags (SkyRegion *region, MyStars *stars, int Nstars, Catalog *catalog) {
  
  off_t i, j, n, N, J, Jmin;
  double RADIUS, RADIUS2, Rmin;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2;
  off_t Nave, NAVE;

  /** allocate local arrays (stars) **/
  ALLOCATE (X1, double, Nstars);
  ALLOCATE (Y1, double, Nstars);
  ALLOCATE (N1, off_t,  Nstars);

  /** allocate local arrays (catalog) **/
  NAVE = Nave = catalog[0].Naverage;
  ALLOCATE (X2, double, NAVE);
  ALLOCATE (Y2, double, NAVE);
  ALLOCATE (N2, off_t,  NAVE);

  // use this structure to count the number of mops detections
  ALLOCATE_ZERO (catalog[0].nOwn_t, int,  NAVE);

  /* internal counters */
  off_t Nmatch = 0;
  off_t Nmissed = 0;

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
  RADIUS = SRC_RADIUS; /* provided by config */
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
       to that same object.  In this case (setobjflags), this is actually the behavior I
       want: I have multiple independent star lists which need to be assigned to the
       database.
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

    /*** a match is found ***/
    Nmatch ++;
    n = N2[Jmin];
    N = N1[i];

    /** set bit for this star **/
    catalog[0].average[n].flags |= stars[N].myBit;
    if (stars[N].myBit & ID_OBJ_HAS_SOLSYS_DET) {
      catalog[0].nOwn_t[n] ++;
      if (catalog[0].nOwn_t[n] >= 0.5*catalog[0].average[n].Nmeasure) {
	catalog[0].average[n].flags |= ID_OBJ_MOST_SOLSYS_DET;
      }
    }
    stars[N].found = TRUE;
    i++;
  }

  /** count unmatched sources **/
  for (i = 0; i < Nstars; i++) {
    // skip already matched stars
    if (stars[i].found) continue;
    Nmissed ++;
  }

  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nmatched, Nmissed: %d "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT"\n",  Nstars, Nave, Nmatch, Nmissed);

  free (X1);
  free (Y1);
  free (N1);
  free (N2);
  free (X2);
  free (Y2);

  return (Nmatch);
}
