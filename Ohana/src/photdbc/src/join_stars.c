# include "photdbc.h"

void join_stars (Catalog *catalog) {

  off_t i, j, k, m, Ni, Nj, first_j;
  off_t Naves, Nmeas, Ncurr;
  off_t Naverage, Nmeasure, *index;
  double *X, *Y, dX, dY, dR, RADIUS2;
  double Rmid, Dmid;
  int basecode, baseNsec, Nsecfilt, *found;

  Average *naverage, *average;
  Measure *nmeasure, *measure;
  SecFilt *nsecfilt, *secfilt;
  Mpointer *mpointer;
  Coords tcoords;

  if (VERBOSE) fprintf (stderr, "joining overlapping stars\n");
  if (VERBOSE) fprintf (stderr, "require base star to have i-band\n");

  basecode = GetPhotcodeCodebyName ("i");
  baseNsec = GetPhotcodeNsec (basecode);
  Nsecfilt = catalog[0].Nsecfilt;

  average = catalog[0].average;
  measure = catalog[0].measure;
  secfilt = catalog[0].secfilt;

  Naverage = catalog[0].Naverage;
  Nmeasure = catalog[0].Nmeasure;
  RADIUS2 = JOIN_RADIUS*JOIN_RADIUS;

  /* reference for coords is this region */
  Ncurr = 0;
  Naves = 0;
  Rmid = Dmid = 0;
  for (i = 0; i < Naverage; i++) {
    // XXX we should be vigilant against R,D becoming nan : this must be due to pm...
    if (isnan(average[i].R) || isnan(average[i].D)) {
      average[i].R = 0.0;
      average[i].D = 0.0;
      continue;
    }
    Rmid += average[i].R;
    Dmid += average[i].D;
    Naves ++;
  }
  Rmid /= Naverage;
  Dmid /= Naverage;

  /* coordinate system for projection */
  InitCoords (&tcoords, "DEC--TAN");
  tcoords.crval1 = Rmid;
  tcoords.crval2 = Dmid;
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;
  
  /* project & sort coordinates in local linear frame */
  ALLOCATE (X, double, Naverage);
  ALLOCATE (Y, double, Naverage);
  ALLOCATE (index, off_t, Naverage);
  for (i = 0; i < Naverage; i++) {
    index[i] = i;
    RD_to_XY (&X[i], &Y[i], average[i].R, average[i].D, &tcoords);
  }
  sort_coords_index (X, Y, index, Naverage);
  
  /* flags to mark if a star has been handled */
  ALLOCATE (found, int, Naverage);
  for (i = 0; i < Naverage; i++) found[i] = FALSE;

  ALLOCATE (naverage, Average, Naverage);
  ALLOCATE (nsecfilt, SecFilt, Naverage*Nsecfilt);
  ALLOCATE (mpointer, Mpointer, Nmeasure);
  for (i = 0; i < Nmeasure; i++) mpointer[i].averef = -1;

  Naves =  0; // counter for new averages
  Nmeas =  0; // counter for new measures
  for (i = j = 0; (i < Naverage) && (j < Naverage); ) {
    
    Ni = index[i];
    Nj = index[j];

    // if ((average[Ni].R > 131.259) && (average[Ni].R < 131.267) && (average[Ni].D > 20.440) && (average[Ni].D < 20.450)) {
    // fprintf (stderr, "outer: %f, %f - %f, %f (%f, %f) == (%f, %f)\n", average[Ni].R, average[Ni].D, average[Nj].R, average[Nj].D, 
    // 3600.0*(average[Ni].R - average[Nj].R), 3600.0*(average[Ni].D - average[Nj].D), X[i] - X[j], Y[i] - Y[j]);
    // }

    // require base star to meet certain conditions:
    if (isnan(secfilt[Ni*Nsecfilt + baseNsec].MpsfChp)) {
      i++;
      continue;
    }

    /* a new star, add it to naverage[] */
    if (!found[i]) { 	
      naverage[Naves]               = average[Ni];
      naverage[Naves].measureOffset = Nmeas;

      for (k = 0; k < Nsecfilt; k++) {
	nsecfilt[Naves*Nsecfilt + k] = secfilt[Ni*Nsecfilt + k];
      }

      for (k = 0; k < average[Ni].Nmeasure; k++) {
	m = average[Ni].measureOffset + k;
	mpointer[Nmeas].measure = m;
	mpointer[Nmeas].averef  = Naves;
	mpointer[Nmeas].R       = measure[m].R;
	mpointer[Nmeas].D       = measure[m].D;
	Nmeas ++;
      }
      Ncurr = Naves;
      found[i] = TRUE;
      Naves ++;
    }

    if (found[j]) { j++; continue; }  // don't duplicate
    if (j == i)   { j++; continue; }  // don't auto-correlate

    dX = X[i] - X[j];
    if (dX <= -JOIN_RADIUS) { /* X[j] is too large */
      while (found[i] && (i < Naverage)) i++;
      continue;
    }
    if (dX >= +JOIN_RADIUS) { /* X[i] is too large */
      j++;
      continue;
    }

    first_j = j;
    for (; (dX > -2*JOIN_RADIUS) && (j < Naverage); j++) {
      Nj = index[j];

      // if ((average[Ni].R > 131.259) && (average[Ni].R < 131.267) && (average[Ni].D > 20.440) && (average[Ni].D < 20.450)) {
      // fprintf (stderr, "inner: %f, %f - %f, %f (%f, %f) == (%f, %f)\n", average[Ni].R, average[Ni].D, average[Nj].R, average[Nj].D, 
      // 3600.0*(average[Ni].R - average[Nj].R), 3600.0*(average[Ni].D - average[Nj].D), X[i] - X[j], Y[i] - Y[j]);
      // }

      if (found[j]) continue;
      dX = X[i] - X[j];
      dY = Y[i] - Y[j];
      dR = dX*dX + dY*dY;
      if (dR < RADIUS2) {  /* matched star, join to first */

	// if ((average[Ni].R > 131.259) && (average[Ni].R < 131.267) && (average[Ni].D > 20.440) && (average[Ni].D < 20.450)) {
	// fprintf (stderr, "match: %f, %f - %f, %f\n", average[Ni].R, average[Ni].D, average[Nj].R, average[Nj].D);
	// }

	/* define pointers for new measures */
	for (k = 0; k < average[Nj].Nmeasure; k++) {
	  m = average[Nj].measureOffset + k;
	  mpointer[Nmeas].measure = m;
	  mpointer[Nmeas].averef  = Ncurr;
	  mpointer[Nmeas].R       = measure[m].R;
	  mpointer[Nmeas].D       = measure[m].D;
	  Nmeas ++;
	}
	naverage[Ncurr].Nmeasure += average[Nj].Nmeasure;
	found[j] = TRUE;

# if 0
	/* recalculate naverage[Ncurr].RA,DEC */
	/* this must be done here to keep the average position consistent
	   for the next star found */
	for (Sr = Sd = k = 0; k < naverage[Ncurr].Nmeasure; k++) {
	  m = naverage[Ncurr].measureOffset + k;
	  Sr += mpointer[m].R;
	  Sd += mpointer[m].D;
	}
	Sr = Sr / naverage[Ncurr].Nmeasure;
	Sd = Sd / naverage[Ncurr].Nmeasure;
	naverage[Ncurr].R = Sr;
	naverage[Ncurr].D = Sd;

	/* update original measurement offsets for new detections */
	// for (k = Nfirst; k < naverage[Ncurr].Nmeasure; k++) {
	//   m = naverage[Ncurr].measureOffset + k;
	//   M = mpointer[m].measure;
	//   measure[M].dR = 3600.0*(Sr - mpointer[m].R);
	//   measure[M].dD = 3600.0*(Sd - mpointer[m].D);
	// }

	/* update current reference star position */
	RD_to_XY (&X[i], &Y[i], Sr, Sd, &tcoords);
# else
	/* update original measurement offsets for new detections */
	// now not needed measure[M] carries R,D
	// for (k = Nfirst; k < naverage[Ncurr].Nmeasure; k++) {
	//   m = naverage[Ncurr].measureOffset + k;
	//   M = mpointer[m].measure;
	//   measure[M].dR = 3600.0*(naverage[Ncurr].R - mpointer[m].R);
	//   measure[M].dD = 3600.0*(naverage[Ncurr].D - mpointer[m].D);
	// }
# endif
      }
    }
    while (found[i] && (i < Naverage)) i++;
    j = first_j;
  }

  if (Nmeas != Nmeasure) {
    fprintf (stderr, "failure to match "OFF_T_FMT" measures ("OFF_T_FMT" of "OFF_T_FMT" matched)\n", Nmeasure - Nmeas, Nmeas, Nmeasure);
  }
  
  /* create a new Measure table in the appropriate sequence */
  ALLOCATE (nmeasure, Measure, Nmeas);
  for (i = 0; i < Nmeas; i++) {
    nmeasure[i]        = measure[mpointer[i].measure];
    nmeasure[i].averef = mpointer[i].averef;
  }

  /* create an empty SecFilt table */
  // ALLOCATE (nsecfilt, SecFilt, Naves*catalog[0].Nsecfilt);
  // bzero (nsecfilt, Naves*catalog[0].Nsecfilt*sizeof(SecFilt));

  free (catalog[0].average);
  free (catalog[0].measure);
  free (catalog[0].secfilt);

  catalog[0].average = naverage;
  catalog[0].measure = nmeasure;
  catalog[0].secfilt = nsecfilt;
  
  // allow output catalog to retain fewer measures
  catalog[0].Naverage = Naves;
  catalog[0].Nmeasure = Nmeas;
  catalog[0].Nsecfilt_mem = Naves*catalog[0].Nsecfilt;
  
  return;
}

/* notes:
   merge averages of stars within RADIUS to single stars 
   updates the average RA and DEC fields
   creates a new Average table 'naverage' 
   updates the Measure table to the new sequence
   generates a new SecFilt table with empty values 
   does NOT update magnitudes 
   output measure table is the same size as the input measure table
   (measures are just moved around).  thus, catalog[0].Nmeasure does not change.
*/   
