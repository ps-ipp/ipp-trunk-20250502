# include "dvoshell.h"

// SkyRegion *region, Stars *stars, unsigned int NstarsIn, Catalog *catalog, AddstarClientOptions options)

// attempt to match every RA,DEC entry with an entry in the catalog.  the result is stored in 'index'
// index >= 0 : valid match; index == -2 : no valid match; index == -1 : not contained by this catalog
int find_matches_by_vectors_closest (SkyRegion *region, Catalog *catalog, Vector *RAvec, Vector *DECvec, float RADIUS, off_t *index) {

  off_t i, j, J, Jmin, Npoints, Nmatch;
  off_t *N1, *N2;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR, Doff, Dmin, Dmax, Rmin, Rmax;
  int status;
  double RADIUS2, RadMin;
  off_t Nave;
  Coords tcoords;

  // XXX dvoconvert does not correctly maintain 'sorted' 
  // assert(catalog[0].sorted);

  Npoints = RAvec->Nelements;
  Nmatch = 0;

  /** allocate local arrays (points) **/
  ALLOCATE (X1, double, Npoints);
  ALLOCATE (Y1, double, Npoints);
  ALLOCATE (N1, off_t,  Npoints);

  /** allocate local arrays (catalog) **/
  Nave = catalog[0].Naverage;
  ALLOCATE (X2, double, Nave);
  ALLOCATE (Y2, double, Nave);
  ALLOCATE (N2, off_t,  Nave);

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

  // this region includes a boundary layer of size RADIUS
  if (fabs(region[0].Dmin) < fabs(region[0].Dmax)) {
    Doff = RAD_DEG*region[0].Dmax;
  } else {
    Doff = RAD_DEG*region[0].Dmin;
  }    
  if (Doff < 80) {
    Rmin = region[0].Rmin - RADIUS / 3600.0 / cos(Doff);
    Rmax = region[0].Rmax + RADIUS / 3600.0 / cos(Doff);
  } else {
    Rmin = 0.0;
    Rmax = 360.0;
  }
  Dmin = region[0].Dmin - RADIUS / 3600.0;
  Dmax = region[0].Dmax + RADIUS / 3600.0;

  // identify the entries contained by this catalog & init index
  for (i = 0; i < Npoints; i++) {
    index[i] = -1;
    if (RAvec->elements.Flt[i] < Rmin) continue;
    if (RAvec->elements.Flt[i] > Rmax) continue;
    if (DECvec->elements.Flt[i] < Dmin) continue;
    if (DECvec->elements.Flt[i] > Dmax) continue;
    index[i] = -2;
  }

  /* build spatial index (RA sort) referencing input array sequence */
  for (i = 0; i < Npoints; i++) {
    // we need to have a finite number for the sort below; index == -1 entries are skipped
    // in the matching process
    X1[i] = Y1[i] = 0.0; 
    N1[i] = i;
    if (index[i] == -1) continue;
    status = RD_to_XY (&X1[i], &Y1[i], RAvec->elements.Flt[i], DECvec->elements.Flt[i], &tcoords);
    assert (status);
  }
  if (Npoints > 1) sort_coords_index (X1, Y1, N1, Npoints);

  /* build spatial index (RA sort) */
  for (i = 0; i < Nave; i++) {
    RD_to_XY (&X2[i], &Y2[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N2[i] = i;
  }
  if (Nave > 1) sort_coords_index (X2, Y2, N2, Nave);

  /* choose a radius for matches */
  RADIUS2 = RADIUS*RADIUS;

# define NEXTi { i++; continue; }
# define NEXTj { j++; continue; }
                                                                                                                                                                                 
  /** find matched stars **/
  for (i = j = 0; (i < Npoints) && (j < Nave); ) {
    if (index[N1[i]] == -1) NEXTi;
    if (!finite(X1[i]) || !finite(Y1[i])) NEXTi;
    if (!finite(X2[j]) || !finite(Y2[j])) NEXTj;
    
    /* negative dX: j is too large */
    dX = X1[i] - X2[j];
    if (dX <= -1.02*RADIUS) NEXTi;

    /* positive dX, i is too large */
    if (dX >= 1.02*RADIUS) NEXTj;

    /* within match range; look for matches */
    Jmin = -1;
    RadMin = RADIUS2;
    for (J = j; (dX > -1.02*RADIUS) && (J < Nave); J++) {
      /* find closest match for this detection */
      dX = X1[i] - X2[J];
      dY = Y1[i] - Y2[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;
      if (dR < RadMin) {
	RadMin = dR;
	Jmin  = J;
      }
    }

    /* no match, try next detection */ 
    if (Jmin == -1) {
      NEXTi;
    }

    /*** a match is found, set the index for this entry ***/
    index[N1[i]] = N2[Jmin];

    Nmatch ++;

    NEXTi;
  }

  free (X1);
  free (Y1);
  free (N1);
  free (N2);
  free (X2);
  free (Y2);

  return (TRUE);
}

// attempt to match every RA,DEC entry with an entry in the catalog.  the result is stored in 'result'
AvselectResult *find_matches_by_vectors_allmatch (SkyRegion *region, Catalog *catalog, Vector *RAvec, Vector *DECvec, float RADIUS, off_t *nresult) {

  off_t i, j, J;
  double dX, dY, dR, Doff, Dmin, Dmax, Rmin, Rmax;
  int status;
  double RADIUS2;
  Coords tcoords;

  // XXX dvoconvert does not correctly maintain 'sorted' 
  // assert(catalog[0].sorted);

  off_t Npoints = RAvec->Nelements;

  /** allocate local arrays (points) **/
  ALLOCATE_PTR (X1, double, Npoints);
  ALLOCATE_PTR (Y1, double, Npoints);
  ALLOCATE_PTR (N1, off_t,  Npoints);

  ALLOCATE_PTR (inCatalog, int,  Npoints);

  /** allocate local arrays (catalog) **/
  off_t Nave = catalog[0].Naverage;
  ALLOCATE_PTR (X2, double, Nave);
  ALLOCATE_PTR (Y2, double, Nave);
  ALLOCATE_PTR (N2, off_t,  Nave);

  off_t Nresult = 0;
  off_t NRESULT = 1000;
  ALLOCATE_PTR (result, AvselectResult, NRESULT);

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

  // this region includes a boundary layer of size RADIUS
  if (fabs(region[0].Dmin) < fabs(region[0].Dmax)) {
    Doff = RAD_DEG*region[0].Dmax;
  } else {
    Doff = RAD_DEG*region[0].Dmin;
  }    
  if (Doff < 80) {
    Rmin = region[0].Rmin - RADIUS / 3600.0 / cos(Doff);
    Rmax = region[0].Rmax + RADIUS / 3600.0 / cos(Doff);
  } else {
    Rmin = 0.0;
    Rmax = 360.0;
  }
  Dmin = region[0].Dmin - RADIUS / 3600.0;
  Dmax = region[0].Dmax + RADIUS / 3600.0;

  // identify the entries contained by this catalog
  for (i = 0; i < Npoints; i++) {
    inCatalog[i] = FALSE;

    // we need to worry about points near the 0,360 boundary.  I have expanded Rmin and
    // Rmax to account for the RADIUS.  if I have a region which is close to the boundary,
    // then: Rmin < 0 or Rmax > 360.  if so, check on the other side as well

    int altTest = FALSE;
    double Rnorm = ohana_normalize_angle (RAvec->elements.Flt[i]);
    if ((Rmax > 360.0) && (Rnorm < 180.0)) {
      double Rtest = Rnorm + 360.0;
      if (Rtest < Rmin) continue;
      if (Rtest > Rmax) continue;
      altTest = TRUE;
    }
    if ((Rmin < 0.0) && (Rnorm > 180.0)) {
      double Rtest = Rnorm - 360.0;
      if (Rtest < Rmin) continue;
      if (Rtest > Rmax) continue;
      altTest = TRUE;
    }
    if (!altTest) {
      if (Rnorm < Rmin) continue;
      if (Rnorm > Rmax) continue;
    }

    if (DECvec->elements.Flt[i] < Dmin) continue;
    if (DECvec->elements.Flt[i] > Dmax) continue;
    inCatalog[i] = TRUE;
  }

  /* build spatial index (RA sort) referencing input array sequence */
  for (i = 0; i < Npoints; i++) {
    // we need to have a finite number for the sort below; inCatalog == 0 entries are skipped
    // in the matching process
    X1[i] = Y1[i] = 0.0; 
    N1[i] = i;
    if (!inCatalog[i]) continue;
    status = RD_to_XY (&X1[i], &Y1[i], RAvec->elements.Flt[i], DECvec->elements.Flt[i], &tcoords);
    assert (status);
  }
  if (Npoints > 1) sort_coords_index (X1, Y1, N1, Npoints);

  /* build spatial index (RA sort) */
  for (i = 0; i < Nave; i++) {
    RD_to_XY (&X2[i], &Y2[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N2[i] = i;
  }
  if (Nave > 1) sort_coords_index (X2, Y2, N2, Nave);

  /* choose a radius for matches */
  RADIUS2 = RADIUS*RADIUS;

# define NEXTi { i++; continue; }
# define NEXTj { j++; continue; }
                                                                                                                                                                                 
  /** find matched stars **/
  for (i = j = 0; (i < Npoints) && (j < Nave); ) {
    if (!inCatalog[N1[i]]) NEXTi;
    if (!finite(X1[i]) || !finite(Y1[i])) NEXTi;
    if (!finite(X2[j]) || !finite(Y2[j])) NEXTj;
    
    /* negative dX: j is too large */
    dX = X1[i] - X2[j];
    if (dX <= -1.02*RADIUS) NEXTi;

    /* positive dX, i is too large */
    if (dX >= 1.02*RADIUS) NEXTj;

    /* within match range; look for matches */
    for (J = j; (dX > -1.02*RADIUS) && (J < Nave); J++) {
      /* find closest match for this detection */
      dX = X1[i] - X2[J];
      dY = Y1[i] - Y2[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;

      /*** a match is found, set the result for this entry ***/
      result[Nresult].Ncat = N2[J];
      result[Nresult].Nseq = N1[i];
      result[Nresult].Roff = sqrt(dR);

      Nresult ++;
      CHECK_REALLOCATE (result, AvselectResult, NRESULT, Nresult, 1000);
    }
    NEXTi;
  }

  free (X1);
  free (Y1);
  free (N1);
  free (N2);
  free (X2);
  free (Y2);

  *nresult = Nresult;
  return (result);
}
