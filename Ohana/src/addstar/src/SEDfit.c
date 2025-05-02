# include "sedstar.h"

// XXX a couple of fixes should speed this up a bit: test

int SEDfitCatalog (Catalog *outcat, Catalog *incat, SEDtable *table) {
  
  int i, j, m, n, idx, start, done, row, Nsec, Nfit, Nphot;
  int Nave, Nmeas, NAVE, NMEAS, Nmodel, Nreq;
  unsigned short USNOred, USNOblu;
  float color;
  int *found, valid, *modelRow, *reqRow;

  SEDtableRow sourceValue, sourceError;
  SEDfit minFit, testFit;

  Nmodel = Nreq = 0;
  for (i = 0; i < table[0].Nfilter; i++) {
    if (table[0].mode[i] == SED_REQ) Nreq ++;
    if (table[0].mode[i] == SED_MODEL) Nmodel ++;
  }
  if (Nmodel < 1) {
    fprintf (stderr, "no model filter defined\n!");
    exit (2);
  }
  if (Nreq < 1) {
    fprintf (stderr, "no required filter defined\n!");
    exit (2);
  }

  ALLOCATE (modelRow, int, Nmodel);
  for (j = i = 0; i < table[0].Nfilter; i++) {
    if (table[0].mode[i] == SED_MODEL) { 
      modelRow[j] = i;
      j++;
    }
  }
  ALLOCATE (reqRow, int, Nreq);
  for (j = i = 0; i < table[0].Nfilter; i++) {
    if (table[0].mode[i] == SED_REQ) { 
      reqRow[j] = i;
      j++;
    }
  }

  sourceValue.mags = NULL;
  sourceError.mags = NULL;

  Nsec = GetPhotcodeNsecfilt ();
  Nave = outcat[0].Naverage;
  Nmeas = outcat[0].Nmeasure;

  NAVE = 100;
  NMEAS = 100;
  REALLOCATE (outcat[0].average, Average, NAVE);
  REALLOCATE (outcat[0].secfilt, SecFilt, NAVE*Nsec);
  REALLOCATE (outcat[0].measure, Measure, NMEAS);

  // artificially set USNOred and blu errors to 0.3
  USNOred = GetPhotcodeCodebyName ("USNO_RED");
  USNOblu = GetPhotcodeCodebyName ("USNO_BLUE");

  // create holder for the source data
  ALLOCATE (sourceValue.mags, float, table[0].Nfilter);
  ALLOCATE (sourceError.mags, float, table[0].Nfilter);
  ALLOCATE (found, int, table[0].Nfilter);

  if (PLOT) SEDfitInit (table);

  // perform the fit to all sources
  for (i = 0; i < incat[0].Naverage; i++) {

    // blank out the source array
    for (j = 0; j < table[0].Nfilter; j++) {
      sourceValue.mags[j] = 100;
      found[j] = FALSE;
    }	

    // load the measurements for this source
    m = incat[0].average[i].measureOffset;
    Nphot = 0;
    for (j = 0; j < incat[0].average[i].Nmeasure; j++) {
      idx = table[0].hashcode[incat[0].measure[m+j].photcode];
      if (idx == -1) continue;
      // only fit the selected photcodes (mode == "fit")
      if (table[0].mode[idx] == SED_MODEL) continue; 
      if (table[0].mode[idx] == SED_SAMPLE) continue; 
      // XXX do something more clever if more than one value exists per photcode
      sourceValue.mags[idx] = incat[0].measure[m+j].M + table[0].vegaToAB[idx];
      sourceError.mags[idx] = incat[0].measure[m+j].dM;
      if (incat[0].measure[m+j].photcode == USNOred) sourceError.mags[idx] = 0.3;
      if (incat[0].measure[m+j].photcode == USNOblu) sourceError.mags[idx] = 0.3;
      found[idx] = TRUE;
      Nphot ++;
    }
    if (Nphot < 3) continue;

    // XXX pre-select list of REQ entries; loop over only those?
    valid = TRUE;
    for (j = 0; valid && (j < Nreq); j++) {
      if ((table[0].mode[reqRow[j]] == SED_REQ) && !found[reqRow[j]]) valid = FALSE;
    }
    if (!valid) continue;

    // skip sources without ref color
    if (sourceValue.mags[table[0].codeP] > 50) continue;
    if (sourceValue.mags[table[0].codeM] > 50) continue;
    color = sourceValue.mags[table[0].codeP] - sourceValue.mags[table[0].codeM];

    // find tableRow within 0.1 mag of color 
    // XXX : check on the delta value
    start = SEDcolorBracket (table, color, 0.05);
    minFit = SEDchisq (table[0].row[start], &sourceValue, &sourceError, table[0].Nfilter);
    minFit.row = start;

    // search for min chisq backwards
    // XXX : check on the delta value
    done = FALSE;
    row = start - 1;
    while (!done && (row > 0)) {
      testFit = SEDchisq (table[0].row[row], &sourceValue, &sourceError, table[0].Nfilter);
      if (testFit.chisq < minFit.chisq) {
	minFit = testFit;
	minFit.row = row;
      }
      if (fabs(table[0].row[row][0].color - color) > 0.25) done = TRUE;
      row --;
    }

    // search for min chisq forwards
    // XXX : check on the delta value
    done = FALSE;
    row = start + 1;
    while (!done && (row < table[0].Nrow)) {
      testFit = SEDchisq (table[0].row[row], &sourceValue, &sourceError, table[0].Nfilter);
      if (testFit.chisq < minFit.chisq) {
	minFit = testFit;
	minFit.row = row;
      }
      if (fabs(table[0].row[row][0].color - color) > 0.25) done = TRUE;
      row ++;
    }

    Nfit ++;
    // create the vectors for the example plots
    if (PLOT) SEDfitPlot (table, incat[0].average[i].R, incat[0].average[i].D, &minFit, &sourceValue, &sourceError);

    // construct an average object for this object
    // XXX for now, the output objects will have limited astrometric interpretation...
    dvo_average_init (&outcat[0].average[Nave]);
    outcat[0].average[Nave].R         = incat[0].average[i].R;
    outcat[0].average[Nave].D         = incat[0].average[i].D;

    for (j = 0; j < Nsec; j++) {
      dvo_secfilt_init (&outcat[0].secfilt[Nave*Nsec+j], SECFILT_RESET_ALL);
    }

    // we now have the min chisq row. use this to supply the other filter values....
    // XXX pre-select the SED_MODEL rows...
    if (Nmeas + table[0].Nfilter >= NMEAS) {
	NMEAS += 100 + table[0].Nfilter;
	REALLOCATE (outcat[0].measure, Measure, NMEAS);
    }

    double R = incat[0].average[i].R;
    double D = incat[0].average[i].D;

    for (j = 0; valid && (j < Nmodel); j++) {
      n = modelRow[j];
      dvo_measure_init (&outcat[0].measure[Nmeas]);
      outcat[0].measure[Nmeas].R         = R;
      outcat[0].measure[Nmeas].D         = D;
      outcat[0].measure[Nmeas].M         = table[0].row[minFit.row][0].mags[n] + minFit.Md;
      outcat[0].measure[Nmeas].dM        = 0.0;
      outcat[0].measure[Nmeas].McalPSF   = 0;
      outcat[0].measure[Nmeas].McalAPER  = 0;
      outcat[0].measure[Nmeas].t         = TIMEREF;
      outcat[0].measure[Nmeas].averef    = Nave;
      outcat[0].measure[Nmeas].photcode  = table[0].code[n];
      outcat[0].measure[Nmeas].photFlags = 0;
      outcat[0].measure[Nmeas].dbFlags   = 0;
      outcat[0].measure[Nmeas].dt        = 0xffff;
					 
      outcat[0].measure[Nmeas].airmass   = 0;
      outcat[0].measure[Nmeas].FWx       = NAN_S_SHORT;
      outcat[0].measure[Nmeas].FWy       = NAN_S_SHORT;
      outcat[0].measure[Nmeas].theta     = NAN_S_SHORT;

      outcat[0].average[Nave].Nmeasure++;
      Nmeas ++;
    }

    Nave ++;
    if (Nave >= NAVE) {
      NAVE += 100;
      REALLOCATE (outcat[0].average, Average, NAVE);
      REALLOCATE (outcat[0].secfilt, SecFilt, NAVE*Nsec);
    }
  }
  outcat[0].Naverage  = Nave;
  outcat[0].Nmeasure  = Nmeas;
  outcat[0].Nsecfilt_mem = Nave*Nsec;
  
  free (sourceValue.mags);
  free (sourceError.mags);
  free (found);

  SEDfitClear ();
  return (TRUE);
}
