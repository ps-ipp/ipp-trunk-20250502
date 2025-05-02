# include "data.h"

int cline (int argc, char **argv) {
  
  int kapa, status;
  Graphdata graphmode;
  float x[2], y[2];
  double r[2], d[2];
  
  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: cline <x> <y> to <x> <y>\n");
    return (FALSE);
  }
  r[0] = atof(argv[1]);
  d[0] = atof(argv[2]);
  r[1] = atof(argv[4]);
  d[1] = atof(argv[5]);

  /* set point style and errorbar mode (these are NOT sticky) */
  graphmode.style = KAPA_PLOT_CONNECT;
  graphmode.etype = 0;

  // need to plot to edge of picture..
  status = fRD_to_XY (&x[0], &y[0], r[0], d[0], &graphmode.coords);
  if (!status) {
    return FALSE;
  }
  status = fRD_to_XY (&x[1], &y[1], r[1], d[1], &graphmode.coords);
  if (!status) {
    return FALSE;
  }
  
  if (!KapaPrepPlot (kapa, 2, &graphmode)) return (FALSE);
  KapaPlotVector (kapa, 2, x, "x");
  KapaPlotVector (kapa, 2, y, "y");
  
  return (TRUE);
}
