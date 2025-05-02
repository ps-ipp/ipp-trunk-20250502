# include "dvo.h"

void CopyAverageToTiny (AverageTiny *averageT, Average *average) {
  memset (averageT, 0, sizeof(AverageTiny));
  averageT[0].R     	    = average[0].R;
  averageT[0].D     	    = average[0].D;
  averageT[0].flags 	    = average[0].flags;
  averageT[0].Nmeasure      = average[0].Nmeasure;
  averageT[0].measureOffset = average[0].measureOffset;
  averageT[0].catID         = average[0].catID;
  averageT[0].objID         = average[0].objID;
  averageT[0].nOwn          = -1;
  return;
}

void CopyMeasureToTiny (MeasureTiny *measureT, Measure *measure) {
  memset (measureT, 0, sizeof(MeasureTiny));
  measureT[0].R          = measure[0].R;
  measureT[0].D          = measure[0].D;
  measureT[0].M          = measure[0].M;
  measureT[0].Mkron      = measure[0].Mkron;
  measureT[0].McalPSF    = measure[0].McalPSF;
  measureT[0].McalAPER   = measure[0].McalAPER;
  measureT[0].dM         = measure[0].dM;
  measureT[0].airmass    = measure[0].airmass;
  measureT[0].Mflat      = measure[0].Mflat;
  measureT[0].Xccd       = measure[0].Xccd;
  measureT[0].Yccd       = measure[0].Yccd;
  measureT[0].Xfix       = measure[0].Xfix;
  measureT[0].Yfix       = measure[0].Yfix;
  measureT[0].t          = measure[0].t;
  measureT[0].dt         = measure[0].dt;
  measureT[0].psfQF      = measure[0].psfQF;
  measureT[0].averef     = measure[0].averef;
  measureT[0].imageID    = measure[0].imageID;
  measureT[0].dbFlags    = measure[0].dbFlags;
  measureT[0].photFlags  = measure[0].photFlags;
  measureT[0].photcode   = measure[0].photcode;
  measureT[0].catID      = measure[0].catID;
  measureT[0].dXccd      = measure[0].dXccd;
  measureT[0].dYccd      = measure[0].dYccd;
  measureT[0].dRsys      = measure[0].dRsys;
  measureT[0].myDet      = 0;
  return ;
}

// for the cases where we are not using a subset of the data, we still need to have a copy of these fields
int populate_tiny_values (Catalog *catalog, DVOTinyValueMode mode) {

  off_t i;

  AverageTiny *averageT;
  Average *average;

  MeasureTiny *measureT;
  Measure *measure;

  if (mode & DVO_TV_AVERAGE) {
    ALLOCATE (catalog[0].averageT, AverageTiny, catalog[0].Naverage);
    average  = catalog[0].average;
    averageT = catalog[0].averageT;

    for (i = 0; i < catalog[0].Naverage; i++) {
      CopyAverageToTiny (&averageT[i], &average[i]);
    }
  }

  if (mode & DVO_TV_MEASURE) {
    ALLOCATE (catalog[0].measureT, MeasureTiny, catalog[0].Nmeasure);
    measure  = catalog[0].measure;
    measureT = catalog[0].measureT;

    for (i = 0; i < catalog[0].Nmeasure; i++) {
      CopyMeasureToTiny (&measureT[i], &measure[i]);
    }
  }
  return (TRUE);
}

int free_tiny_values (Catalog *catalog) {

  if (catalog[0].averageT) {
    free (catalog[0].averageT);
    catalog[0].averageT = NULL;
  }
  if (catalog[0].measureT) { 
    free (catalog[0].measureT);
    catalog[0].measureT = NULL;
  }
  return (TRUE);
}

