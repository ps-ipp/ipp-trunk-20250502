# include "relastro.h"

// sort the measure Sequence based on the average Sequence entries
void SortAveMeasMatch (off_t *MEAS, off_t *AVE, off_t N) {

# define SWAPFUNC(A,B){ off_t tmp_meas; off_t tmp_ave;	\
    tmp_meas = MEAS[A]; MEAS[A] = MEAS[B]; MEAS[B] = tmp_meas;		\
    tmp_ave  = AVE[A];  AVE[A]  = AVE[B];  AVE[B]  = tmp_ave;		\
  }
# define COMPARE(A,B)(AVE[A] < AVE[B])
  OHANA_SORT (N, COMPARE, SWAPFUNC);
# undef SWAPFUNC
# undef COMPARE
}

// XXX : where is the time going?  perhaps the ALLOCATE?
// XXX : I don't thnk his is getting the right answer yet.

void resort_catalog (Catalog *catalog) {

  off_t Naverage, Nmeasure;
  Measure *measure;
  Average *average;
  off_t i, j, N, currentAve;

  off_t *measureSeq = NULL;
  off_t *averageSeq = NULL;
  Measure *measureTMP = NULL;

  if (catalog[0].sorted == TRUE) return;

  // INITTIME;

  /* internal counters */
  Nmeasure = catalog[0].Nmeasure;
  Naverage = catalog[0].Naverage;

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
      myAssert(average[averageSeq[i]].objID == measure[measureSeq[i]].objID, "object / detection mismatch");
      myAssert(average[averageSeq[i]].catID == measure[measureSeq[i]].catID, "object / detection mismatch");
    }
  }
  
  // check that averageSeq is now in order
  // for (i = 1; i < Nmeasure; i++) {
  //   if (averageSeq[i] < averageSeq[i-1]) {
  //     fprintf (stderr, "%d ", (int) i);
  //   }
  // }
  // fprintf (stderr, "\n");

  SortAveMeasMatch(measureSeq, averageSeq, Nmeasure);
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

  catalog[0].sorted = TRUE;

  FREE (measureSeq);
  FREE (averageSeq);

  return;
}

