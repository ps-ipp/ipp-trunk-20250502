# include "data.h"

int antialias (int argc, char **argv) {

  int N, kapa;
  Graphdata graphmode;

  char *name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (&graphmode, &kapa, name)) return (FALSE);
  FREE (name);

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: antialias (sigma)\n");
    return (FALSE);
  }

  float sigma = atof (argv[1]);

  KapaSetSmoothSigma (kapa, sigma);
  return (TRUE);
}
