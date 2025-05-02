# include "addstar.h"

/* 

There are two modes for the measure table: sorted and unsorted. 

In sorted mode, all measures associated with a given average are in a single block.  The block
is pointed to by average->measureOffset and the range is average->Nmeasure

In unsorted mode, it is not possible to go directly from average to measure without scanning.
In this case, it is necessary to use the value measure->averef to find the corresponding
average entry.  

Note that average->measureOffset and measure->averef are only valid for a given load of the
data: they refer to the sequence number in the data blocks.

next_meas is a list of the equivalent sequence of the measure block as if it were sorted.

to find the sequence of measurements for a given average:
n_0 = average->measureOffset
n_1 = next_meas[n_0]
n_i = next_meas[n_i-1]

*/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].measureOffset,Nmeasure values */
off_t *init_measure_links (Average *average, off_t Naverage, Measure *measure, off_t Nmeasure) {
  OHANA_UNUSED_PARAM(measure);

  off_t i, j, N;
  off_t *next_meas;

  N = 0;

  ALLOCATE (next_meas, off_t, Nmeasure);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nmeasure) continue;
    for (j = 0; j < average[i].Nmeasure - 1; j++, N++) {
      next_meas[N] = N + 1;
      if (N >= Nmeasure) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_meas[N] = -1;
    if (N >= Nmeasure) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }
    if (N >= Nmeasure) {
      myAbort ("overflow in init_measure_links\n");
    }
    N++;
  }
  return (next_meas);
}

/* construct measure links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_measure_links (Average *average, off_t Naverage, Measure *measure, off_t Nmeasure) {

  off_t i, m, k, Nm, averef;
  off_t *next_meas;

  ALLOCATE (next_meas, off_t, Nmeasure);

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].measureOffset = -1;
    average[i].Nmeasure     =  0;
  }

  for (Nm = 0; Nm < Nmeasure; Nm++) {
    averef = measure[Nm].averef;
    m = average[averef].measureOffset;  
    next_meas[Nm] = -1;

    if (m == -1) { /* no links yet for source */
      average[averef].measureOffset = Nm;
      average[averef].Nmeasure     = 1;
      continue;
    }

    for (k = 0; next_meas[m] != -1; k++) {
      m = next_meas[m];
      if (m >= Nmeasure) {
	fprintf (stderr, "WARNING: m out of bounds (1)\n");
      }
    }

    average[averef].Nmeasure = k + 2;
    next_meas[m] = Nm;
    if (m >= Nmeasure) {
      fprintf (stderr, "WARNING: m out of bounds (2)\n");
    }
  }
  return (next_meas);
}

/* average[].measureOffset, average[].Nmeasure are valid within an addstar run */
int add_meas_link (Average *average, off_t *next_meas, off_t Nmeasure, off_t NMEASURE) {

  off_t k, m;

  /* if we have trouble, check validity of next_meas[m] : m < Nmeasure */
  m = average[0].measureOffset;  

  for (k = 0; k < average[0].Nmeasure - 1; k++)  {
    m = next_meas[m];
    if (m >= NMEASURE) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_meas[Nmeasure] = -1;
  if (Nmeasure >= NMEASURE) {
    fprintf (stderr, "WARNING: Nmeasure out of bounds (1)\n");
  }

  if (m == -1) {
    average[0].measureOffset = Nmeasure;
  } else {
    next_meas[m] = Nmeasure;
    if (m >= NMEASURE) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

Measure *sort_measure (Average *average, off_t Naverage, Measure *measure, off_t Nmeasure, off_t *next_meas) {

  int i, k, n, N;
  Measure *tmpmeasure;

  /* fix order of Measure (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmpmeasure, Measure, Nmeasure);
  for (i = 0; i < Naverage; i++) {
    n = average[i].measureOffset;
    average[i].measureOffset = N;
    for (k = 0; k < average[i].Nmeasure; k++, N++) {
      if (n == -1) myAbortF("invalid measureOffset for ave %d, measureOffset %d, measure %d\n", i, average[i].measureOffset, k);
      tmpmeasure[N] = measure[n]; 
      if (measure[n].averef != i) {
	myAbortF("invalid averef for ave %d, measureOffset %d, measure %d, averef %d\n", i, average[i].measureOffset, k, measure[n].averef);
      }
      tmpmeasure[N].averef = i;
      n = next_meas[n];
    }
  }
  free (measure);
  return (tmpmeasure);
}

/*******************************************************************************************/

/* build the initial links assuming the table is sorted */
off_t *init_missing_links (Average *average, off_t Naverage, Missing *missing, off_t Nmissing) {
  OHANA_UNUSED_PARAM(missing);

  off_t i, j, N;
  off_t *next_miss;

  N = 0;

  ALLOCATE (next_miss, off_t, Nmissing);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nmissing) continue;
    for (j = 0; j < average[i].Nmissing - 1; j++, N++) {
      next_miss[N] = N + 1;
    }
    next_miss[N] = -1;
    if (N >= Nmissing) {
      myAbort ("overflow in init_missing_links");
    }
    N++;
  }
  return (next_miss);
}

int add_miss_link (Average *average, off_t *next_miss, off_t Nmissing) {

  off_t k, m;

  /* there may be 0 Nmiss; this is not true for Nmeas */
  if (average[0].Nmissing < 1) {
    average[0].missingOffset = Nmissing;
    next_miss[Nmissing] = -1;
    return (TRUE);
  }

  m = average[0].missingOffset;  
  for (k = 0; k < average[0].Nmissing - 1; k++) m = next_miss[m];
  /* set up references */
  next_miss[Nmissing] = -1;
  next_miss[m] = Nmissing;
  return (TRUE);
}

/* Missing does not carry enough information to reconstruct the links
   we must always save the missing table, if it exists */

Missing *sort_missing (Average *average, off_t Naverage, Missing *missing, off_t Nmissing, off_t *next_miss) {

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
      n = next_miss[n];
    }
  }
  free (missing);
  return (tmpmissing);
}

/*******************************************************************************************/

/* build the initial links assuming the table is sorted, 
   not partial, and has a correct set of average[].lensingOffset,Nlensing values */
off_t *init_lensing_links (Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing) {
  OHANA_UNUSED_PARAM(lensing);

  off_t i, j, N;
  off_t *next_lens;

  N = 0;

  ALLOCATE (next_lens, off_t, Nlensing);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nlensing) continue;
    for (j = 0; j < average[i].Nlensing - 1; j++, N++) {
      next_lens[N] = N + 1;
      if (N >= Nlensing) {
	fprintf (stderr, "WARNING: N out of bounds (1)\n");
      }
    }
    next_lens[N] = -1;
    if (N >= Nlensing) {
      fprintf (stderr, "WARNING: N out of bounds (2)\n");
    }
    if (N >= Nlensing) {
      myAbort ("overflow in init_lensing_links\n");
    }
    N++;
  }
  return (next_lens);
}

/* construct lensing links which are valid FOR THIS LOAD
 * - if we have a full load, we will get links which can
 *   be used by other programs (eg, relphot, etc)
 * - if we have a partial load, the links are only valid
 *   for that partial load
 */ 

off_t *build_lensing_links (Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing) {

  off_t i, m, k, Nm, averef;
  off_t *next_lens;

  ALLOCATE (next_lens, off_t, Nlensing);

  /* reset the Nm, offset values for average */
  for (i = 0; i < Naverage; i++) {
    average[i].lensingOffset = -1;
    average[i].Nlensing     =  0;
  }

  for (Nm = 0; Nm < Nlensing; Nm++) {
    averef = lensing[Nm].averef;
    m = average[averef].lensingOffset;  
    next_lens[Nm] = -1;

    if (m == -1) { /* no links yet for source */
      average[averef].lensingOffset = Nm;
      average[averef].Nlensing     = 1;
      continue;
    }

    for (k = 0; next_lens[m] != -1; k++) {
      m = next_lens[m];
      if (m >= Nlensing) {
	fprintf (stderr, "WARNING: m out of bounds (1)\n");
      }
    }

    average[averef].Nlensing = k + 2;
    next_lens[m] = Nm;
    if (m >= Nlensing) {
      fprintf (stderr, "WARNING: m out of bounds (2)\n");
    }
  }
  return (next_lens);
}

/* average[].lensingOffset, average[].Nlensing are valid within an addstar run */
int add_lens_link (Average *average, off_t *next_lens, off_t Nlensing, off_t NLENSING) {

  off_t k, m;

  /* if we have trouble, check validity of next_lens[m] : m < Nlensing */
  m = average[0].lensingOffset;  

  for (k = 0; k < average[0].Nlensing - 1; k++)  {
    m = next_lens[m];
    if (m >= NLENSING) {
      fprintf (stderr, "WARNING: m out of bounds (3)\n");
    }
  }

  /* set up references */
  next_lens[Nlensing] = -1;
  if (Nlensing >= NLENSING) {
    fprintf (stderr, "WARNING: Nlensing out of bounds (1)\n");
  }

  if (m == -1) {
    average[0].lensingOffset = Nlensing;
  } else {
    next_lens[m] = Nlensing;
    if (m >= NLENSING) {
      fprintf (stderr, "WARNING: m out of bounds (4)\n");
    }
  }

  return (TRUE);
}

Lensing *sort_lensing (Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing, off_t *next_lens) {

  int i, k, n, N;
  Lensing *tmplensing;

  /* fix order of Lensing (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmplensing, Lensing, Nlensing);
  for (i = 0; i < Naverage; i++) {
    if (!average[i].Nlensing) continue;
    n = average[i].lensingOffset;
    average[i].lensingOffset = N;
    for (k = 0; k < average[i].Nlensing; k++, N++) {
      if (n == -1) myAbortF("invalid lensingOffset for ave %d, lensingOffset %d, lensing %d\n", i, average[i].lensingOffset, k);
      tmplensing[N] = lensing[n]; 
      if (lensing[n].averef != i) myAbortF("invalid averef for ave %d, lensingOffset %d, lensing %d, averef %d\n", i, average[i].lensingOffset, k, lensing[n].averef);
      tmplensing[N].averef = i;
      n = next_lens[n];
    }
  }
  free (lensing);
  return (tmplensing);
}


