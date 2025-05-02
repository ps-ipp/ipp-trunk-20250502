# include "data.h"

// default is to clear all plots, but not the sections or the images
int clear (int argc, char **argv) {
  
  int N;
  int kapa;
  char *name;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  // clear all sections
  if ((N = get_argument (argc, argv, "-s")) || 
      (N = get_argument (argc, argv, "-section"))) {
      KapaClearSections (kapa);
      return (TRUE);
  }

  // clear all sections
  if ((N = get_argument (argc, argv, "-graph"))) {
      KapaClearCurrentPlot (kapa);
      return (TRUE);
  }

  // clear image
  if ((N = get_argument (argc, argv, "-image"))) {
      KapaClearImage (kapa);
      return (TRUE);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: clear [-n Xgraph] [-s|-section] [-image] [-graph]\n");
    gprint (GP_ERR, "       [-s|-section] : clear all sections\n");
    gprint (GP_ERR, "       [-graph]      : clear current graph\n");
    gprint (GP_ERR, "       [-image]      : clear current image\n");
    return (FALSE);
  }

  KapaClearPlots (kapa);
  return (TRUE);
}
