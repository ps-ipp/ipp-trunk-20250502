# include "delstar.h"

delete_orphans (char *name) {

  off_t i, j, k, n, m, Nmeasfound, Nsecfilt, N, M, found;
  off_t *next_meas, *next_miss, *ave_miss, last, last_miss;
  off_t Nave, NAVE, Nmeas, NMEAS, Nmiss, NMISS, Nmatch;
  e_time start, end;
  unsigned int flags;
  Measure *tmpmeasure;
  Missing *tmpmissing;
  Coords tcoords;
  Catalog catalog;

  /* find and load catalog file */
  catalog.filename = name;
  dvo_catalog_load (&catalog);
  gcatstats (&catalog, &catstats);

  /* find images overlapping catalog */
  image = find_images_region (&catstats, &Nimage);
  match_images (&catalog, image, Nimage);

  /** allocate local arrays **/
  Nave = catalog.Naverage;

  Nmeas = catalog.Nmeasure;
  ALLOCATE (next_meas, int, Nmeas);
  
  Nmiss = catalog.Nmissing;
  ALLOCATE (next_miss, int, Nmiss);
  ALLOCATE (ave_miss, int, Nmiss);
  
  if (VERBOSE) fprintf (stderr, "starting with Nave, Nmeas, Nmiss: %d %d %d\n", Nave, Nmeas, Nmiss);

  /* set up pointers for linked list of measure */
  for (i = 0; i < Nmeas - 1; i++) {
    next_meas[i] = i+1;
  }
  next_meas[i] = -1;
  last = i;
  /* set up pointers for linked list of missing */
  for (i = 0; i < Nmiss - 1; i++) {
    next_miss[i] = i+1;
  }
  next_miss[i] = -1;
  last_miss = i;
  /* set up references for missing to average */
  for (i = 0; i < Nave; i++) {
    for (j = 0; j < catalog.average[i].Nn; j++) {
      ave_miss[catalog.average[i].missing + j] = i;
    }
  }
  Nmeasfound = 0;
  Nsecfilt = catalog.Nsecfilt;

  /* fprintf (stderr, "fixing the measures...\n"); */
  for (i = 0; (i < Nmeas); i++) {
    if ((catalog.measure[i].t != 0) && (catalog.image[i] == -1)) { 
      /* this star is an orphan */
      Nmeasfound ++;
      next_meas[i] = -2; /* we delete this one */
      /* fix the list links: connect the previous valid link to the next valid link */
      for (j = i; (j >= 0) && (next_meas[j] == -2); j--); /* find previous entry to fix link */
      if (j >= 0) { /* if j < 0, there is no previous valid link, ignore this step */
	if (next_meas[j] != i) {
	  fprintf (stderr, "error?  this link seems to have been lost\n");
	  exit (1);
	}
	/* find next valid entry to fix link */
	for (k = i; (k < Nmeas) && (next_meas[k] == -2); k++);
	if (k < Nmeas)
	  next_meas[j] = k;
	else 
	  next_meas[j] = -1;  /* last link in list gets a -1 */
      }
      
      /*** fix the corresponding average entry ***/
      n = catalog.measure[i].averef;
      if (catalog.average[n].Nm == 0) { /* this should never happen */
	fprintf (stderr, "error? we deleted one too many objects?\n");
	exit (1);
      }
      catalog.average[n].Nm --;
      /* this was only entry in list: will be deleted below.  meanwhile, delete all missing entries*/
      if ((catalog.average[n].Nm < 1) && (catalog.average[n].Nn > 0)) { 
	m = catalog.average[n].missing;
	for (j = 0; j < catalog.average[n].Nn; j++) {
	  M = next_miss[m];
	  next_miss[m] = -2;
	  m = M;
	}
	m = catalog.average[n].missing;
	/* fix the list links: connect the previous valid link to the next valid link */
	for (j = m; (j >= 0) && (next_miss[j] == -2); j--); /* find previous entry to fix link */
	if (j >= 0) { /* if j < 0, there is no previous valid link, ignore this step */
	  if (next_miss[j] != m) {
	    fprintf (stderr, "error?  this link seems to have been lost\n");
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
      if ((catalog.average[n].offset == i) && (catalog.average[n].Nm > 0)) { 
	m = catalog.average[n].offset;
	/* find next valid entry -- notice lack of error checking... */
	for (j = 0; (j < Nmeas) && (next_meas[m+j] == -2); j++);
	catalog.average[n].offset = m + j;
      }

    }
  }
  fprintf (stderr, "found %d meas to remove\n", Nmeasfound);

  /* fprintf (stderr, "fixing the missing...\n"); */
  /** find missing in time range of image **/
  for (i = 0; (i < Nmiss); i++) {
    if ((next_miss[i] != -2) && (catalog.missing[i].t >= start) && (catalog.missing[i].t <= end)) { 
      /* this star is in this image */

      next_miss[i] = -2; /* we delete this one */
      /* fix the list links: connect the previous valid link to the next valid link */
      for (j = i; (j >= 0) && (next_miss[j] == -2); j--); /* find previous entry to fix link */
      if (j >= 0) { /* if j < 0, there is no previous valid link, ignore this step */
	if (next_miss[j] != i) {
	  fprintf (stderr, "error?  this link seems to have been lost\n");
	  exit (1);
	}
	/* find next valid entry to fix link */
	for (k = i; (k < Nmiss) && (next_miss[k] == -2); k++);
	if (k < Nmiss)
	  next_miss[j] = k;
	else 
	  next_miss[j] = -1;  /* last link in list gets a -1 */
      }
      
      /* find the corresponding avearge entry 
      found = FALSE;
      for (n = 0; !found && (n < Nave); n++) {
	if ((catalog.average[n].missing > 0) && (catalog.average[n].missing <= i) && (catalog.average[n].missing + catalog.average[n].Nn > i)) 
	  found = TRUE;
      }
      n--; */
      /*** fix the corresponding average entry ***/
      n = ave_miss[i];
      if (catalog.average[n].Nn == 0) { /* this should never happen */
	fprintf (stderr, "error? we deleted one too many missing?\n");
	exit (1);
      }
      catalog.average[n].Nn --;
      /* this was first entry in list */
      if ((catalog.average[n].missing == i) && (catalog.average[n].Nn > 0)) { 
	m = catalog.average[n].missing;
	for (j = 0; (j < Nmiss) && (next_miss[m+j] == -2); j++);
	catalog.average[n].missing = m + j;
      }

    }
  }

  /* currently not worked out, but we will need to delete the references to blended image and cat stars */
# if 0
  /*** handle multiple stars */
  /* this image star matches more than one catalog star */
  if (stars[N].found > -1) {
    catalog.measure[stars[N].found].flags |= ID_MEAS_BLEND_MEAS;
    catalog.measure[Nmeas].flags |= ID_MEAS_BLEND_MEAS;
  } 
  if (stars[N].found == -2) { /* this image star matches a catalog star on a neighboring catalog */
    catalog.measure[Nmeas].flags |= ID_MEAS_BLEND_MEAS_X;
  } 
  if (stars[N].found == -1) { /* this image star matches only this star */
    stars[N].found = Nmeas;  /* save first match, in case coincidences are found */
  }
  /* this catalog star matches more than one image star */
  if (catalog.found[n] > -1) {
    catalog.measure[catalog.found[n]].flags |= ID_MEAS_BLEND_OBJ;
    catalog.measure[Nmeas].flags |= ID_MEAS_BLEND_OBJ;
  } else {
    catalog.found[n] = Nmeas;
  }
# endif  

  /* fprintf (stderr, "fixing the averages...\n"); */
  /* fix Average list: delete entries with Nm == 0 */
  for (i = j = 0; (i < Nave) && (j < Nave); i++, j++) {
    for (; (j < Nave) && (catalog.average[j].Nm == 0); j++);
    if ((i != j) && (j < Nave)) {
      catalog.average[i] = catalog.average[j];
      for (k = 0; k < catalog.Nsecfilt; k++) {
	catalog.secfilt[i*Nsecfilt + k] = catalog.secfilt[j*Nsecfilt + k];
      }
    }
    if (j == Nave) i--;
  }
  Nave = i;
  REALLOCATE (catalog.average, Average, Nave);
  REALLOCATE (catalog.secfilt, SecFilt, MAX (1, Nave*Nsecfilt));
  
  /* fprintf (stderr, "fixing the measure order...\n"); */
  /* fix order of Measure (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmpmeasure, Measure, Nmeas);
  for (i = 0; i < Nave; i++) {
    n = catalog.average[i].offset;
    catalog.average[i].offset = N;
    for (k = 0; k < catalog.average[i].Nm; k++, N++) {
      if ((n == -1) || (n == -2)) {
	fprintf (stderr, "error: linked list is confused\n");
	exit (1);
      }
      tmpmeasure[N] = catalog.measure[n]; 
      tmpmeasure[N].averef = i;
      n = next_meas[n];
    }
  }
  Nmeas = N;
  free (catalog.measure);
  catalog.measure = tmpmeasure;
  REALLOCATE (catalog.measure, Measure, Nmeas);
    
  /* fprintf (stderr, "fixing the mising order...\n"); */
  /* fix order of Missing (memory intensive, but fast) */
  N = 0; 
  ALLOCATE (tmpmissing, Missing, Nmiss);
  for (i = 0; i < Nave; i++) {
    if (catalog.average[i].Nn > 0) {
      n = catalog.average[i].missing;
      catalog.average[i].missing = N;
      for (k = 0; k < catalog.average[i].Nn; k++, N++) {
	if ((n == -1) || (n == -2)) {
	  fprintf (stderr, "error: linked list is confused\n");
	  exit (1);
	}
	tmpmissing[N] = catalog.missing[n]; 
	n = next_miss[n];
      }
    }
  }
  Nmiss = N;
  free (catalog.missing);
  catalog.missing = tmpmissing;
  REALLOCATE (catalog.missing, Missing, Nmiss);

  catalog.Naverage = Nave;
  catalog.Nmeasure = Nmeas;
  catalog.Nmissing = Nmiss;
  catalog.Nsecfilt_mem = Nave*Nsecfilt;

  if (VERBOSE) fprintf (stderr, "  ending with Nave, Nmeas, Nmiss: %d %d %d\n", Nave, Nmeas, Nmiss);

}
