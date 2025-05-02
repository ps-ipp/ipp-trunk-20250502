# include "data.h"

int tv (int argc, char **argv) {
  
  int N, kapa;
  char *name, *file;
  Coords coords;
  Buffer *buf;
  KiiImage image;
  KapaImageData data;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (&data, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    remove_argument (N, &argc, argv);
    channel = GetKapaChannelFromString (argv[N]);
    remove_argument (N, &argc, argv);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  int plane = 0;
  if ((N = get_argument (argc, argv, "-plane"))) {
    remove_argument (N, &argc, argv);
    plane = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (plane < 0) {
    gprint (GP_ERR, " ERROR: -plane (plane) : cannot be negative\n");
    return (FALSE);
  }

  /* shell exits on pipe close, FIX */
  if ((N = get_argument (argc, argv, "-kill"))) {
    KapaClose (kapa);
    return (TRUE);
  }

  data.logflux = FALSE;
  if ((N = get_argument (argc, argv, "-log"))) {
    remove_argument (N, &argc, argv);
    data.logflux = TRUE;
  }

  // use the currently-set zero,range values if not supplied
  if ((argc != 2) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: tv <buffer> [zero range] [-n Nimage] [-log] [-kill]\n");
    return (FALSE);
  }

  if (argc == 4) {
    data.zero = atof (argv[2]);
    data.range = atof (argv[3]);
    if (data.range == 0.0) data.range = 0.001;
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  GetCoords (&coords, &buf[0].header);
  
  image.Nx = buf[0].matrix.Naxis[0];
  image.Ny = buf[0].matrix.Naxis[1];

  int tooBig = buf[0].matrix.Naxis[2] ? (plane >= buf[0].matrix.Naxis[2]) : plane > 0;
  if (tooBig) {
    gprint (GP_ERR, " ERROR: -plane (plane) : out of bounds (%d vs "OFF_T_FMT")\n", plane, buf[0].matrix.Naxis[2]);
    return (FALSE);
  }
  int Npix2D = image.Nx * image.Ny;

  float *imdata = (float *) buf[0].matrix.buffer;
  image.data1d = &imdata[plane*Npix2D];

  // send only the root of the file, not the full path
  file = filerootname (buf[0].file);
  strcpy (data.file, file);
  free (file);

  strcpy (data.name, argv[1]);
  
  KiiNewPicture1D (kapa, &image, &data, &coords);

  set_str_variable ("TV", argv[1]);
  return (TRUE);
}
