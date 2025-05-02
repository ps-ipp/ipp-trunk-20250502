# include "data.h"

int textline (int argc, char **argv) {

  int N, size, FracPositions;
  char name[64];
  double x, y, angle;
  Graphdata graphmode;

  // if (!style_args (&graphmode, &argc, argv, &kapa)) return (FALSE);

  // Using only these style_args options
  char *kapaName = NULL;
  int kapa = -1;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    kapaName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (&graphmode, &kapa, kapaName)) return (FALSE);
  FREE (kapaName);

  int color = KapaColorByName ("black");
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    color = KapaColorByName (argv[N]);
    if (color == -1) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-fn"))) {
    remove_argument (N, &argc, argv);
    strcpy (name, argv[N]);
    remove_argument (N, &argc, argv);
    size = atof (argv[N]);
    remove_argument (N, &argc, argv);
    KapaSetFont (kapa, name, size);
  } 

  /* FracPositions uses coordinates of 0-1 relative to axis range */
  FracPositions = FALSE;
  if ((N = get_argument (argc, argv, "-frac"))) {
    remove_argument (N, &argc, argv);
    FracPositions = TRUE;
  } 

  angle = 0.0;
  if ((N = get_argument (argc, argv, "-rot"))) {
    remove_argument (N, &argc, argv);
    angle = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } 

  int justify = 5; // default
  if ((N = get_argument (argc, argv, "-justify"))) {
    remove_argument (N, &argc, argv);
    justify = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  } 

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: text x y (line) [-fn (font) size] [-rot angle] [-justify N]\n");
    return (FALSE);
  }

  if (strlen (argv[3]) > 127) {
    gprint (GP_ERR, "labels currently limited to 127 chars\n");
    return (FALSE);
  }

  x = atof (argv[1]);
  y = atof (argv[2]);
  
  if (FracPositions) {
    x =  x * (graphmode.xmax - graphmode.xmin) + graphmode.xmin;
    y =  y * (graphmode.ymax - graphmode.ymin) + graphmode.ymin;
  }    

  KapaSendTextline (kapa, argv[3], x, y, angle, justify, color);
  return (TRUE);
}
