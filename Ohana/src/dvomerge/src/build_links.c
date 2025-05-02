# include "dvomerge.h"

/* 

There are two modes for the measure table: sorted and unsorted. 

In sorted mode, all measures associated with a given average are in a single block.  The block
is pointed to by average->measureOffset and the range is average->Nmeasure

In unsorted mode, it is not possible to go directly from average to measure without scanning.
In this case, it is necessary to use the value measure->averef to find the corresponding
average entry.  

Note that average->measureOffset and measure->averef are only valid for a given load of the
data: they refer to the sequence number in the data blocks.

next_measure is a list of the equivalent sequence of the measure block as if it were sorted.

to find the sequence of measurements for a given average:
n_0 = average->measureOffset
n_1 = next_measure[n_0]
n_i = next_measure[n_i-1]

*/

/*** Measure ****************************************************************************************/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].measureOffset,Nmeasure values */
off_t *init_measure_links (Average *average, off_t Naverage, Measure *measure, off_t Nmeasure) {

  off_t i, j, N;
  off_t *next_measure;

  if (!measure) return NULL;
  if (SKIP_MEASURE) return NULL;

  N = 0;

  ALLOCATE (next_measure, off_t, Nmeasure);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nmeasure) continue;
    off_t m = average[i].measureOffset;
    myAssert (measure[m].averef == i, "not sorted");
    for (j = 0; j < average[i].Nmeasure - 1; j++, N++) {
      myAssert (measure[m+j+1].averef == i, "not sorted");
      next_measure[N] = N + 1;
      if (N >= Nmeasure) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_measure[N] = -1;
    if (N >= Nmeasure) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }

    if (N >= Nmeasure) {
      fprintf (stderr, "overflow in init_measure_links\n");
      abort ();
    }
    N++;
  }
  return (next_measure);
}

/* construct measure links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_measure_links (Average *average, off_t Naverage, Measure *measure, off_t Nmeasure) {

  off_t i, m, k, Nm, averef;
  off_t *next_measure;

  ALLOCATE (next_measure, off_t, Nmeasure);

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].measureOffset = -1;
    average[i].Nmeasure     =  0;
  }

  for (Nm = 0; Nm < Nmeasure; Nm++) {
    averef = measure[Nm].averef;
    m = average[averef].measureOffset;  
    next_measure[Nm] = -1;

    if (m == -1) { /* no links yet for source */
      average[averef].measureOffset = Nm;
      average[averef].Nmeasure     = 1;
      continue;
    }

    for (k = 0; next_measure[m] != -1; k++) {
      m = next_measure[m];
      if (m >= Nmeasure) {
	fprintf (stderr, "WARNING: m out of bounds (1)\n");
      }
    }

    average[averef].Nmeasure = k + 2;
    next_measure[m] = Nm;
    if (m >= Nmeasure) {
      fprintf (stderr, "WARNING: m out of bounds (2)\n");
    }
  }
  return (next_measure);
}

/* average[].measureOffset, average[].Nmeasure are valid within an addstar run */
int add_measure_link (Average *average, off_t *next_measure, off_t Nmeasure, off_t NMEASURE) {

  off_t k, m;

  /* if we have trouble, check validity of next_measure[m] : m < Nmeasure */
  m = average[0].measureOffset;  

  for (k = 0; k < average[0].Nmeasure - 1; k++)  {
    m = next_measure[m];
    if (m >= NMEASURE) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_measure[Nmeasure] = -1;
  if (Nmeasure >= NMEASURE) {
    fprintf (stderr, "WARNING: Nmeasure out of bounds (1)\n");
  }

  // if Nmeasure is 0, m may have been mis-set; add to the end
  if ((average[0].Nmeasure == 0) || (m == -1)) {
    average[0].measureOffset = Nmeasure;
  } else {
    next_measure[m] = Nmeasure;
    if (m >= NMEASURE) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

Measure *sort_measure (Average *average, off_t Naverage, Measure *measure, off_t Nmeasure, off_t *next_measure) {

  off_t i, k, n, np, N;
  Measure *tmpmeasure;

  /* fix order of Measure (memory intensive, but fast) */
  np = -1; // previous entry in case of errors
  N = 0; 
  ALLOCATE (tmpmeasure, Measure, Nmeasure);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nmeasure) continue;
    n = average[i].measureOffset;
    average[i].measureOffset = N;
    int myObjID = average[i].objID;
    for (k = 0; k < average[i].Nmeasure; k++, N++) {
      if (n == -1) {
	fprintf (stderr, "entry after %d has a problem\n", (int) np);
	abort();
      }
      tmpmeasure[N] = measure[n]; 
      myAssert (measure[n].averef == i, "error in averef?");
      myAssert (measure[n].objID == myObjID, "error in objID?");
      tmpmeasure[N].averef = i;
      np = n;
      n = next_measure[n];
    }
  }
  free (measure);
  return (tmpmeasure);
}

/*** Missing ****************************************************************************************/

/* build the initial links assuming the table is sorted */
off_t *init_missing_links (Average *average, off_t Naverage, Missing *missing, off_t Nmissing) {
  OHANA_UNUSED_PARAM(missing);

  off_t i, j, N;
  off_t *next_missing;

  if (!missing) return NULL;
  if (SKIP_MISSING) return NULL;

  N = 0;

  ALLOCATE (next_missing, off_t, Nmissing);
  for (i = 0; i < Naverage; i++) {
    for (j = 0; j < average[i].Nmissing - 1; j++, N++) {
      next_missing[N] = N + 1;
    }
    if (average[i].Nmissing > 0) {
      next_missing[N] = -1;
      if (N >= Nmissing) {
	fprintf (stderr, "overflow in init_missing_links");
	abort ();
      }
      N++;
    }

  }
  return (next_missing);
}

int add_missing_link (Average *average, off_t *next_missing, off_t Nmissing) {

  off_t k, m;

  /* there may be 0 Nmiss; this is not true for Nmeas */
  if (average[0].Nmissing < 1) {
    average[0].missingOffset = Nmissing;
    next_missing[Nmissing] = -1;
    return (TRUE);
  }

  m = average[0].missingOffset;  
  for (k = 0; k < average[0].Nmissing - 1; k++) m = next_missing[m];
  /* set up references */
  next_missing[Nmissing] = -1;
  next_missing[m] = Nmissing;
  return (TRUE);
}

/* Missing does not carry enough information to reconstruct the links
   we must always save the missing table, if it exists */

Missing *sort_missing (Average *average, off_t Naverage, Missing *missing, off_t Nmissing, off_t *next_missing) {

  off_t i, k, n, N;
  Missing *tmpmissing;

  /* fix order of Missing (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmpmissing, Missing, Nmissing);
  for (i = 0; i < Naverage; i++) {
    n = average[i].missingOffset;
    average[i].missingOffset = N;
    for (k = 0; k < average[i].Nmissing; k++, N++) {
      tmpmissing[N] = missing[n]; 
      n = next_missing[n];
    }
  }
  free (missing);
  return (tmpmissing);
}

/*** Lensing ****************************************************************************************/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].lensingOffset,Nlensing values */
off_t *init_lensing_links (Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing) {

  off_t i, j, N;
  off_t *next_lensing;

  if (!lensing) return NULL;
  if (SKIP_LENSING) return NULL;

  N = 0;

  ALLOCATE (next_lensing, off_t, Nlensing);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nlensing) continue;
    off_t m = average[i].lensingOffset;
    myAssert (lensing[m].averef == i, "not sorted");
    for (j = 0; j < average[i].Nlensing - 1; j++, N++) {
      myAssert (lensing[m+j+1].averef == i, "not sorted");
      next_lensing[N] = N + 1;
      if (N >= Nlensing) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_lensing[N] = -1;
    if (N >= Nlensing) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }

    if (N >= Nlensing) {
      fprintf (stderr, "overflow in init_lensing_links\n");
      abort ();
    }
    N++;
  }
  return (next_lensing);
}

/* construct lensing links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_lensing_links (Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing) {

  off_t i, m, k, Nm, averef;
  off_t *next_lensing;

  ALLOCATE (next_lensing, off_t, Nlensing);

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].lensingOffset = -1;
    average[i].Nlensing     =  0;
  }

  for (Nm = 0; Nm < Nlensing; Nm++) {
    averef = lensing[Nm].averef;
    m = average[averef].lensingOffset;  
    next_lensing[Nm] = -1;

    if (m == -1) { /* no links yet for source */
      average[averef].lensingOffset = Nm;
      average[averef].Nlensing     = 1;
      continue;
    }

    for (k = 0; next_lensing[m] != -1; k++) {
      m = next_lensing[m];
      if (m >= Nlensing) {
	fprintf (stderr, "WARNING: m out of bounds (1)\n");
      }
    }

    average[averef].Nlensing = k + 2;
    next_lensing[m] = Nm;
    if (m >= Nlensing) {
      fprintf (stderr, "WARNING: m out of bounds (2)\n");
    }
  }
  return (next_lensing);
}

/* average[].lensingOffset, average[].Nlensing are valid within an addstar run */
int add_lensing_link (Average *average, off_t *next_lensing, off_t Nlensing, off_t NLENSING) {

  off_t k, m;

  /* if we have trouble, check validity of next_lensing[m] : m < Nlensing */
  m = average[0].lensingOffset;  

  for (k = 0; k < average[0].Nlensing - 1; k++)  {
    m = next_lensing[m];
    if (m >= NLENSING) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_lensing[Nlensing] = -1;
  if (Nlensing >= NLENSING) {
    fprintf (stderr, "WARNING: Nlensing out of bounds (1)\n");
  }

  // if Nlensing is 0, m may have been mis-set; add to the end
  if ((average[0].Nlensing == 0) || (m == -1)) {
    average[0].lensingOffset = Nlensing;
  } else {
    next_lensing[m] = Nlensing;
    if (m >= NLENSING) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

Lensing *sort_lensing (Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing, off_t *next_lensing) {

  off_t i, k, n, np, N;
  Lensing *tmplensing;

  /* fix order of Lensing (memory intensive, but fast) */
  np = -1;
  N = 0; 
  ALLOCATE (tmplensing, Lensing, Nlensing);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nlensing) continue;
    n = average[i].lensingOffset;
    average[i].lensingOffset = N;
    int myObjID = average[i].objID;
    for (k = 0; k < average[i].Nlensing; k++, N++) {
      if (n == -1) {
	fprintf (stderr, "entry after %d has a problem\n", (int) np);
	abort();
      }
      tmplensing[N] = lensing[n]; 
      myAssert (lensing[n].averef == i, "error in averef");
      myAssert (lensing[n].objID == myObjID, "error in objID?");
      tmplensing[N].averef = i;
      np = n;
      n = next_lensing[n];
    }
  }
  free (lensing);
  return (tmplensing);
}

/*** Lensobj ****************************************************************************************/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].lensobjOffset,Nlensobj values */
off_t *init_lensobj_links (Average *average, off_t Naverage, Lensobj *lensobj, off_t Nlensobj) {

  off_t i, j, N;
  off_t *next_lensobj;

  if (!lensobj) return NULL;
  if (SKIP_LENSOBJ) return NULL;

  N = 0;

  ALLOCATE (next_lensobj, off_t, Nlensobj);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nlensobj) continue;
    // off_t m = average[i].lensobjOffset;
    // myAssert (lensobj[m].averef == i, "not sorted");
    for (j = 0; j < average[i].Nlensobj - 1; j++, N++) {
      // myAssert (lensobj[m+j+1].averef == i, "not sorted");
      next_lensobj[N] = N + 1;
      if (N >= Nlensobj) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_lensobj[N] = -1;
    if (N >= Nlensobj) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }

    if (N >= Nlensobj) {
      fprintf (stderr, "overflow in init_lensobj_links\n");
      abort ();
    }
    N++;
  }
  return (next_lensobj);
}

/* construct lensobj links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_lensobj_links (Average *average, off_t Naverage, Lensobj *lensobj, off_t Nlensobj) {

  if (Nlensobj) {
    fprintf (stderr, "input is not sorted but contains lensobj -- trouble\n");
    exit (2);
  }

  off_t i;
  off_t *next_lensobj;

  ALLOCATE (next_lensobj, off_t, Nlensobj);

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].lensobjOffset = -1;
    average[i].Nlensobj     =  0;
  }

  return (next_lensobj);
}

/* average[].lensobjOffset, average[].Nlensobj are valid within an addstar run */
int add_lensobj_link (Average *average, off_t *next_lensobj, off_t Nlensobj, off_t NLENSOBJ) {

  off_t k, m;

  /* if we have trouble, check validity of next_lensobj[m] : m < Nlensobj */
  m = average[0].lensobjOffset;  

  for (k = 0; k < average[0].Nlensobj - 1; k++)  {
    m = next_lensobj[m];
    if (m >= NLENSOBJ) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_lensobj[Nlensobj] = -1;
  if (Nlensobj >= NLENSOBJ) {
    fprintf (stderr, "WARNING: Nlensobj out of bounds (1)\n");
  }

  // if Nlensobj is 0, m may have been mis-set; add to the end
  if ((average[0].Nlensobj == 0) || (m == -1)) {
    average[0].lensobjOffset = Nlensobj;
  } else {
    next_lensobj[m] = Nlensobj;
    if (m >= NLENSOBJ) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

Lensobj *sort_lensobj (Average *average, off_t Naverage, Lensobj *lensobj, off_t Nlensobj, off_t *next_lensobj) {

  off_t i, k, n, np, N;
  Lensobj *tmplensobj;

  /* fix order of Lensobj (memory intensive, but fast) */
  np = -1;
  N = 0; 
  ALLOCATE (tmplensobj, Lensobj, Nlensobj);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nlensobj) continue;
    n = average[i].lensobjOffset;
    average[i].lensobjOffset = N;
    int myObjID = average[i].objID;
    for (k = 0; k < average[i].Nlensobj; k++, N++) {
      if (n == -1) {
	fprintf (stderr, "entry after %d has a problem\n", (int) np);
	abort();
      }
      tmplensobj[N] = lensobj[n]; 
      // myAssert (lensobj[n].averef == i, "error in averef");
      myAssert ((lensobj[n].objID == myObjID) || (lensobj[n].objID == -1), "error in objID?");
      // tmplensobj[N].averef = i;
      np = n;
      n = next_lensobj[n];
    }
  }
  free (lensobj);
  return (tmplensobj);
}

/*** StarPar ********************************************************************************/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].starparOffset,Nstarpar values */
off_t *init_starpar_links (Average *average, off_t Naverage, StarPar *starpar, off_t Nstarpar) {

  off_t i, j, N;
  off_t *next_starpar;

  if (!starpar) return NULL;
  if (SKIP_STARPAR) return NULL;

  N = 0;

  // NOTE that is we choose DVO_SKIP_STARPAR, catalog.starpar is NULL.
  // this code will let merge_catalogs_old.c do nothing for starpar
  ALLOCATE (next_starpar, off_t, Nstarpar);
  if (!starpar) return next_starpar;

  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nstarpar) continue;
    off_t m = average[i].starparOffset;
    myAssert (starpar[m].averef == i, "not sorted");
    for (j = 0; j < average[i].Nstarpar - 1; j++, N++) {
      myAssert (starpar[m+j+1].averef == i, "not sorted");
      next_starpar[N] = N + 1;
      if (N >= Nstarpar) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_starpar[N] = -1;
    if (N >= Nstarpar) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }

    if (N >= Nstarpar) {
      fprintf (stderr, "overflow in init_starpar_links\n");
      abort ();
    }
    N++;
  }
  return (next_starpar);
}

/* construct starpar links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_starpar_links (Average *average, off_t Naverage, StarPar *starpar, off_t Nstarpar) {

  off_t i, m, k, Nm, averef;
  off_t *next_starpar;

  ALLOCATE (next_starpar, off_t, Nstarpar);
  if (!starpar) return next_starpar;

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].starparOffset = -1;
    average[i].Nstarpar     =  0;
  }

  for (Nm = 0; Nm < Nstarpar; Nm++) {
    averef = starpar[Nm].averef;
    m = average[averef].starparOffset;  
    next_starpar[Nm] = -1;

    if (m == -1) { /* no links yet for source */
      average[averef].starparOffset = Nm;
      average[averef].Nstarpar     = 1;
      continue;
    }

    for (k = 0; next_starpar[m] != -1; k++) {
      m = next_starpar[m];
      if (m >= Nstarpar) {
	fprintf (stderr, "WARNING: m out of bounds (1)\n");
      }
    }

    average[averef].Nstarpar = k + 2;
    next_starpar[m] = Nm;
    if (m >= Nstarpar) {
      fprintf (stderr, "WARNING: m out of bounds (2)\n");
    }
  }
  return (next_starpar);
}

/* average[].starparOffset, average[].Nstarpar are valid within an addstar run */
int add_starpar_link (Average *average, off_t *next_starpar, off_t Nstarpar, off_t NSTARPAR) {

  off_t k, m;

  /* if we have trouble, check validity of next_starpar[m] : m < Nstarpar */
  m = average[0].starparOffset;  

  for (k = 0; k < average[0].Nstarpar - 1; k++)  {
    m = next_starpar[m];
    if (m >= NSTARPAR) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_starpar[Nstarpar] = -1;
  if (Nstarpar >= NSTARPAR) {
    fprintf (stderr, "WARNING: Nstarpar out of bounds (1)\n");
  }

  // if Nmeasure is 0, m may have been mis-set; add to the end
  if ((average[0].Nstarpar == 0) || (m == -1)) {
    average[0].starparOffset = Nstarpar;
  } else {
    next_starpar[m] = Nstarpar;
    if (m >= NSTARPAR) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

StarPar *sort_starpar (Average *average, off_t Naverage, StarPar *starpar, off_t Nstarpar, off_t *next_starpar) {

  off_t i, k, n, np, N;
  StarPar *tmpstarpar;

  if (!starpar) return NULL;
  if (SKIP_STARPAR) return NULL;

  /* fix order of StarPar (memory intensive, but fast) */
  np = -1; // previous entry in case of errors
  N = 0; 
  ALLOCATE (tmpstarpar, StarPar, Nstarpar);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nstarpar) continue;
    n = average[i].starparOffset;
    average[i].starparOffset = N;
    int myObjID = average[i].objID;
    for (k = 0; k < average[i].Nstarpar; k++, N++) {
      if (n == -1) {
	fprintf (stderr, "entry after %d has a problem\n", (int) np);
	abort();
      }
      tmpstarpar[N] = starpar[n]; 
      myAssert (starpar[n].averef == i, "error in averef?");
      myAssert (starpar[n].objID == myObjID, "error in objID?");
      tmpstarpar[N].averef = i;
      np = n;
      n = next_starpar[n];
    }
  }
  free (starpar);
  return (tmpstarpar);
}

/*** GalPhot ****************************************************************************************/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].galphotOffset,Ngalphot values */
off_t *init_galphot_links (Average *average, off_t Naverage, GalPhot *galphot, off_t Ngalphot) {

  off_t i, j, N;
  off_t *next_galphot;

  if (!galphot) return NULL;
  if (SKIP_GALPHOT) return NULL;

  N = 0;

  ALLOCATE (next_galphot, off_t, Ngalphot);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Ngalphot) continue;
    off_t m = average[i].galphotOffset;
    myAssert (galphot[m].averef == i, "not sorted");
    for (j = 0; j < average[i].Ngalphot - 1; j++, N++) {
      myAssert (galphot[m+j+1].averef == i, "not sorted");
      next_galphot[N] = N + 1;
      if (N >= Ngalphot) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_galphot[N] = -1;
    if (N >= Ngalphot) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }

    if (N >= Ngalphot) {
      fprintf (stderr, "overflow in init_galphot_links\n");
      abort ();
    }
    N++;
  }
  return (next_galphot);
}

/* construct galphot links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_galphot_links (Average *average, off_t Naverage, GalPhot *galphot, off_t Ngalphot) {

  off_t i, m, k, Nm, averef;
  off_t *next_galphot;

  ALLOCATE (next_galphot, off_t, Ngalphot);

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].galphotOffset = -1;
    average[i].Ngalphot     =  0;
  }

  for (Nm = 0; Nm < Ngalphot; Nm++) {
    averef = galphot[Nm].averef;
    m = average[averef].galphotOffset;  
    next_galphot[Nm] = -1;

    if (m == -1) { /* no links yet for source */
      average[averef].galphotOffset = Nm;
      average[averef].Ngalphot     = 1;
      continue;
    }

    for (k = 0; next_galphot[m] != -1; k++) {
      m = next_galphot[m];
      if (m >= Ngalphot) {
	fprintf (stderr, "WARNING: m out of bounds (1)\n");
      }
    }

    average[averef].Ngalphot = k + 2;
    next_galphot[m] = Nm;
    if (m >= Ngalphot) {
      fprintf (stderr, "WARNING: m out of bounds (2)\n");
    }
  }
  return (next_galphot);
}

/* average[].galphotOffset, average[].Ngalphot are valid within an addstar run */
int add_galphot_link (Average *average, off_t *next_galphot, off_t Ngalphot, off_t NGALPHOT) {

  off_t k, m;

  /* if we have trouble, check validity of next_galphot[m] : m < Ngalphot */
  m = average[0].galphotOffset;  

  for (k = 0; k < average[0].Ngalphot - 1; k++)  {
    m = next_galphot[m];
    if (m >= NGALPHOT) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_galphot[Ngalphot] = -1;
  if (Ngalphot >= NGALPHOT) {
    fprintf (stderr, "WARNING: Ngalphot out of bounds (1)\n");
  }

  // if Ngalphot is 0, m may have been mis-set; add to the end
  if ((average[0].Ngalphot == 0) || (m == -1)) {
    average[0].galphotOffset = Ngalphot;
  } else {
    next_galphot[m] = Ngalphot;
    if (m >= NGALPHOT) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

GalPhot *sort_galphot (Average *average, off_t Naverage, GalPhot *galphot, off_t Ngalphot, off_t *next_galphot) {

  off_t i, k, n, np, N;
  GalPhot *tmpgalphot;

  /* fix order of GalPhot (memory intensive, but fast) */
  np = -1;
  N = 0; 
  ALLOCATE (tmpgalphot, GalPhot, Ngalphot);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Ngalphot) continue;
    n = average[i].galphotOffset;
    average[i].galphotOffset = N;
    int myObjID = average[i].objID;
    for (k = 0; k < average[i].Ngalphot; k++, N++) {
      if (n == -1) {
	fprintf (stderr, "entry after %d has a problem\n", (int) np);
	abort();
      }
      tmpgalphot[N] = galphot[n]; 
      myAssert (galphot[n].averef == i, "error in averef?");
      myAssert (galphot[n].objID == myObjID, "error in objID?");
      tmpgalphot[N].averef = i;
      np = n;
      n = next_galphot[n];
    }
  }
  free (galphot);
  return (tmpgalphot);
}

