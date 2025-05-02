# include "data.h"

int point (int argc, char **argv) {
  
  int N, celestial, pixscale;
  int kapa;
  double ra, dec, dra, ddec, angle;
  double x1, y1, ra1, dec1;
  char *name;
  Coords coords;
  Buffer *buf;
  KiiOverlay overlay;
  KapaImageData data;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImageData (&data, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  celestial = FALSE;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    celestial = TRUE;
  }
  
  pixscale = FALSE;
  if ((N = get_argument (argc, argv, "-pixscale"))) {
    remove_argument (N, &argc, argv);
    pixscale = TRUE;
  }

  angle = 0.0;
  if ((N = get_argument (argc, argv, "-angle"))) {
    remove_argument (N, &argc, argv);
    angle = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: point (overlay) TYPE x y dx dy [-c]\n");
    return (FALSE);
  }
  
  if (celestial) {
    if ((buf = SelectBuffer (data.name, OLDBUFFER, TRUE)) == NULL) return (FALSE);
    GetCoords (&coords, &buf[0].header);
  }

  if (celestial) {
    ra   = atof(argv[3]);
    dec  = atof(argv[4]);
    dra  = atof(argv[5]);
    ddec = atof(argv[6]);

    fRD_to_XY (&overlay.x, &overlay.y, ra, dec, &coords);
    if (pixscale) {
      overlay.dx = atof(argv[5]);
      overlay.dy = atof(argv[6]);
    } else {
      ra1 = ra + dra;
      dec1 = dec + ddec;
      RD_to_XY (&x1, &y1, ra1, dec1, &coords);
      overlay.dx = x1 - overlay.x;
      overlay.dy = y1 - overlay.y;
    }
  }
  else {
    overlay.x  = atof(argv[3]);
    overlay.y  = atof(argv[4]);
    overlay.dx = atof(argv[5]);
    overlay.dy = atof(argv[6]);
  }
  overlay.angle = angle;
  overlay.type = KiiOverlayTypeByName (argv[2]);
  KiiLoadOverlay (kapa, &overlay, 1, argv[1]);
  return (TRUE);
}
