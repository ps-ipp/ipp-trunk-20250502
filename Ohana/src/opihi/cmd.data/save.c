# include "data.h"

int save (int argc, char **argv) {
  
  int N, celestial;
  int kapa;
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

  celestial = FALSE;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    celestial = TRUE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: save (overlay) <filename> [-c]\n");
    gprint (GP_ERR, "  -c: write contour in celestial coords\n");
    return (FALSE);
  }

  KiiSaveOverlay (kapa, celestial, argv[1], argv[2]);
  return (TRUE);
}
