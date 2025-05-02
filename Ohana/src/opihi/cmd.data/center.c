# include "data.h"

int center (int argc, char **argv) {
  
  double x, y;
  int zoom;
  int kapa, N;
  char *name;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  // XXX need an option to center the image based on the current plot limits

  if ((argc != 3) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: center x y [zoom]\n");
    return (FALSE);
  }

  x = atof (argv[1]);
  y = atof (argv[2]);
  zoom = 0;
  if (argc == 4) zoom = atof (argv[3]);

  KiiCenter (kapa, x, y, zoom);
  return (TRUE);
}

int parity (int argc, char **argv) {
  
  int x, y;
  int kapa, N;
  char *name;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: parity x y\n");
    return (FALSE);
  }

  x = atof (argv[1]);
  y = atof (argv[2]);

  KiiParity (kapa, x, y);
  return (TRUE);
}
