# include "display.h"

int PlotVectorSingle (int kapa, Vector *vec, char *mask, char *mode) {

  int i, Npts, Nout;
  float *temp;

  Npts = vec->Nelements;
  ALLOCATE (temp, float, Npts);

  Nout = 0;
  if (vec->type == OPIHI_FLT) {
    opihi_flt *value = vec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  } else {
    opihi_int *value = vec->elements.Int;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  }
  KapaPlotVector (kapa, Nout, temp, mode);

  free (temp);

  return (TRUE);
}

int PlotVectorPair (int kapa, Vector *xVec, Vector *yVec, char *mask, Graphdata *graphmode) {

  int i, Npts, Nout;
  float *temp;

  if (xVec->Nelements != yVec->Nelements) return (FALSE);
  Npts = xVec->Nelements;

  ALLOCATE (temp, float, Npts);

  Nout = 0;
  if (xVec->type == OPIHI_FLT) {
    opihi_flt *value = xVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  } else {
    opihi_int *value = xVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  }
  KapaPrepPlot (kapa, Nout, graphmode);
  KapaPlotVector (kapa, Nout, temp, "x");

  Nout = 0;
  if (yVec->type == OPIHI_FLT) {
    opihi_flt *value = yVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  } else {
    opihi_int *value = yVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  }
  KapaPlotVector (kapa, Nout, temp, "y");

  free (temp);

  return (TRUE);
}

int PlotVectorTriplet (int kapa, Vector *xVec, Vector *yVec, Vector *zVec, char *mask, Graphdata *graphmode) {

  int i, Npts, Nout;
  float *temp;

  if (xVec->Nelements != yVec->Nelements) return (FALSE);
  if (xVec->Nelements != zVec->Nelements) return (FALSE);
  Npts = xVec->Nelements;

  ALLOCATE (temp, float, Npts);

  Nout = 0;
  if (xVec->type == OPIHI_FLT) {
    opihi_flt *value = xVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  } else {
    opihi_int *value = xVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  }
  KapaPrepPlot (kapa, Nout, graphmode);
  KapaPlotVector (kapa, Nout, temp, "x");

  Nout = 0;
  if (yVec->type == OPIHI_FLT) {
    opihi_flt *value = yVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  } else {
    opihi_int *value = yVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  }
  KapaPlotVector (kapa, Nout, temp, "y");

  Nout = 0;
  if (zVec->type == OPIHI_FLT) {
    opihi_flt *value = zVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  } else {
    opihi_int *value = zVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      if (mask && mask[i]) continue;
      temp[Nout] = value[i];
      Nout ++;
    }
  }
  KapaPlotVector (kapa, Nout, temp, "z");

  free (temp);

  return (TRUE);
}

int PlotVectorPairErrors (int kapa, Vector *xVec, Vector *yVec, Vector *dyVec, Graphdata *graphmode) {

  int i, Npts;
  float *temp;

  if (xVec->Nelements != yVec->Nelements) return (FALSE);
  if (xVec->Nelements != dyVec->Nelements) return (FALSE);
  Npts = xVec->Nelements;

  ALLOCATE (temp, float, Npts);

  KapaPrepPlot (kapa, Npts, graphmode);

  if (xVec->type == OPIHI_FLT) {
    opihi_flt *value = xVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      temp[i] = value[i];
    }
  } else {
    opihi_int *value = xVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      temp[i] = value[i];
    }
  }
  KapaPlotVector (kapa, Npts, temp, "x");

  if (yVec->type == OPIHI_FLT) {
    opihi_flt *value = yVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      temp[i] = value[i];
    }
  } else {
    opihi_int *value = yVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      temp[i] = value[i];
    }
  }
  KapaPlotVector (kapa, Npts, temp, "y");

  if (dyVec->type == OPIHI_FLT) {
    opihi_flt *value = dyVec->elements.Flt;
    for (i = 0; i < Npts; i++) {
      temp[i] = value[i];
    }
  } else {
    opihi_int *value = dyVec->elements.Int;
    for (i = 0; i < Npts; i++) {
      temp[i] = value[i];
    }
  }
  KapaPlotVector (kapa, Npts, temp, "dym");
  KapaPlotVector (kapa, Npts, temp, "dyp");

  free (temp);

  return (TRUE);
}

