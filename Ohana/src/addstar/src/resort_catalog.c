# include "addstar.h"

void SortAveMatch (off_t *MEAS, off_t *AVE, off_t N);

void resort_catalog (Catalog *catalog) {

  if (catalog[0].sorted == TRUE) return;

  resort_catalog_measure (catalog);
  resort_catalog_lensing (catalog);
  resort_catalog_starpar (catalog);
  resort_catalog_galphot (catalog);
  catalog[0].sorted = TRUE;
}

void resort_catalog_measure (Catalog *catalog) {

  off_t Naverage, Nmeasure;
  Measure *measure;
  Average *average;
  off_t i, j, N, currentAve;

  off_t *measureSeq = NULL;
  off_t *averageSeq = NULL;
  Measure *measureTMP = NULL;

  // struct timeval start, stop;
  // gettimeofday (&start, NULL);

  /* internal counters */
  Nmeasure = catalog[0].Nmeasure;
  Naverage = catalog[0].Naverage;

  if (!Nmeasure) return;

  measure = catalog[0].measure;
  average = catalog[0].average;
  
  // we have a table of average objects and an unsorted table of measurements.  each measurement
  // has a reference to the average object sequence (as well as an ID)
  // measure[i].averef -> average[averef]
  // measure[i].objID = average[averef].objID
  // measure[i].catID = average[averef].catID

  // we want a sorted measure array with all averef entries in sequence

  ALLOCATE (measureSeq, off_t,   Nmeasure);
  ALLOCATE (averageSeq, off_t,   Nmeasure);

  for (i = 0; i < Nmeasure; i++) {
    measureSeq[i] = i;
    averageSeq[i] = measure[i].averef;
    
    if (catalog[0].catformat >= DVO_FORMAT_PS1_V1) {
      // earlier formats did not carry the objID or catID, so they are not available (we could assign on load, but we don't)
      myAssert(average[averageSeq[i]].catID == measure[measureSeq[i]].catID, "object / detection mismatch");
      myAssert (average[averageSeq[i]].objID == measure[measureSeq[i]].objID, "object ID mismatch?");
# if (0)
      // myAssert(average[averageSeq[i]].objID == measure[measureSeq[i]].objID, "object / detection mismatch");

      // Check some possible causes for objID failures
      if (average[averageSeq[i]].objID != measure[measureSeq[i]].objID) {
	fprintf (stderr, "object / detection mismatch average.objID = %d, measure.objID = %d, catID: %d, detID: %d", 
		 average[averageSeq[i]].objID, measure[measureSeq[i]].objID, 
		 measure[measureSeq[i]].catID, measure[measureSeq[i]].detID);
	// is the byte-swapped value the correct objID?
	int objIDalt = measure[measureSeq[i]].objID;
	char *byte = (char *) &objIDalt;
	char tmp;
	tmp = byte[0]; byte[0] = byte[3]; byte[3] = tmp;
	tmp = byte[1]; byte[1] = byte[2]; byte[2] = tmp;
	if (average[averageSeq[i]].objID == objIDalt) {
	  myAbort ("measure.objID is byte-swapped, consider repairing\n");
	  // measure[measureSeq[i]].objID = average[averageSeq[i]].objID; // XXX I don't really like this...
	} 
	if (measure[measureSeq[i]].objID == 0) {
	  myAbort ("measure.objID is 0, repairing\n");
	  // measure[measureSeq[i]].objID = average[averageSeq[i]].objID; // XXX I don't really like this...
	} else {
	  myAbort ("objID is NOT byte-swapped and NOT 0, aborting\n");
	}
      }
# endif
# if (0)
      // for reasons I do not understand, the mini dvodbs generated on stsci1X had a handful of detections with an inconsistency between averef and objID.  
      // this happened for 28 detections in the dbs on /data/stsci1?.0/eugene/dvo3pi.20130616, but not at all (as far as I know) in the rest of LAP DVO
      if (average[averageSeq[i]].objID != measure[measureSeq[i]].objID) {
	fprintf (stderr, "R");
	measure[measureSeq[i]].objID = average[averageSeq[i]].objID; // XXX I don't really like this...
      }
# endif
    }
  }
  
  // check that averageSeq is now in order
  // for (i = 1; i < Nmeasure; i++) {
  //   if (averageSeq[i] < averageSeq[i-1]) {
  //     fprintf (stderr, "%d ", (int) i);
  //   }
  // }
  // fprintf (stderr, "\n");

  SortAveMatch(measureSeq, averageSeq, Nmeasure);
  // MARKTIME("sort : %f sec\n", dtime);

  // check that averageSeq is now in order
  // for (i = 1; i < Nmeasure; i++) {
  //   if (averageSeq[i] < averageSeq[i-1]) {
  //     fprintf (stderr, "%d ", (int) i);
  //   }
  // }
  // fprintf (stderr, "\n");

  // copy the measurements in the sorted order
  ALLOCATE (measureTMP, Measure, Nmeasure);
  for (i = 0; i < Nmeasure; i++) {
    j = measureSeq[i];
    measureTMP[i] = measure[j];
  }
  // MARKTIME("assign measure : %f sec\n", dtime);

  // update the values of average.measureOffset and average.Nmeasure
  FREE(measure);
  catalog[0].measure = measureTMP;

  N = 0;
  currentAve = averageSeq[0];
  average[currentAve].measureOffset = 0;
  for (i = 0; i < Nmeasure; i++) {
    if (averageSeq[i] != currentAve) {
      // we have hit the next entry in the list
      average[currentAve].Nmeasure = N;
      N = 0;
      currentAve = averageSeq[i];
      average[currentAve].measureOffset = i;
    }
    N++;
  }
  // N++;
  average[currentAve].Nmeasure = N;
  // MARKTIME("update Nmeasure : %f sec\n", dtime);

  int NmeasureTotal = 0;
  int measureOffsetOK = TRUE;
  for (i = 0; i < Naverage; i++) {
    NmeasureTotal += catalog[0].average[i].Nmeasure;
    if (VERBOSE && !(NmeasureTotal <= catalog[0].Nmeasure)) {
      fprintf (stderr, "too few measurements: %d %d %d\n", (int) i, NmeasureTotal, (int) catalog[0].Nmeasure);
    }
    measureOffsetOK &= (catalog[0].average[i].measureOffset < catalog[0].Nmeasure);
    if (VERBOSE && !(catalog[0].average[i].measureOffset < catalog[0].Nmeasure)) {
      fprintf (stderr, "offset too large: %d %d %d\n", (int) i, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
    }
    measureOffsetOK &= (catalog[0].average[i].measureOffset + catalog[0].average[i].Nmeasure <= catalog[0].Nmeasure);
    if (VERBOSE && !(catalog[0].average[i].measureOffset + catalog[0].average[i].Nmeasure <= catalog[0].Nmeasure)) {
      fprintf (stderr, "orrset + Nmeasure too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].measureOffset, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
    }
  }

  if (!measureOffsetOK) {
    fprintf (stderr, "ERROR: catalog %s has an invalid measureOffset\n", catalog[0].filename);
  }

  if (NmeasureTotal != catalog[0].Nmeasure) {
    fprintf (stderr, "ERROR: catalog %s has an invalid Nmeasure\n", catalog[0].filename);
  }

  // MARKTIME("  match time %9.4f sec for %7lld measures, %6lld average\n", dtime, (long long) Nmeasure, (long long) Naverage);

  FREE (measureSeq);
  FREE (averageSeq);

  return;
}

void resort_catalog_lensing (Catalog *catalog) {

  off_t Naverage, Nlensing;
  Lensing *lensing;
  Average *average;
  off_t i, j, N, currentAve;

  off_t *lensingSeq = NULL;
  off_t *averageSeq = NULL;
  Lensing *lensingTMP = NULL;

  // struct timeval start, stop;
  // gettimeofday (&start, NULL);

  /* internal counters */
  Nlensing = catalog[0].Nlensing;
  Naverage = catalog[0].Naverage;

  if (!Nlensing) return;

  lensing = catalog[0].lensing;
  average = catalog[0].average;
  
  // we have a table of average objects and an unsorted table of measurements.  each measurement
  // has a reference to the average object sequence (as well as an ID)
  // lensing[i].averef -> average[averef]
  // lensing[i].objID = average[averef].objID
  // lensing[i].catID = average[averef].catID

  // we want a sorted lensing array with all averef entries in sequence

  ALLOCATE (lensingSeq, off_t,   Nlensing);
  ALLOCATE (averageSeq, off_t,   Nlensing);

  for (i = 0; i < Nlensing; i++) {
    lensingSeq[i] = i;
    averageSeq[i] = lensing[i].averef;
    
    if (catalog[0].catformat >= DVO_FORMAT_PS1_V1) {
      // earlier formats did not carry the objID or catID, so they are not available (we could assign on load, but we don't)
      myAssert(average[averageSeq[i]].catID == lensing[lensingSeq[i]].catID, "object / detection mismatch");
      myAssert(average[averageSeq[i]].objID == lensing[lensingSeq[i]].objID, "object / detection mismatch");
    }
  }
  
  SortAveMatch(lensingSeq, averageSeq, Nlensing);

  // copy the lensing entries in the sorted order
  ALLOCATE (lensingTMP, Lensing, Nlensing);
  for (i = 0; i < Nlensing; i++) {
    j = lensingSeq[i];
    lensingTMP[i] = lensing[j];
  }

  // update the values of average.lensingOffset and average.Nlensing
  FREE(lensing);
  catalog[0].lensing = lensingTMP;

  N = 0;
  currentAve = averageSeq[0];
  average[currentAve].lensingOffset = 0;
  for (i = 0; i < Nlensing; i++) {
    if (averageSeq[i] != currentAve) {
      // we have hit the next entry in the list
      average[currentAve].Nlensing = N;
      N = 0;
      currentAve = averageSeq[i];
      average[currentAve].lensingOffset = i;
    }
    N++;
  }
  average[currentAve].Nlensing = N;

  int NlensingTotal = 0;
  int lensingOffsetOK = TRUE;
  for (i = 0; i < Naverage; i++) {
    NlensingTotal += catalog[0].average[i].Nlensing;
    if (VERBOSE && !(NlensingTotal <= catalog[0].Nlensing)) {
      fprintf (stderr, "too few lensing: %d %d %d\n", (int) i, NlensingTotal, (int) catalog[0].Nlensing);
    }
    lensingOffsetOK &= (catalog[0].average[i].lensingOffset < catalog[0].Nlensing);
    if (VERBOSE && !(catalog[0].average[i].lensingOffset < catalog[0].Nlensing)) {
      fprintf (stderr, "offset too large: %d %d %d\n", (int) i, catalog[0].average[i].Nlensing, (int) catalog[0].Nlensing);
    }
    lensingOffsetOK &= (catalog[0].average[i].lensingOffset + catalog[0].average[i].Nlensing <= catalog[0].Nlensing);
    if (VERBOSE && !(catalog[0].average[i].lensingOffset + catalog[0].average[i].Nlensing <= catalog[0].Nlensing)) {
      fprintf (stderr, "orrset + Nlensing too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].lensingOffset, catalog[0].average[i].Nlensing, (int) catalog[0].Nlensing);
    }
  }

  if (!lensingOffsetOK) {
    fprintf (stderr, "ERROR: catalog %s has an invalid lensingOffset\n", catalog[0].filename);
  }

  if (NlensingTotal != catalog[0].Nlensing) {
    fprintf (stderr, "ERROR: catalog %s has an invalid Nlensing\n", catalog[0].filename);
  }

  FREE (lensingSeq);
  FREE (averageSeq);

  return;
}

void resort_catalog_starpar (Catalog *catalog) {

  off_t Naverage, Nstarpar;
  StarPar *starpar;
  Average *average;
  off_t i, j, N, currentAve;

  off_t *starparSeq = NULL;
  off_t *averageSeq = NULL;
  StarPar *starparTMP = NULL;

  // struct timeval start, stop;
  // gettimeofday (&start, NULL);

  /* internal counters */
  Nstarpar = catalog[0].Nstarpar;
  Naverage = catalog[0].Naverage;

  if (!Nstarpar) return;

  starpar = catalog[0].starpar;
  average = catalog[0].average;
  
  // we have a table of average objects and an unsorted table of measurements.  each measurement
  // has a reference to the average object sequence (as well as an ID)
  // starpar[i].averef -> average[averef]
  // starpar[i].objID = average[averef].objID
  // starpar[i].catID = average[averef].catID

  // we want a sorted starpar array with all averef entries in sequence

  ALLOCATE (starparSeq, off_t,   Nstarpar);
  ALLOCATE (averageSeq, off_t,   Nstarpar);

  for (i = 0; i < Nstarpar; i++) {
    starparSeq[i] = i;
    averageSeq[i] = starpar[i].averef;
    
    if (catalog[0].catformat >= DVO_FORMAT_PS1_V1) {
      // earlier formats did not carry the objID or catID, so they are not available (we could assign on load, but we don't)
      myAssert(average[averageSeq[i]].catID == starpar[starparSeq[i]].catID, "object / detection mismatch");
      myAssert(average[averageSeq[i]].objID == starpar[starparSeq[i]].objID, "object / detection mismatch");
    }
  }
  
  SortAveMatch(starparSeq, averageSeq, Nstarpar);

  // copy the starpar entries in the sorted order
  ALLOCATE (starparTMP, StarPar, Nstarpar);
  for (i = 0; i < Nstarpar; i++) {
    j = starparSeq[i];
    starparTMP[i] = starpar[j];
  }

  // update the values of average.starparOffset and average.Nstarpar
  FREE(starpar);
  catalog[0].starpar = starparTMP;

  N = 0;
  currentAve = averageSeq[0];
  average[currentAve].starparOffset = 0;
  for (i = 0; i < Nstarpar; i++) {
    if (averageSeq[i] != currentAve) {
      // we have hit the next entry in the list
      average[currentAve].Nstarpar = N;
      N = 0;
      currentAve = averageSeq[i];
      average[currentAve].starparOffset = i;
    }
    N++;
  }
  average[currentAve].Nstarpar = N;

  int NstarparTotal = 0;
  int starparOffsetOK = TRUE;
  for (i = 0; i < Naverage; i++) {
    NstarparTotal += catalog[0].average[i].Nstarpar;
    if (VERBOSE && !(NstarparTotal <= catalog[0].Nstarpar)) {
      fprintf (stderr, "too few starpar: %d %d %d\n", (int) i, NstarparTotal, (int) catalog[0].Nstarpar);
    }
    starparOffsetOK &= (catalog[0].average[i].starparOffset < catalog[0].Nstarpar);
    if (VERBOSE && !(catalog[0].average[i].starparOffset < catalog[0].Nstarpar)) {
      fprintf (stderr, "offset too large: %d %d %d\n", (int) i, catalog[0].average[i].Nstarpar, (int) catalog[0].Nstarpar);
    }
    starparOffsetOK &= (catalog[0].average[i].starparOffset + catalog[0].average[i].Nstarpar <= catalog[0].Nstarpar);
    if (VERBOSE && !(catalog[0].average[i].starparOffset + catalog[0].average[i].Nstarpar <= catalog[0].Nstarpar)) {
      fprintf (stderr, "orrset + Nstarpar too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].starparOffset, catalog[0].average[i].Nstarpar, (int) catalog[0].Nstarpar);
    }
  }

  if (!starparOffsetOK) {
    fprintf (stderr, "ERROR: catalog %s has an invalid starparOffset\n", catalog[0].filename);
  }

  if (NstarparTotal != catalog[0].Nstarpar) {
    fprintf (stderr, "ERROR: catalog %s has an invalid Nstarpar\n", catalog[0].filename);
  }

  FREE (starparSeq);
  FREE (averageSeq);

  return;
}

void resort_catalog_galphot (Catalog *catalog) {

  off_t Naverage, Ngalphot;
  GalPhot *galphot;
  Average *average;
  off_t i, j, N, currentAve;

  off_t *galphotSeq = NULL;
  off_t *averageSeq = NULL;
  GalPhot *galphotTMP = NULL;

  // struct timeval start, stop;
  // gettimeofday (&start, NULL);

  /* internal counters */
  Ngalphot = catalog[0].Ngalphot;
  Naverage = catalog[0].Naverage;

  if (!Ngalphot) return;

  galphot = catalog[0].galphot;
  average = catalog[0].average;
  
  // we have a table of average objects and an unsorted table of measurements.  each measurement
  // has a reference to the average object sequence (as well as an ID)
  // galphot[i].averef -> average[averef]
  // galphot[i].objID = average[averef].objID
  // galphot[i].catID = average[averef].catID

  // we want a sorted galphot array with all averef entries in sequence

  ALLOCATE (galphotSeq, off_t,   Ngalphot);
  ALLOCATE (averageSeq, off_t,   Ngalphot);

  for (i = 0; i < Ngalphot; i++) {
    galphotSeq[i] = i;
    averageSeq[i] = galphot[i].averef;
    
    if (catalog[0].catformat >= DVO_FORMAT_PS1_V1) {
      // earlier formats did not carry the objID or catID, so they are not available (we could assign on load, but we don't)
      myAssert(average[averageSeq[i]].catID == galphot[galphotSeq[i]].catID, "object / detection mismatch");
      myAssert(average[averageSeq[i]].objID == galphot[galphotSeq[i]].objID, "object / detection mismatch");
    }
  }
  
  SortAveMatch(galphotSeq, averageSeq, Ngalphot);

  // copy the galphot entries in the sorted order
  ALLOCATE (galphotTMP, GalPhot, Ngalphot);
  for (i = 0; i < Ngalphot; i++) {
    j = galphotSeq[i];
    galphotTMP[i] = galphot[j];
  }

  // update the values of average.galphotOffset and average.Ngalphot
  FREE(galphot);
  catalog[0].galphot = galphotTMP;

  N = 0;
  currentAve = averageSeq[0];
  average[currentAve].galphotOffset = 0;
  for (i = 0; i < Ngalphot; i++) {
    if (averageSeq[i] != currentAve) {
      // we have hit the next entry in the list
      average[currentAve].Ngalphot = N;
      N = 0;
      currentAve = averageSeq[i];
      average[currentAve].galphotOffset = i;
    }
    N++;
  }
  average[currentAve].Ngalphot = N;

  int NgalphotTotal = 0;
  int galphotOffsetOK = TRUE;
  for (i = 0; i < Naverage; i++) {
    NgalphotTotal += catalog[0].average[i].Ngalphot;
    if (VERBOSE && !(NgalphotTotal <= catalog[0].Ngalphot)) {
      fprintf (stderr, "too few galphot: %d %d %d\n", (int) i, NgalphotTotal, (int) catalog[0].Ngalphot);
    }
    galphotOffsetOK &= (catalog[0].average[i].galphotOffset < catalog[0].Ngalphot);
    if (VERBOSE && !(catalog[0].average[i].galphotOffset < catalog[0].Ngalphot)) {
      fprintf (stderr, "offset too large: %d %d %d\n", (int) i, catalog[0].average[i].Ngalphot, (int) catalog[0].Ngalphot);
    }
    galphotOffsetOK &= (catalog[0].average[i].galphotOffset + catalog[0].average[i].Ngalphot <= catalog[0].Ngalphot);
    if (VERBOSE && !(catalog[0].average[i].galphotOffset + catalog[0].average[i].Ngalphot <= catalog[0].Ngalphot)) {
      fprintf (stderr, "orrset + Ngalphot too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].galphotOffset, catalog[0].average[i].Ngalphot, (int) catalog[0].Ngalphot);
    }
  }

  if (!galphotOffsetOK) {
    fprintf (stderr, "ERROR: catalog %s has an invalid galphotOffset\n", catalog[0].filename);
  }

  if (NgalphotTotal != catalog[0].Ngalphot) {
    fprintf (stderr, "ERROR: catalog %s has an invalid Ngalphot\n", catalog[0].filename);
  }

  FREE (galphotSeq);
  FREE (averageSeq);

  return;
}

// sort the measure or lensing Sequence based on the average Sequence entries
void SortAveMatch (off_t *MEAS, off_t *AVE, off_t N) {

# define SWAPFUNC(A,B){ off_t tmp_meas; off_t tmp_ave;		\
    tmp_meas = MEAS[A]; MEAS[A] = MEAS[B]; MEAS[B] = tmp_meas;	\
    tmp_ave  = AVE[A];  AVE[A]  = AVE[B];  AVE[B]  = tmp_ave;	\
  }
# define COMPARE(A,B)(AVE[A] < AVE[B])
  OHANA_SORT (N, COMPARE, SWAPFUNC);
# undef SWAPFUNC
# undef COMPARE
}

