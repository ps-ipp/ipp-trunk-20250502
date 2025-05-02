# include "data.h"

int dot (int argc, char **argv) {
  
  int kapa, N;
  Graphdata graphmode;
  float x, y;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  /* FracPositions uses coordinates of 0-1 relative to axis range */
  int FracPositions = FALSE;
  if ((N = get_argument (argc, argv, "-frac"))) {
    remove_argument (N, &argc, argv);
    FracPositions = TRUE;
  } 

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: dot <x> <y>\n");
    return (FALSE);
  }
  x = atof(argv[1]);
  y = atof(argv[2]);

  if (FracPositions) {
    x =  x * (graphmode.xmax - graphmode.xmin) + graphmode.xmin;
    y =  y * (graphmode.ymax - graphmode.ymin) + graphmode.ymin;
  }    

  /* set point style and errorbar mode (these are NOT sticky) */
  graphmode.style = KAPA_PLOT_POINTS;
  graphmode.etype = 0;

  if (!KapaPrepPlot (kapa, 1, &graphmode)) return (FALSE);
  KapaPlotVector (kapa, 1, &x, "x");
  KapaPlotVector (kapa, 1, &y, "y");
  
  return (TRUE);
}
