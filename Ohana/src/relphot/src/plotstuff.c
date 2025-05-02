# include "relphot.h"
# include <signal.h>

static int Xgraph[5] = {0,0,0,0,0};
static int active;

/*
static union { unsigned char c[4]; float f; } f_undef = { {0xff, 0xff, 0xff, 0xfe} };
# define fUNDEF (f_undef.f)
*/

static union { unsigned char c[8]; float d; } d_undef = { {0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00} };
# define dUNDEF (d_undef.d)

void XDead () {
  signal (SIGPIPE, XDead);
  fprintf (stderr, "kapa is dead, must restart\n");
  Xgraph[active] = -1;
}

int get_graph (int N) {
  return Xgraph[N];
}

int open_graph (int N) {

  char name[100];

  sprintf (name, "gastro:%d", N);

  // if -plot is supplied, the plots are shown on the screen; 
  // otherwise, only the final plots are generated and saved without display
  if (PLOTSTUFF) {
    Xgraph[N] = KapaOpen ("kapa", name);
  } else {
    Xgraph[N] = KapaOpen ("kapa -noX", name);
  }

  if (Xgraph[N] < 0) {
    fprintf (stderr, "error starting kapa\n");
    return (FALSE);
  }
  return (TRUE);
}

void DonePlotting (Graphdata *graphmode, int N) {

  if (Xgraph[N] == 0) return;
  KapaBox (Xgraph[N], graphmode);
  return;
}

void JpegPlot (Graphdata *graphmode, int N, char *filename) {
  OHANA_UNUSED_PARAM(graphmode);

  if (Xgraph[N] == 0) return;

  KapaPNG (Xgraph[N], filename);
  return;
}

void PSPlot (Graphdata *graphmode, int N, char *filename) {
  OHANA_UNUSED_PARAM(graphmode);

  if (Xgraph[N] == 0) return;

  KiiPS (Xgraph[N], filename, TRUE, KAPA_PS_NEWPLOT, "default");
  return;
}

void PrepPlotting (int Npts, Graphdata *graphmode, int N) {

  if (Npts < 1) return;

  if (Xgraph[N] < 1) if (!open_graph(N)) return;

  KapaClearSections (Xgraph[N]);

  KapaSetLimits (Xgraph[N], graphmode);
  KapaSetFont (Xgraph[N], "helvetica", 14);
  KapaBox (Xgraph[N], graphmode);
  // KapaSendLabel (Xgraph[N], "PSF Mag", KAPA_LABEL_XM);
  // KapaSendLabel (Xgraph[N], "Ap Mag - PSF Mag", KAPA_LABEL_YM);

  KapaPrepPlot (Xgraph[N], Npts, graphmode);
  return;
}

void PlotLabel (char *string, int N) {

  if (Xgraph[N] == 0) return;

  KapaSendLabel (Xgraph[N], string, 2);
}

void PlotVector (int Npts, double *vect, int mode, int N, char *type) {
  OHANA_UNUSED_PARAM(mode);

  float *values;
  int i;

  if (Npts < 1) return;

  ALLOCATE (values, float, Npts);
  for (i = 0; i < Npts; i++) {
    values[i] = vect[i];
  }

  KapaPlotVector (Xgraph[N], Npts, values, type);
  free (values);
  return;
}

void plot_wait () {
  if (PLOTDELAY > 500000) {
    fprintf (stdout, "press return\n"); 
    if (fscanf (stdin, "%*c") != 1) fprintf (stderr, "\n");
  } else {
    usleep (PLOTDELAY);
  }
}

// plot the vector pair to a file with name defined by the varargs format
void plot_list (Graphdata *graphdata, double *xlist, double *ylist, int N, char *label, char *format, ...) {

  char tmp, *filename;
  int i, Nbyte;
  StatType stats;
  va_list argp;  

  va_start (argp, format);
  Nbyte = vsnprintf (&tmp, 0, format, argp);
  va_end (argp);
  if (!Nbyte) return;
  Nbyte ++;

  ALLOCATE (filename, char, Nbyte);
  va_start (argp, format);
  vsnprintf (filename, Nbyte, format, argp);
  va_end (argp);
  
  stats.min = stats.max = xlist[0];
  for (i = 0; i < N; i++) {
    stats.min = MIN (stats.min, xlist[i]);
    stats.max = MAX (stats.max, xlist[i]);
  }
  if (graphdata[0].xmin == dUNDEF) graphdata[0].xmin = 1.05*stats.min - 0.05*stats.max;
  if (graphdata[0].xmax == dUNDEF) graphdata[0].xmax = 1.05*stats.max - 0.05*stats.min;

  stats.min = stats.max = ylist[0];
  for (i = 0; i < N; i++) {
    stats.min = MIN (stats.min, ylist[i]);
    stats.max = MAX (stats.max, ylist[i]);
  }
  if (graphdata[0].ymin == dUNDEF) graphdata[0].ymin = 1.05*stats.min - 0.05*stats.max;
  if (graphdata[0].ymax == dUNDEF) graphdata[0].ymax = 1.05*stats.max - 0.05*stats.min;

  PrepPlotting (N, graphdata, 0);
  PlotVector (N, xlist, 0, 0, "x");
  PlotVector (N, ylist, 1, 0, "y");
  if (label != NULL) PlotLabel (label, 0);
  DonePlotting (graphdata, 0);

  if (SAVEPLOT) JpegPlot (graphdata, 0, filename);
  plot_wait();
  free (filename);
}

// plot the vector pair to a file with name defined by the varargs format
void plot_list_add (Graphdata *graphdata, double *xlist, double *ylist, int Npts) {

  KapaPrepPlot (Xgraph[0], Npts, graphdata);
  PlotVector (Npts, xlist, 0, 0, "x");
  PlotVector (Npts, ylist, 1, 0, "y");
  plot_wait();
}

void plot_defaults (Graphdata *graphdata) {

  KapaInitGraph (graphdata);
  graphdata[0].style = KAPA_PLOT_POINTS; /* points */
  graphdata[0].ptype = 2;
  graphdata[0].ltype = 0;
  graphdata[0].etype = 0;
  graphdata[0].color = black;
  graphdata[0].lweight = 0;
  graphdata[0].size = 1.0;

  graphdata[0].xmin = dUNDEF;
  graphdata[0].xmax = dUNDEF;
  graphdata[0].ymin = dUNDEF;
  graphdata[0].ymax = dUNDEF;
   
}
