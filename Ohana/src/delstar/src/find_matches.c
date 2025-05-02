# include "delstar.h"

void find_matches (Catalog *catalog, int photcode, e_time start, e_time end) {

  int drop;
  off_t i, j, k, n, m, N, M, averef;
  off_t *next_meas, *next_miss, *ave_miss;
  off_t Nave, Nmeas, NMEAS, Nmiss;
  off_t Nmeasfound, Nsecfilt;
  off_t this, prev;
  Measure *tmpmeasure;
  Missing *tmpmissing;

  /** allocate local arrays **/
  Nave = catalog[0].Naverage;

  Nmeas = catalog[0].Nmeasure;
  ALLOCATE (next_meas, off_t, MAX(Nmeas,1));
  
  Nmiss = catalog[0].Nmissing;
  ALLOCATE (next_miss, off_t, MAX(Nmiss,1));
  ALLOCATE (ave_miss, off_t, MAX(Nmiss,1));
  
  if (VERBOSE) fprintf (stderr, "starting with Nave, Nmeas, Nmiss: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT"\n",  Nave,  Nmeas,  Nmiss);

  /* set up pointers for linked list of measure */
  for (i = 0; i < Nmeas - 1; i++) {
    next_meas[i] = i+1;
  }
  next_meas[i] = -1;
  /* set up pointers for linked list of missing */
  for (i = 0; i < Nmiss - 1; i++) {
    next_miss[i] = i+1;
  }
  next_miss[i] = -1;
  /* set up references for missing to average */
  for (i = 0; i < Nave; i++) {
    for (j = 0; j < catalog[0].average[i].Nmissing; j++) {
      ave_miss[catalog[0].average[i].missingOffset + j] = i;
    }
  }

  if (VERBOSE) fprintf (stderr, "deleting for range %d to %d, photcode %d\n", start, end, photcode);
  Nmeasfound = 0;
  Nsecfilt = catalog[0].Nsecfilt;

  if (VERBOSE) fprintf (stderr, "fixing measure...\n"); 

  /** find measure in time range **/
  this = prev = -1;
  for (i = 0; (i < Nmeas); i++) {
    if (VERBOSE && !(i % 10000)) fprintf (stderr, ". ");
    drop = TRUE;
    drop &= (catalog[0].measure[i].t >= start);
    drop &= (catalog[0].measure[i].t <= end);
    drop &= ((photcode == -1) || (photcode == catalog[0].measure[i].photcode));
    if (!drop) {
      prev = i;
      continue;
    }
    Nmeasfound ++;

    /* this star is in this image */
    this = next_meas[i];
    next_meas[i] = -2; /* we delete this one */
    if (prev != -1) { next_meas[prev] = this; }

# if (0) 
    /* why is this section disabled? */
    /* fix the list links: connect the previous valid link to the next valid link */
    for (j = i; (j >= 0) && (next_meas[j] == -2); j--); /* find previous entry to fix link */
    if (j >= 0) { /* if j < 0, there is no previous valid link, ignore this step */
      if (next_meas[j] != i) {
	fprintf (stderr, "error? (1)  this link seems to have been lost\n");
	fprintf (stderr, "j: "OFF_T_FMT", next_meas[j]: "OFF_T_FMT", i: "OFF_T_FMT"\n",  j,  next_meas[j],  i);
	exit (1);
      }
      /* find next valid entry to fix link */
      for (k = i; (k < Nmeas) && (next_meas[k] == -2); k++);
      if (k < Nmeas)
	next_meas[j] = k;
      else 
	next_meas[j] = -1;  /* last link in list gets a -1 */
    }
# endif      

    /*** fix the corresponding average entry ***/
    n = catalog[0].measure[i].averef;
    if (catalog[0].average[n].Nmeasure == 0) { /* this should never happen */
      fprintf (stderr, "error? we deleted one too many objects?\n");
      exit (1);
    }
    catalog[0].average[n].Nmeasure --;
    /* this was only entry in list: will be deleted below.  meanwhile, delete all missing entries*/
    if ((catalog[0].average[n].Nmeasure < 1) && (catalog[0].average[n].Nmissing > 0)) { 
      m = catalog[0].average[n].missingOffset;
      for (j = 0; j < catalog[0].average[n].Nmissing; j++) {
	M = next_miss[m];
	next_miss[m] = -2;
	m = M;
      }
      m = catalog[0].average[n].missingOffset;
      /* fix the list links: connect the previous valid link to the next valid link */
      for (j = m; (j >= 0) && (next_miss[j] == -2); j--); /* find previous entry to fix link */
      if (j >= 0) { /* if j < 0, there is no previous valid link, ignore this step */
	if (next_miss[j] != m) {
	  fprintf (stderr, "error? (2) this link seems to have been lost\n");
	  fprintf (stderr, "j: "OFF_T_FMT", next_miss[j]: "OFF_T_FMT", i: "OFF_T_FMT"\n",  j,  next_miss[j],  i);
	  exit (1);
	}
	/* find next valid entry to fix link */
	for (k = m; (k < Nmiss) && (next_miss[k] == -2); k++);
	if (k < Nmiss)
	  next_miss[j] = k;
	else 
	  next_miss[j] = -1;  /* last link in list gets a -1 */
      }
    }
    /* this was first entry in list */
    if ((catalog[0].average[n].measureOffset == i) && (catalog[0].average[n].Nmeasure > 0)) { 
      m = catalog[0].average[n].measureOffset;
      /* find next valid entry -- notice lack of error checking... */
      for (j = 0; (j < Nmeas) && (next_meas[m+j] == -2); j++);
      if (catalog[0].measure[m+j].averef != n) {
	fprintf (stderr, "error? measure.averef and average.measureOffset are mismatched\n");
	exit (1);
      }
      catalog[0].average[n].measureOffset = m + j;
    }
  } 
  fprintf (stderr, "found "OFF_T_FMT" meas to remove\n",  Nmeasfound);

  if (VERBOSE) fprintf (stderr, "fixing missing..."); 
  /** find missing in time range of image **/
  for (i = 0; (i < Nmiss); i++) {
    if (next_miss[i] == -2) continue;
    if (catalog[0].missing[i].t < start) continue;
    if (catalog[0].missing[i].t > end) continue;

    next_miss[i] = -2; /* we delete this one */
    /* fix the list links: connect the previous valid link to the next valid link */
    for (j = i; (j >= 0) && (next_miss[j] == -2); j--); /* find previous entry to fix link */
    if (j >= 0) { /* if j < 0, there is no previous valid link, ignore this step */
      if (next_miss[j] != i) {
	fprintf (stderr, "error? (3) this link seems to have been lost\n");
	fprintf (stderr, "j: "OFF_T_FMT", next_miss[j]: "OFF_T_FMT", i: "OFF_T_FMT"\n",  j,  next_miss[j],  i);
	exit (1);
      }
      /* find next valid entry to fix link */
      for (k = i; (k < Nmiss) && (next_miss[k] == -2); k++);
      if (k < Nmiss)
	next_miss[j] = k;
      else 
	next_miss[j] = -1;  /* last link in list gets a -1 */
    }
      
    /*** fix the corresponding average entry ***/
    n = ave_miss[i];
    if (catalog[0].average[n].Nmissing == 0) { /* this should never happen */
      fprintf (stderr, "error? we deleted one too many missing?\n");
      exit (1);
    }
    catalog[0].average[n].Nmissing --;
    /* this was first entry in list */
    if ((catalog[0].average[n].missingOffset == i) && (catalog[0].average[n].Nmissing > 0)) { 
      m = catalog[0].average[n].missingOffset;
      for (j = 0; (j < Nmiss) && (next_miss[m+j] == -2); j++);
      catalog[0].average[n].missingOffset = m + j;
    }
  }

  /* we should delete the references to blended image and cat stars ?? */
  /* or drop since we are changing this concept ?? */

  /* fix Average list: delete entries with Nm == 0 */
  for (i = j = 0; (i < Nave) && (j < Nave); i++, j++) {
    for (; (j < Nave) && (catalog[0].average[j].Nmeasure == 0); j++);
    if ((i != j) && (j < Nave)) {
      catalog[0].average[i] = catalog[0].average[j];
      for (k = 0; k < catalog[0].Nsecfilt; k++) {
	catalog[0].secfilt[i*Nsecfilt + k] = catalog[0].secfilt[j*Nsecfilt + k];
      }
    }    
    if (j == Nave) i--;
  }
  Nave = i;
  REALLOCATE (catalog[0].average, Average, Nave);
  REALLOCATE (catalog[0].secfilt, SecFilt, MAX (1, Nave*Nsecfilt));

  /* fix order of Measure (memory intensive, but fast) */
  N = 0; 
  NMEAS = Nmeas;
  ALLOCATE (tmpmeasure, Measure, NMEAS);
  for (i = 0; i < Nave; i++) {
    n = catalog[0].average[i].measureOffset;
    catalog[0].average[i].measureOffset = N;
    averef = catalog[0].measure[n].averef;
    for (k = 0; k < catalog[0].average[i].Nmeasure; k++, N++) {
      if ((n == -1) || (n == -2)) {
	fprintf (stderr, "error: linked list is confused\n");
	exit (1);
      }
      // all measures for this object should have the same initial averef
      if (catalog[0].measure[n].averef != averef) {
	fprintf (stderr, "measure table is confused\n");
	exit (1);
      }
      tmpmeasure[N] = catalog[0].measure[n]; 
      tmpmeasure[N].averef = i;
      n = next_meas[n];
      CHECK_REALLOCATE (tmpmeasure, Measure, NMEAS, N, 10000);
    }
  }
  Nmeas = N;
  free (catalog[0].measure);
  catalog[0].measure = tmpmeasure;
  REALLOCATE (catalog[0].measure, Measure, Nmeas);
    
  /* fprintf (stderr, "fixing the mising order...\n"); */
  /* fix order of Missing (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmpmissing, Missing, Nmiss);
  for (i = 0; i < Nave; i++) {
    if (catalog[0].average[i].Nmissing > 0) {
      n = catalog[0].average[i].missingOffset;
      catalog[0].average[i].missingOffset = N;
      for (k = 0; k < catalog[0].average[i].Nmissing; k++, N++) {
	if ((n == -1) || (n == -2)) {
	  fprintf (stderr, "error: linked list is confused\n");
	  exit (1);
	}
	tmpmissing[N] = catalog[0].missing[n]; 
	n = next_miss[n];
      }
    }
  }
  Nmiss = N;
  free (catalog[0].missing);
  catalog[0].missing = tmpmissing;
  REALLOCATE (catalog[0].missing, Missing, Nmiss);

  fprintf (stderr, "\n");

  catalog[0].Naverage = Nave;
  catalog[0].Nmeasure = Nmeas;
  catalog[0].Nmissing = Nmiss;
  catalog[0].Nsecfilt_mem = Nave*Nsecfilt;

  if (VERBOSE) fprintf (stderr, "  ending with Nave, Nmeas, Nmiss: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT"\n",  Nave,  Nmeas,  Nmiss);

  free (next_meas);
  free (next_miss);
  free (ave_miss);
  return;
}

