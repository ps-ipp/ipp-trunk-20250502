# include "photdbc.h"

void unique_measures (Catalog *catalog) {

  int i, j, k, m, N;
  int No, Nm, Nvalid, Nlist;
  int *ilist;
  unsigned int *tlist;
  Measure *mlist, *nmeasure;
  double dR, dD, R, RADIUS2;
  Average *p;

  if (VERBOSE) fprintf (stderr, "removing duplicate measurements\n");

  /* allocate a list for temp storage of measure values */
  Nlist = 0;
  for (i = 0; i < catalog[0].Naverage; i++) {
    Nlist = MAX (Nlist, catalog[0].average[i].Nm);
  }
  ALLOCATE (ilist, int, Nlist);
  ALLOCATE (tlist, unsigned int, Nlist);
  ALLOCATE (mlist, Measure, Nlist);

  RADIUS2 = SQ(UNIQ_RADIUS);
  
  p = catalog[0].average;
  for (i = 0; i < catalog[0].Naverage; i++) {

    if (catalog[0].average != p) {
      fprintf (stderr, "error\n");
    }

    /* extract list of measurements */
    N = 0;
    m = catalog[0].average[i].offset;
    for (j = 0; j < catalog[0].average[i].Nm; j++, m++) {
      ilist[N] = m;
      tlist[N] = catalog[0].measure[m].t;
      N++;
    }

    /* sort the images by time */
    sort_time (tlist, ilist, N);

    /* look for duplicates */
    for (j = k = 0; (j < N) && (k < N); j++) {
      if (j == k) {
	k++;
	continue;
      }
      if (tlist[j] != tlist[k]) {
	j++;
	k = j + 1;
	continue;
      }
      if (catalog[0].measure[ilist[j]].flags & FLAG_DUPMEAS) {
	j++;
	k = j + 1;
	continue;
      }
      
      dR = (catalog[0].measure[ilist[j]].dR - catalog[0].measure[ilist[k]].dR);
      dD = (catalog[0].measure[ilist[j]].dD - catalog[0].measure[ilist[k]].dD);
      R = SQ(dR) + SQ(dD);
      
      /* dR, dD in arcsec, RADIUS is 0.1 arcsec */
      if (R < RADIUS2) {
	/* matched pair */
	catalog[0].measure[ilist[k]].flags |= FLAG_DUPMEAS;
      }
      k++;
    }
  }

  Nvalid = 0;
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    if (!(catalog[0].measure[i].flags & FLAG_DUPMEAS)) Nvalid ++;
  }

  No = 0;
  ALLOCATE (nmeasure, Measure, MAX (Nvalid, 1));
  for (i = 0; i < catalog[0].Naverage; i++) {
    /* copy valid measures to new table, update offset, Nm */
    Nm = 0;
    m = catalog[0].average[i].offset;
    catalog[0].average[i].offset = No;
    for (j = 0; j < catalog[0].average[i].Nm; j++, m++, No++) {
      if (catalog[0].measure[m].flags & FLAG_DUPMEAS) continue;
      nmeasure[No] = catalog[0].measure[m];
      if (catalog[0].measure[m].averef != i) { 
	fprintf (stderr, "average reference mismatch!!\n");
      }
      Nm ++;
    }
    catalog[0].average[i].Nm = Nm;
  }

  free (catalog[0].measure);
  catalog[0].measure = nmeasure;
  catalog[0].Nmeasure = No;

}

/*** XXX 
     this function does not quite do the right thing.  it should remove duplicate measurements 
     for a single average object.  unique measurements have a unique combination of time and photcode
     (IS this true?  only for DET, not for REF...)
***/


