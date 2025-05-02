# include "markstar.h"

void find_bright_stars (Catalog *catalog, CatStats *catstats) {

  off_t i, j, n, m, first_j, Nave, Ngsc;
  Catalog GSCdata;
  double MinRA, MinDEC, MaxRA, MaxDEC, RaCenter, DecCenter;
  Coords *tcoords;
  double *X1, *Y1, *X2, *Y2;
  off_t *N1, *N2;
  char *mark;
  double dX, dY, dR, MaxDist, MaxDist1, MaxRadius, radius, radius2;

  // put limits on the ref catalog mags
  load_gsc_data (&GSCdata, catstats);

  Nave = catalog[0].Naverage;
  Ngsc = GSCdata.Naverage;
    
  /* in the function below, it is better to have the catalog with
     more stars associated with index 2 (j) */ 
  X2 = catstats[0].X;
  Y2 = catstats[0].Y;
  N2 = catstats[0].N;
  ALLOCATE (X1, double, Ngsc);
  ALLOCATE (Y1, double, Ngsc);
  ALLOCATE (N1, off_t, Ngsc);
  ALLOCATE (mark, char, Nave);
  bzero (mark, Nave);

  tcoords = &catstats[0].coords;
  for (i = 0; i < Ngsc; i++) {
    RD_to_XY (&X1[i], &Y1[i], GSCdata.average[i].R, GSCdata.average[i].D, tcoords);
    N1[i] = i;
  }
  if (Ngsc > 1) sort_coords_index (X1, Y1, N1, Ngsc);
  
  /* first find stellar halos */
  /* max radius (mag = -1) */
  /** the j index moves quickly and is better associated with the catalog with more stars */
  MaxRadius = BRIGHT_HALO_SLOPE * (-1.0 - BRIGHT_HALO_MAG); 
  /** find catalog stars near GSC stars **/
  for (i = j = 0; (i < Ngsc) && (j < Nave); ) {
    dX = X1[i] - X2[j];
    if (dX <= -2*MaxRadius) {
      i++;
      continue;
    }
    if (dX >= 2*MaxRadius) {
      j++;
      continue;
    }
    /* negative dX: j is too large, positive dX, i is too large */
    first_j = j;
    radius = MAX (BRIGHT_HALO_SLOPE * (0.001*GSCdata.average[N1[i]].M - BRIGHT_HALO_MAG), 0.0);
    if (radius == 0) {
      i++; 
      continue;
    }
    radius2 = radius*radius;
    for (; (dX > -2*radius) && (j < Nave); j++) {
      dX = X1[i] - X2[j];
      dY = Y1[i] - Y2[j];
      dR = dX*dX + dY*dY;
      if (dR < radius2) {  /* new measurement of this star */
	mark[j] = TRUE;
      }
    }
    j = first_j;
    i++;
  }
  
  /* next find y spikes */
  /** find catalog stars near GSC stars **/
  for (i = j = 0; (i < Ngsc) && (j < Nave); ) {
    dX = X1[i] - X2[j];
    if (dX <= -BRIGHT_YTRAIL_WIDTH) {
      i++;
      continue;
    }
    if (dX >= BRIGHT_YTRAIL_WIDTH) {
      j++;
      continue;
    }
    /* negative dX: j is too large, positive dX, i is too large */
    first_j = j;
    MaxDist = MAX (BRIGHT_YTRAIL_SLOPE * (0.001*GSCdata.average[N1[i]].M - BRIGHT_YTRAIL_MAG), 0.0);
    for (; (dX > -BRIGHT_YTRAIL_WIDTH) && (j < Nave); j++) {
      dX = X1[i] - X2[j];
      dY = Y1[i] - Y2[j];
      if ((fabs(dX) < BRIGHT_YTRAIL_WIDTH) && (fabs(dY) < MaxDist)) {  /* star on spike */
	mark[j] = TRUE;
      }
    }
    j = first_j;
    i++;
  }
  
  /* next find x spikes */
  /** find catalog stars near GSC stars **/
  MaxDist = MAX (BRIGHT_XTRAIL_SLOPE * (-1.0 - BRIGHT_XTRAIL_MAG), 0.0);
  for (i = j = 0; (i < Ngsc) && (j < Nave); ) {
    dX = X1[i] - X2[j];
    if (dX <= -MaxDist) {
      i++;
      continue;
    }
    if (dX >= MaxDist) {
      j++;
      continue;
    }
    /* negative dX: j is too large, positive dX, i is too large */
    first_j = j;
    MaxDist1 = MAX (BRIGHT_XTRAIL_SLOPE * (0.001*GSCdata.average[N1[i]].M - BRIGHT_XTRAIL_MAG), 0.0);
    for (; (dX > -MaxDist1) && (j < Nave); j++) {
      dX = X1[i] - X2[j];
      dY = Y1[i] - Y2[j];
      if ((fabs(dY) < BRIGHT_XTRAIL_WIDTH) && (fabs(dX) < MaxDist1)) {  /* star on spike */
	mark[j] = TRUE;
      }
    }
    j = first_j;
    i++;
  }

  /* done with search, mark selected stars */
  for (i = 0; i < Nave; i++) {
    if (mark[i]) {
      catalog[0].average[N2[i]].code = ID_BLEED;
    }
  } 
  
  free (X1);
  free (Y1);
  free (N1);
  free (mark);
  free (GSCdata.average);

}

