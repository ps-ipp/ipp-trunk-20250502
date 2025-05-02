# include "addstar.h"

void resort_catalog_old (Catalog *catalog) {

  off_t *next_meas;
  off_t Naves, Nmeas;
  double dtime;
  struct timeval start, stop;

  if (catalog[0].sorted == TRUE) return;

  gettimeofday (&start, NULL);

  /* internal counters */
  Nmeas = catalog[0].Nmeasure;
  Naves = catalog[0].Naverage;
  
  /* set up pointers for linked list of measure, missing */
  next_meas = build_measure_links (catalog[0].average, Naves, catalog[0].measure, Nmeas);

  catalog[0].sorted = TRUE;
  catalog[0].measure = sort_measure (catalog[0].average, Naves, catalog[0].measure, Nmeas, next_meas);

  gettimeofday (&stop, NULL);
  dtime = DTIME (stop, start);
  fprintf (stderr, "  match time %9.4f sec for %7lld measures, %6lld average\n", dtime, (long long) Nmeas, (long long) Naves);

  return;
}

# define myAbort(MSG) { fprintf (stderr, "%s\n", MSG); abort(); }
# define myAssert(LOGIC,MSG) { if (!(LOGIC)) { fprintf (stderr, "%s\n", MSG); abort(); } }

// sort the measure Sequence based on the average Sequence entries
void SortAveMeasMatch (off_t *MEAS, off_t *AVE, off_t N) {

# define SWAPFUNC(A,B){ off_t tmp_meas; off_t tmp_ave;	\
    tmp_meas = MEAS[A]; MEAS[A] = MEAS[B]; MEAS[B] = tmp_meas;		\
    tmp_ave  = AVE[A];  AVE[A]  = AVE[B];  AVE[B]  = tmp_ave;		\
  }
# define COMPARE(A,B)(MEAS[A] < MEAS[B])
  OHANA_SORT (N, COMPARE, SWAPFUNC);
# undef SWAPFUNC
# undef COMPARE
}

# define MARKTIME(MSG,...) { \
  float dtime; \
  gettimeofday (&stop, (void *) NULL); \
  dtime = DTIME (stop, start); start = stop; \
  fprintf (stderr, MSG, __VA_ARGS__); }

// XXX : where is the time going?  perhaps the ALLOCATE?
// XXX : I don't thnk his is getting the right answer yet.

static int NMEASURE = 0;
static off_t *measureSeq = NULL;
static off_t *averageSeq = NULL;
static Measure *measureTMP = NULL;

void resort_catalog (Catalog *catalog) {

  off_t Naverage, Nmeasure;
  Measure *measure;
  Average *average;
  off_t i, j, N, currentAve;

  struct timeval start, stop;

  if (catalog[0].sorted == TRUE) return;

  gettimeofday (&start, NULL);

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

  if (!measureSeq) {
    NMEASURE = MAX(Nmeasure, 1000);
    ALLOCATE (measureSeq, off_t,   NMEASURE);
    ALLOCATE (averageSeq, off_t,   NMEASURE);
    ALLOCATE (measureTMP, Measure, NMEASURE);
  }    

  if (Nmeasure > NMEASURE) {
    NMEASURE = Nmeasure;
    REALLOCATE (measureSeq, off_t,   NMEASURE);
    REALLOCATE (averageSeq, off_t,   NMEASURE);
    REALLOCATE (measureTMP, Measure, NMEASURE);
  }
  
  // MARKTIME("array allocation: %f sec\n", dtime);

  for (i = 0; i < Nmeasure; i++) {
    measureSeq[i] = i;
    averageSeq[i] = measure[i].averef;
    
    myAssert(average[averageSeq[i]].objID == measure[measureSeq[i]].objID, "object / detection mismatch");
    myAssert(average[averageSeq[i]].catID == measure[measureSeq[i]].catID, "object / detection mismatch");
  }
  
  // MARKTIME("create index: %f sec\n", dtime);

  SortAveMeasMatch(measureSeq, averageSeq, Nmeasure);
  
  // MARKTIME("sort : %f sec\n", dtime);

  // copy the measurements in the sorted order
  for (i = 0; i < Nmeasure; i++) {
    j = measureSeq[i];
    measureTMP[i] = measure[j];
  }

  // save the sorted list in the correct order
  for (i = 0; i < Nmeasure; i++) {
    measure[i] = measureTMP[i];
  }

  // MARKTIME("assign measure : %f sec\n", dtime);

  // update the values of average.measureOffset and average.Nmeasure

  N = 0;
  currentAve = averageSeq[0];
  average[currentAve].measureOffset = 0;
  for (i = 0; i < Nmeasure; i++) {
    if (averageSeq[i] != currentAve) {
      average[currentAve].Nmeasure = N;
      N = 0;
      currentAve = averageSeq[i];
      average[currentAve].measureOffset = i;
    }
    N++;
  }
  N++;
  average[currentAve].Nmeasure = N;

  // MARKTIME("update Nmeasure : %f sec\n", dtime);

  MARKTIME("  match time %9.4f sec for %7lld measures, %6lld average\n", dtime, (long long) Nmeasure, (long long) Naverage);

  // FREE (measureSeq);
  // FREE (averageSeq);

  return;
}

