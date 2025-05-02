# include "data.h"

int line (int argc, char **argv) {
  
  int kapa, N;
  Graphdata graphmode;
  float x[2], y[2];

  /* FracPositions uses coordinates of 0-1 relative to axis range */
  int FracPositions = FALSE;
  if ((N = get_argument (argc, argv, "-frac"))) {
    remove_argument (N, &argc, argv);
    FracPositions = TRUE;
  } 

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: line <x> <y> to <x> <y>\n");
    return (FALSE);
  }
  x[0] = atof(argv[1]);
  y[0] = atof(argv[2]);
  x[1] = atof(argv[4]);
  y[1] = atof(argv[5]);

  if (FracPositions) {
    x[0] =  x[0] * (graphmode.xmax - graphmode.xmin) + graphmode.xmin;
    y[0] =  y[0] * (graphmode.ymax - graphmode.ymin) + graphmode.ymin;
    x[1] =  x[1] * (graphmode.xmax - graphmode.xmin) + graphmode.xmin;
    y[1] =  y[1] * (graphmode.ymax - graphmode.ymin) + graphmode.ymin;
  }    

  /* set point style and errorbar mode (these are NOT sticky) */
  graphmode.style = KAPA_PLOT_CONNECT;
  graphmode.etype = 0;

  if (!KapaPrepPlot (kapa, 2, &graphmode)) return (FALSE);
  KapaPlotVector (kapa, 2, x, "x");
  KapaPlotVector (kapa, 2, y, "y");
  
  return (TRUE);
}
