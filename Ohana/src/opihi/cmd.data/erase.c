# include "data.h"

int erase (int argc, char **argv) {
  
  int i, N;
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

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: erase (overlay) [overlay, overlay, ..] \n");
    gprint (GP_ERR, " (overlay) may be: red (0), green (1), blue (2), yellow (3) or all\n");
    return (FALSE);
  }

  for (i = 1; i < argc; i++) {
    if (!(strcasecmp (argv[i], "all"))) {
      KiiEraseOverlay (kapa, "red");
      KiiEraseOverlay (kapa, "green");
      KiiEraseOverlay (kapa, "blue");
      KiiEraseOverlay (kapa, "yellow");
      continue;
    }
    if (!KiiSelectOverlay (argv[i], &N)) {
      gprint (GP_ERR, "%s is not a valid overlay\n", argv[i]);
      return (FALSE);
    }
    KiiEraseOverlay (kapa, argv[i]);
  }
  return (TRUE);
}
