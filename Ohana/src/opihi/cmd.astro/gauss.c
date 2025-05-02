# include "astro.h"

int gauss (int argc, char **argv) {

  char key[20];
  int i, N, Npix, Nborder, Nspot;
  double X, Y, ZP, RA, DEC, max;
  int kapa;
  char *name;
  Buffer *buf;
  KapaImageData data;
  int VERBOSE;

  VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-q"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }

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

  Nborder = 3;
  if ((N = get_argument (argc, argv, "-border"))) {
    remove_argument (N, &argc, argv);
    Nborder  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  Nborder = MAX (Nborder, 1);
  
  max = 60000;
  if ((N = get_argument (argc, argv, "-sat"))) {
    remove_argument (N, &argc, argv);
    max  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  if ((argc != 2) && (argc != 3)) {
    gprint (GP_ERR, "USAGE: gauss Npix [Nspots] [-border N] [-sat cnts]\n");
    return (FALSE);
  }
  
  if (kapa < 1) {
    gprint (GP_ERR, "no active TV\n");
    return (FALSE);
  }

  Nspot = 0;
  Npix = atof (argv[1]);
  if (argc == 3) {
    Nspot = atof (argv[2]);
  }

  if ((buf = SelectBuffer (data.name, OLDBUFFER, TRUE)) == NULL) return (FALSE);

  KiiCursorOn (kapa);

  for (i = 0; (i < Nspot) || (Nspot == 0); i++) {
    KiiCursorRead (kapa, &X, &Y, &ZP, &RA, &DEC, key);
    if (!strcasecmp (key, "Q")) break;
    get_aperture_stats (&buf[0].matrix, (int)(X+0.5), (int)(Y+0.5), Npix, Nborder, max, VERBOSE);
  }
  KiiCursorOff (kapa);
  return (TRUE);
}


