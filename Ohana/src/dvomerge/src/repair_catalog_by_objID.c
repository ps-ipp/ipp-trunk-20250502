# include "dvomerge.h"

int repair_catalog_by_objID (Catalog *catalog) {

  // first find the entries which are bad (there may be none)
  int Nbad = 0;

  Average *average = catalog->average;
  Measure *measure = catalog->measure;

  off_t i, j;
  for (i = 0; i < catalog->Naverage; i++) {
    off_t m = average[i].measureOffset;
    for (j = 0; j < average[i].Nmeasure; j++) {
      if (average[i].objID == measure[m+j].objID) continue;
      
      // this is a bad entry.  now we need to decide if it can be repaired
      
      Measure *myMeasure = &measure[m+j];
      int iTest = myMeasure->objID;
      myAssert (average[iTest].objID == iTest, "cannot find target source");
      
      double dR = 3600.0*(average[iTest].R - myMeasure->R) * cos (RAD_DEG*myMeasure->D);
      double dD = 3600.0*(average[iTest].D - myMeasure->D);
      double dRad = hypot (dR, dD);

      // XXX dRad of 200.0 would allow Barnard's star (10"/yr) to be handled
      myAssert (dRad < 200.0, "bad detection does not seem to match target source");
      
      char *date = ohana_sec_to_date (myMeasure->t);
      fprintf (stderr, "bad detection found: matches averef %d, objID %d, %s : %6.3f %5d : %f, %f = %f\n", iTest, myMeasure->objID, date, myMeasure->M, myMeasure->photcode, dR, dD, dRad);
      free (date);
      Nbad ++;
    }
  }
  if (!Nbad) return TRUE;

  // we have at least one bad detection
  // make a matched list of the measurement sequence and the objID values

  off_t Nmeasure = catalog->Nmeasure;
  ALLOCATE_PTR (seq,   off_t, Nmeasure);
  ALLOCATE_PTR (objID, off_t, Nmeasure);

  for (i = 0; i < Nmeasure; i++) {
    seq[i]   = i;
    objID[i] = measure[i].objID;
  }    

  // sort seq by objID
  llsortpair (objID, seq, Nmeasure);

  // create a new array of measures in objID order
  Measure *measureNew = NULL;
  ALLOCATE (measureNew, Measure, Nmeasure);
  
  for (i = 0; i < Nmeasure; i++) {
    j = seq[i];
    measureNew[i] = measure[j];
  }

  // reset averef and Nmeasure values for the average entries
  off_t N = 0;
  off_t myObjID = objID[0];
  off_t averef = 0;
  average[averef].measureOffset = 0;
  for (i = 0; i < Nmeasure; i++) {
    if (objID[i] != myObjID) {
      // we have hit the next entry in the list
      average[averef].Nmeasure = N;
      N = 0;
      myObjID = objID[i];
      averef ++;
      average[averef].measureOffset = i;
    }
    if (measureNew[i].averef != averef) {
      fprintf (stderr, "fixing averef for measure "OFF_T_FMT" (%d -> "OFF_T_FMT")\n", i, measureNew[i].averef, averef);
      measureNew[i].averef = averef;
    }
    N++;
  }
  average[averef].Nmeasure = N;

  free (catalog->measure);
  catalog->measure = measureNew;

  int NmeasureTotal = 0;
  int measureOffsetOK = TRUE;
  for (i = 0; i < catalog->Naverage; i++) {
    NmeasureTotal += catalog[0].average[i].Nmeasure;
    if (!(NmeasureTotal <= catalog[0].Nmeasure)) {
      fprintf (stderr, "too many measurements: %d %d %d\n", (int) i, NmeasureTotal, (int) catalog[0].Nmeasure);
    }
    measureOffsetOK &= (catalog[0].average[i].measureOffset < catalog[0].Nmeasure);
    if (!(catalog[0].average[i].measureOffset < catalog[0].Nmeasure)) {
      fprintf (stderr, "offset too large: %d %d %d\n", (int) i, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
    }
    measureOffsetOK &= (catalog[0].average[i].measureOffset + catalog[0].average[i].Nmeasure <= catalog[0].Nmeasure);
    if (!(catalog[0].average[i].measureOffset + catalog[0].average[i].Nmeasure <= catalog[0].Nmeasure)) {
      fprintf (stderr, "offset + Nmeasure too large: %d + %d > %d %d\n", (int) i, catalog[0].average[i].measureOffset, catalog[0].average[i].Nmeasure, (int) catalog[0].Nmeasure);
    }
  }

  int status = TRUE;
  if (!measureOffsetOK) {
    fprintf (stderr, "ERROR: catalog %s has an invalid measureOffset\n", catalog[0].filename);
    status = FALSE;
  }

  if (NmeasureTotal != catalog[0].Nmeasure) {
    fprintf (stderr, "ERROR: catalog %s has an invalid Nmeasure\n", catalog[0].filename);
    status = FALSE;
  }

  if (!status) exit (2);

  return TRUE;
}
