# include "data.h"
# include "display.h"

int style_args (Graphdata *graphmode, int *argc, char **argv, int *kapa) {
  
  int N;
  char *colorName;
  char *kapaName;

  kapaName = NULL;
  if ((N = get_argument (*argc, argv, "-n"))) {
    remove_argument (N, argc, argv);
    kapaName = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!GetGraph (graphmode, kapa, kapaName)) return (FALSE);
  FREE (kapaName);

  if (*argc == 1) {
    kapaName = GetKapaName();
    colorName = KapaColorName (graphmode[0].color);
    gprint (GP_ERR, "current style (%s): -x %d -c %s -pt %d -lt %d -lw %f -sz %f\n", kapaName,
	     graphmode[0].style, colorName, graphmode[0].ptype, 
	     graphmode[0].ltype, graphmode[0].lweight,
	     graphmode[0].size);
    return (TRUE);
  }

  if ((N = get_argument (*argc, argv, "-lt"))) {
    remove_argument (N, argc, argv);
    graphmode[0].ltype = KapaLineTypeFromString(argv[N]);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-lw"))) {
    remove_argument (N, argc, argv);
    graphmode[0].lweight = atof(argv[N]);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-pt"))) {
    remove_argument (N, argc, argv);
    graphmode[0].ptype = KapaPointStyleFromString(argv[N]);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "+eb"))) {
    remove_argument (N, argc, argv);
    graphmode[0].ebar = TRUE;
  }
  if ((N = get_argument (*argc, argv, "-eb"))) {
    remove_argument (N, argc, argv);
    graphmode[0].ebar = FALSE;
  }
  if ((N = get_argument (*argc, argv, "-sz"))) {
    remove_argument (N, argc, argv);
    graphmode[0].size = atof(argv[N]);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-op"))) {
    remove_argument (N, argc, argv);
    graphmode[0].alpha = atof(argv[N]);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-c"))) {
    remove_argument (N, argc, argv);
    graphmode[0].color = KapaColorByName (argv[N]);
    if (graphmode[0].color == -1) return (FALSE);
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-x"))) {
    remove_argument (N, argc, argv);
    graphmode[0].style = KapaPlotStyleFromString(argv[N]);
    remove_argument (N, argc, argv);
  }

  if ((graphmode[0].style == KAPA_PLOT_POLYGON) || (graphmode[0].style == KAPA_PLOT_POLYFILL)) {
    if ((N = get_argument (*argc, argv, "-npoint"))) {
      remove_argument (N, argc, argv);
      graphmode[0].ptype = atoi (argv[N]);
      remove_argument (N, argc, argv);
    } else {
      gprint (GP_ERR, "polygon & polyfill styles require number of points argument: -npoint N\n");
      return FALSE;
    }
  }
  return (TRUE);
}
