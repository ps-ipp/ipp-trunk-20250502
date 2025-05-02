# include "sedstar.h"

// fit the data (with errors) to the given table row
SEDfit SEDchisq (SEDtableRow *ref, SEDtableRow *data, SEDtableRow *error, int Nfilter) {

  int i;
  double Sm, Sd, S2, wt, dM;
  SEDfit fit;

  Sm = Sd = S2 = 0.0;

  for (i = 0; i < Nfilter; i++) {
    if (data[0].mags[i] > 50.0) continue;

    if (error[0].mags[i] == 0.0) {
      wt = 1.0;
    } else {
      wt = 1.0 / SQ(error[0].mags[i]);
    }

    dM = data[0].mags[i] - ref[0].mags[i];
    S2 += SQ(dM) * wt;
    Sm += dM * wt;
    Sd += wt;
  }
    
  // row is assigned after fit
  fit.row = -1;
  fit.Md = Sm / Sd;
  fit.chisq = S2 + SQ(fit.Md) * Sd - 2*fit.Md*Sm;

  return (fit);
}

// find the first table row within 0.1 mag of the requested color (or within 10)
int SEDcolorBracket (SEDtable *table, float color, float delta) {

  int Nlo, Nhi, N;
  float tcolor;

  N = Nlo = 0; Nhi = table[0].Nrow;
  tcolor = table[0].row[Nlo][0].color;
  while ((Nhi - Nlo > 10) && (fabs(tcolor-color) > delta)) {
    N = 0.5*(Nlo + Nhi);
    N = MAX (N, 0);
    N = MIN (N, table[0].Nrow - 1);
    tcolor = table[0].row[N][0].color;
    if (tcolor < color) {
      Nlo = N;
    } else {
      Nhi = N + 1;
    }
  }
  return (N);
}

SEDtableRow **sort_SEDtable (SEDtableRow *raw, int N) {

  int i;

  SEDtableRow **value;
  
  if (N <= 0) return (NULL);

  ALLOCATE (value, SEDtableRow *, N);
  for (i = 0; i < N; i++) {
    value[i] = &raw[i];
  }

# define SWAPFUNC(A,B){ SEDtableRow *temp = value[A]; value[A] = value[B]; value[B] = temp; }
# define COMPARE(A,B)(value[A][0].color < value[B][0].color)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

  return (value);
}

static Graphdata graphdata;
static KapaSection magSection, resSection;
static int Xgraph;
static float *fitmags, *fiterrs;

int SEDfitInit (SEDtable *table) {

  Xgraph = KapaOpen ("kapa", "sedstar");
  KapaInitGraph (&graphdata);
  SetLimitsRaw (table[0].wavecode, NULL, table[0].Nfilter, &graphdata);
  graphdata.style = 2;
  graphdata.ptype = 2;
  KapaClearSections (Xgraph);
  magSection.name = strcreate ("mag");
  magSection.x  = 0;
  magSection.dx = 1;
  magSection.y  = 0.5;
  magSection.dy = 0.5;
  resSection.name = strcreate ("res");
  resSection.x  = 0.0;
  resSection.dx = 1.0;
  resSection.y  = 0.0;
  resSection.dy = 0.5;
    
  KiiResize (Xgraph, 900, 500);
  KapaSetFont (Xgraph, "helvetica", 14);
  ALLOCATE (fitmags, float, table[0].Nfilter);
  ALLOCATE (fiterrs, float, table[0].Nfilter);
  return (TRUE);
}

int SEDfitClear () {

  free (fitmags);
  free (fiterrs);
  KapaClose (Xgraph);
  return (TRUE);
}

int SEDfitPlot (SEDtable *table, double R, double D, SEDfit *minFit, SEDtableRow *sourceValue, SEDtableRow *sourceError) {

  int j, minRow, Nfilter;
  double X, Y, Z, RA, DEC;
  char line[1024], key[20];

  minRow = minFit[0].row;
  Nfilter = table[0].Nfilter;

  // we want to plot the OBSERVED magnitudes
  for (j = 0; j < Nfilter; j++) {
    fitmags[j] = table[0].row[minRow][0].mags[j] + minFit[0].Md;
  }

  // find plot range
  SetLimitsRaw (NULL, fitmags, Nfilter, &graphdata);
  SWAP (graphdata.ymin, graphdata.ymax);

  KapaClearSections (Xgraph);
  KapaSetSection (Xgraph, &magSection);
  KapaSetLimits (Xgraph, &graphdata);
  KapaBox (Xgraph, &graphdata);
  graphdata.color = KapaColorByName ("blue");
  graphdata.etype = 0;
  graphdata.ptype = 7;
  KapaPrepPlot (Xgraph, Nfilter, &graphdata);
  KapaPlotVector (Xgraph, Nfilter, table[0].wavecode, "x");
  KapaPlotVector (Xgraph, Nfilter, fitmags, "y");

  graphdata.color = KapaColorByName ("red");
  graphdata.etype = 1;
  graphdata.ptype = 2;
  for (j = 0; j < Nfilter; j++) {
    fitmags[j] = 100;
    fiterrs[j] = 0;
    if (sourceValue[0].mags[j] > 50) continue;
    fitmags[j] = sourceValue[0].mags[j];
    fiterrs[j] = sourceError[0].mags[j];
  }
  KapaPrepPlot (Xgraph, Nfilter, &graphdata);
  KapaPlotVector (Xgraph, Nfilter, table[0].wavecode, "x");
  KapaPlotVector (Xgraph, Nfilter, fitmags, "x");
  KapaPlotVector (Xgraph, Nfilter, fiterrs, "dym");
  KapaPlotVector (Xgraph, Nfilter, fiterrs, "dyp");
  KapaSendLabel (Xgraph, "model,fit (mags)", 1);

  sprintf (line, "star: %10.6f %10.6f  T: %5.0fK  A_V|: %4.2f  M_D|: %5.2f  &sc&h^2|: %5.2f", 
	   R, D, 
	   table[0].row[minRow][0].Temp, 
	   table[0].row[minRow][0].Av, 
	   minFit[0].Md, minFit[0].chisq);
  KapaSendLabel (Xgraph, line, 2);
  KapaSendLabel (Xgraph, "model,fit (mags)", 1);

  KapaSetSection (Xgraph, &resSection);
  graphdata.ymin = -1.0;
  graphdata.ymax = +1.0;
  KapaSetLimits (Xgraph, &graphdata);
  KapaBox (Xgraph, &graphdata);
  graphdata.color = KapaColorByName ("red");
  graphdata.etype = 1;

  for (j = 0; j < Nfilter; j++) {
    fitmags[j] = 100;
    fiterrs[j] = 0;
    if (sourceValue[0].mags[j] > 50) continue;
    fitmags[j] = sourceValue[0].mags[j] - minFit[0].Md - table[0].row[minRow][0].mags[j];
    fiterrs[j] = sourceError[0].mags[j];
  }
  KapaPrepPlot (Xgraph, Nfilter, &graphdata);
  KapaPlotVector (Xgraph, Nfilter, table[0].wavecode, "x");
  KapaPlotVector (Xgraph, Nfilter, fitmags, "y");
  KapaPlotVector (Xgraph, Nfilter, fiterrs, "dym");
  KapaPlotVector (Xgraph, Nfilter, fiterrs, "dyp");
  KapaSendLabel (Xgraph, "wavelength (nm)", 0);
  KapaSendLabel (Xgraph, "resid (mags)", 1);

  KiiCursorOn (Xgraph);
  while (KiiCursorRead (Xgraph, &X, &Y, &Z, &RA, &DEC, key)) {
    // fprintf (stderr, "window: %f %f (%s)\n", X, Y, key);
    if (!strcasecmp (key, "Q")) {
      KiiCursorOff (Xgraph);
      break;
    }
    if (!strcasecmp (key, "ESCAPE")) {
      KiiCursorOff (Xgraph);
      PLOT = FALSE;
      return (TRUE);
    }
    if (!strcasecmp (key, "X")) {
      KiiCursorOff (Xgraph);
      Shutdown ("quitting sedstar");
    }
  }
  return (TRUE);
}

void SetLimitsRaw (float *xvec, float *yvec, int Nelements, Graphdata *graphmode) {

  double maxX, minX, maxY, minY, range;
  int i;

  if (xvec != NULL) {
    maxX = minX = xvec[0];
    for (i = 1; i < Nelements; i++) {
      if (!finite(xvec[i])) continue;
      maxX = MAX (maxX, xvec[i]);
      minX = MIN (minX, xvec[i]);
    }
    range = maxX - minX;
    if (range == 0) range = 0.001 * maxX;
    if (range == 0) range = 0.001;
    graphmode[0].xmin = minX - 0.05*range;
    graphmode[0].xmax = maxX + 0.05*range;
  }

  if (yvec != NULL) {
    maxY = minY = yvec[0];
    for (i = 1; i < Nelements; i++) {
      if (!finite(yvec[i])) continue;
      maxY = MAX (maxY, yvec[i]);
      minY = MIN (minY, yvec[i]);
    }
    range = maxY - minY;
    if (range == 0) range = 0.0011 * maxY;
    if (range == 0) range = 0.0011;
    graphmode[0].ymin = minY - 0.05*range;
    graphmode[0].ymax = maxY + 0.05*range;
  }
}
