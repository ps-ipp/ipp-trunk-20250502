# include "data.h"

int cdot (int argc, char **argv) {
  
  int kapa, status;
  Graphdata graphmode;
  float x, y;
  double r, d, Rmin, Rmax;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: dot <ra> <dec>\n");
    return (FALSE);
  }
  r = atof(argv[1]);
  d = atof(argv[2]);

  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;
  // Rmid = 0.5*(Rmin + Rmax);

  /* set point style and errorbar mode (these are NOT sticky) */
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.etype = 0;

  r = ohana_normalize_angle (r);
  while (r < Rmin) r += 360.0;
  while (r > Rmax) r -= 360.0;

  status = fRD_to_XY (&x, &y, r, d, &graphmode.coords);
  if (!status) return TRUE;
  
  if (!KapaPrepPlot (kapa, 1, &graphmode)) return (FALSE);
  KapaPlotVector (kapa, 1, &x, "x");
  KapaPlotVector (kapa, 1, &y, "y");
  
  return (TRUE);
}
