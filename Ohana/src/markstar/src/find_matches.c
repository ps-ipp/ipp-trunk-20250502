# include "markstar.h"

int find_matches (Catalog *catalog, CatStats *catstats, Catalog *Ghostdata, int Nimage) {

  int i, j, k, n, m, N, M, first_j;
  double X, Y, RADIUS, RADIUS2;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2;
  int Nstar, Nghost, Ng;
  unsigned int flags;
  Coords *tcoords;

  Nstar = catalog[0].Naverage;
  Nghost = Ghostdata[0].Naverage;

  if (Nghost < 1) return (0);

  /* it is better to have the catalog with fewer stars
     assigned to the X1 set */
  X2 = catstats[0].X;
  Y2 = catstats[0].Y;
  N2 = catstats[0].N;

  ALLOCATE (X1, double, Nghost);
  ALLOCATE (Y1, double, Nghost);
  ALLOCATE (N1, off_t, Nghost);

  /* project ghosts to the frame of the catalog */
  tcoords = &catstats[0].coords;
  for (i = 0; i < Nghost; i++) {
    RD_to_XY (&X1[i], &Y1[i], Ghostdata[0].average[i].R, Ghostdata[0].average[i].D, tcoords);
    N1[i] = i;
  }
  if (Nghost > 1) sort_coords_index (X1, Y1, N1, Nghost);

  /* choose a radius for matches */
  RADIUS2 = GHOST_RADIUS*GHOST_RADIUS;

  /** find matched stars **/
  for (i = j = 0; (i < Nghost) && (j < Nstar); ) {
    dX = X1[i] - X2[j];
    if (dX <= -2*GHOST_RADIUS) {
      i++;
      continue;
    }
    if (dX >= 2*GHOST_RADIUS) {
      j++;
      continue;
    }
    /* negative dX: j is too large, positive dX, i is too large */
    first_j = j;
    for (; (dX > -2*GHOST_RADIUS) && (j < Nstar); j++) {
      dX = X1[i] - X2[j];
      dY = Y1[i] - Y2[j];
      dR = dX*dX + dY*dY;
      if (dR < RADIUS2) {  
	/* this object may be a ghost star, 
	   but only mark those measurements on the correct image */
	M = N2[j];
	m = catalog[0].average[M].offset;
	Ng = 0;
	for (k = 0; k < catalog[0].average[M].Nm; k++) {
	  if (catalog[0].image[m+k] == Nimage) {
	    catalog[0].measure[m+k].average |= GHOST_DATA;
	    Ng ++;
	  }
	}
	/* all measurements are ghosts */
	if (catalog[0].average[M].Nm == Ng) {
	  catalog[0].average[M].code = ID_GHOST;
	}
      }
    }
    j = first_j;
    i++;
  }

  free (X1);
  free (Y1);
  free (N1);

}
