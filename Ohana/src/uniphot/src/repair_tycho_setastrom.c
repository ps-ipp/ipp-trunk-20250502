# include "setastrom.h"

int repair_tycho_setastrom (Catalog *catalog, SkyRegion *region) {

  double *tychoR, *tychoD;
  int Ntycho;

  short TYCHO_B = GetPhotcodeCodebyName ("TYCHO_B"); myAssert (TYCHO_B, "TYCHO_B photcode not found\n");
  short TYCHO_V = GetPhotcodeCodebyName ("TYCHO_V"); myAssert (TYCHO_V, "TYCHO_V photcode not found\n");

  // the tycho R,D entries are sorted by tychoR
  if (!get_tyc_correction (&tychoR, &tychoD, &Ntycho)) {
    fprintf (stderr, "TYCHO correction not loaded\n");
    exit (2);
  }

  // first select the subset of objects which lie in this catalog:
  int Ns = ohana_bisection_double (tychoR, Ntycho, region->Rmin);
  Ns = MAX(Ns, 0); // if Rmin = 0.0, Ns could be -1 (since there are no entries with R < 0.0

  int NSUBSET = 100;
  int Nsubset = 0;
  int *index = NULL;
  ALLOCATE (index, int, NSUBSET);

  // scan through tycho stars to select those in this catalog
  while ((Ns < Ntycho) && (tychoR[Ns] < region->Rmax)) {
    if (tychoD[Ns] < region->Dmin) continue;
    if (tychoD[Ns] > region->Dmax) continue;
    // we have an object in range (Rmin <= R < Rmax; Dmin <= D <= Dmax)

    index[Nsubset] = Ns;
    Nsubset ++;
    CHECK_REALLOCATE (index, int, NSUBSET, Nsubset, 100);
  }  
  if (Nsubset == 0) {
    free (index);
    return TRUE;
  }
  
  // now find matches within this catalog
  
  /** allocate local arrays (stars) **/
  off_t *N1, *N2;
  double *X1, *Y1, *X2, *Y2;
  ALLOCATE (X1, double, Nsubset);
  ALLOCATE (Y1, double, Nsubset);
  ALLOCATE (N1, off_t,  Nsubset);

  /** allocate local arrays (catalog) **/
  ALLOCATE (X2, double, catalog[0].Naverage);
  ALLOCATE (Y2, double, catalog[0].Naverage);
  ALLOCATE (N2, off_t,  catalog[0].Naverage);

  Coords tcoords;
  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  if (region[0].Dmax < 90) {
    tcoords.crval2 = 0.5*(region[0].Dmin + region[0].Dmax);
  } else {
    tcoords.crval2 = 90.0;
  }
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  /* build spatial index (RA sort) referencing input array sequence */
  int i, j, J, k;
  for (i = 0; i < Nsubset; i++) {
    RD_to_XY (&X1[i], &Y1[i], tychoR[index[i]], tychoD[index[i]], &tcoords);
    N1[i] = i;
  }
  sort_coords_index (X1, Y1, N1, Nsubset);
  
  /* build spatial index (RA sort) */
  for (i = 0; i < catalog[0].Naverage; i++) {
    RD_to_XY (&X2[i], &Y2[i], catalog[0].average[i].R, catalog[0].average[i].D, &tcoords);
    N2[i] = i;
  }
  sort_coords_index (X2, Y2, N2, catalog[0].Naverage);

    /* choose a radius for matches (defined in args.c or ImageOptions.c) */
  float RADIUS = 1.0;
  float RADIUS2 = RADIUS*RADIUS;

  float dX, dY, dR;

  /** find matched stars **/
  for (i = j = 0; (i < Nsubset) && (j < catalog[0].Naverage); ) {
    if (!finite(X1[i]) || !finite(Y1[i])) { i++; continue; }
    if (!finite(X2[j]) || !finite(Y2[j])) { j++; continue; }

    /* negative dX: j is too large; positive dX, i is too large */
    dX = X1[i] - X2[j];
    if (dX <= -1.02*RADIUS) { i++; continue; }
    if (dX >= +1.02*RADIUS) { j++; continue; }

    /* within match range; look for matches */
    for (J = j; (dX > -1.02*RADIUS) && (J < catalog[0].Naverage); J++) {
      dX = X1[i] - X2[J];
      dY = Y1[i] - Y2[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;

      /*** a match is found ***/

      off_t Ns = N1[i];
      off_t Nc = N2[j];

      off_t m = catalog[0].average[Nc].measureOffset;

      // for these stars, the average.R,D values are correct; replace the measure.R,D
      for (k = 0; k < catalog[0].average[Nc].Nmeasure; k++) {
	// find the tycho photcodes:
	int valid = FALSE;
	valid = valid || (catalog[0].measure[m+k].photcode == TYCHO_B);
	valid = valid || (catalog[0].measure[m+k].photcode == TYCHO_V);
	if (!valid) continue;
	catalog[0].measure[m+k].R = tychoR[Ns];
	catalog[0].measure[m+k].D = tychoD[Ns];
      }
    }
  }
  
  free (X1);
  free (X2);
  free (Y1);
  free (Y2);
  free (N1);
  free (N2);
  free (index);

  return TRUE;
}

