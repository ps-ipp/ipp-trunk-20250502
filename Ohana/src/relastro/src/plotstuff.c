# include "relastro.h"
# include <signal.h>

static int Xgraph[5] = {0,0,0,0,0};
static int active;

enum {black, white, red, orange, yellow, green, blue, indigo, violet};

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

int open_graph (int N) {

  char name[100];

  snprintf (name, 100, "gastro [%d]", N);
  Xgraph[N] = KapaOpen ("kapa", name);

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

void plot_list (Graphdata *graphdata, double *xlist, double *ylist, int N, char *label, char *file) {

  int i;
  StatType stats;
  
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

  if ((file != NULL) && SAVEPLOT) JpegPlot (graphdata, 0, file);
  if (PLOTDELAY > 500000) {
    fprintf (stdout, "press return\n"); 
    if (fscanf (stdin, "%*c") != 0) fprintf (stderr, "\n");
  } else {
    usleep (PLOTDELAY);
  }
}

void plot_defaults (Graphdata *graphdata) {

  graphdata[0].style = 2;
  graphdata[0].ptype = 2;
  graphdata[0].ltype = 0;
  graphdata[0].etype = 0;
  graphdata[0].color = black;
  graphdata[0].lweight = 0;
  graphdata[0].size = 0.5;

  graphdata[0].xmin = dUNDEF;
  graphdata[0].xmax = dUNDEF;
  graphdata[0].ymin = dUNDEF;
  graphdata[0].ymax = dUNDEF;
   
  graphdata[0].ticktextPad = NAN;
  graphdata[0].labelPadXm = NAN;
  graphdata[0].labelPadXp = NAN;
  graphdata[0].labelPadYm = NAN;
  graphdata[0].labelPadYp = NAN;
  graphdata[0].padXm = NAN;
  graphdata[0].padXp = NAN;
  graphdata[0].padYm = NAN;
  graphdata[0].padYp = NAN;
}
