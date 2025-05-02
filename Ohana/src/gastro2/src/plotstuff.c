# include "gastro2.h"
# include <signal.h>

static int Xgraph[5] = {0,0,0,0,0};
static int active;

void XDead (int value) {
  signal (SIGPIPE, XDead);
  fprintf (stderr, "kapa is dead, must restart\n");
  Xgraph[active] = -1;
}

int open_graph (int N) {

  char name[100];
  
  active = N;

  sprintf (name, "gastro [%d]", N);
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

void PrepPlotting (int Npts, Graphdata *graphmode, int N) {

  if (Xgraph[N] == 0) return;

  active = N;
  if (Npts < 1) return;

  KapaPrepPlot (Xgraph[N], Npts, graphmode);
}

void PlotVector (int Npts, float *vect, int mode, int N) {

  if (Npts < 1) return;
  active = N;

  switch (mode) {
    case 0:
      KapaPlotVector (Xgraph[N], Npts, vect, "x");
      break;
    case 1:
      KapaPlotVector (Xgraph[N], Npts, vect, "y");
      break;
    default:
      abort();
  }
}

void PlotReset (int N) {

  char buffer[128];
  int i;

  /* test Xgraph[N], flush junk from pipe */
  signal (SIGPIPE, XDead);
  fcntl (Xgraph[N], F_SETFL,  O_NONBLOCK); 
  for (i = 0; (read (Xgraph[N], buffer, 64) > 0) && (i < 20); i++);
  fcntl (Xgraph[N], F_SETFL, !O_NONBLOCK); 
  
  if (Xgraph[N] < 1) if (!open_graph(N)) return;
  KapaClearSections (Xgraph[N]);
}

/* include these lines to plot a pair of vectors: 

   typedef struct {
   double xmin, xmax, ymin, ymax;
   int style, ptype, ltype, etype, color;
   double lweight, size;
   } Graphdata;
   Graphdata graphdata;
   
   graphdata.xmin = -200;
   graphdata.xmax = 4200;
   graphdata.ymin = -500;
   graphdata.ymax = 500;
   graphdata.style = 2;
   graphdata.ptype = 2;
   graphdata.ltype = 0;
   graphdata.etype = 0;
   graphdata.color = 0;
   graphdata.lweight = 0;
   graphdata.size = 0.5;
   
   PrepPlotting (N, &graphdata, n);
   PlotVector (N, Y, 0, n);
   PlotVector (N, dM, 1, n);
   DonePlotting (&graphdata, n);
   
 */
