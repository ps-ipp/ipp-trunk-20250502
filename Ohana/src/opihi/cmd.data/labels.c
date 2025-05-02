# include "data.h"

int labels (int argc, char **argv) {
  
  char name[64];
  int N, size, kapa;

  if (!GetGraph (NULL, &kapa, NULL)) return (FALSE);

  if (get_argument (argc, argv, "-h")) {
    gprint (GP_ERR, "label options: \n");
    gprint (GP_ERR, " -x : bottom-center\n");
    gprint (GP_ERR, " +x : top-center\n");
    gprint (GP_ERR, " -y : right-side\n");
    gprint (GP_ERR, " +y : left-side\n\n");

    gprint (GP_ERR, " -ul : upper-left corner\n");
    gprint (GP_ERR, " -ll : lower-left corner\n");
    gprint (GP_ERR, " -ur : upper-right corner\n");
    gprint (GP_ERR, " -lr : lower-right corner\n\n");

    gprint (GP_ERR, " -fn (font) (size) : set font and size\n");
    gprint (GP_ERR, "   (font) : courier, helvetica, times, symbol\n\n");
    gprint (GP_ERR, " label special characters:\n");
    gprint (GP_ERR, " ^ : superscript\n");
    gprint (GP_ERR, " _ : subscript\n");
    gprint (GP_ERR, " | : default script \n");
    gprint (GP_ERR, " &c, &h, &t, &s : set font\n\n");
    return (FALSE);
  }

  if ((N = get_argument (argc, argv, "-fn"))) {
    remove_argument (N, &argc, argv);
    strcpy (name, argv[N]);
    remove_argument (N, &argc, argv);
    size = atof (argv[N]);
    remove_argument (N, &argc, argv);
    KapaSetFont (kapa, name, size);
  } 

  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 0);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-y"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 1);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "+x"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 2);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "+y"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 3);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-ul"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 4);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-ur"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 5);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-ll"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 6);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-lr"))) {
    remove_argument (N, &argc, argv);
    KapaSendLabel (kapa, argv[N], 7);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: labels [-x] [-y] [+x] [+y] [-ul] [-ur] [-ll] [-lr]\n");
    return (FALSE);
  }

  return (TRUE);
}
