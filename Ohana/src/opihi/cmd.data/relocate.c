# include "data.h"

int relocate (int argc, char **argv) {

  int x, y;
  int N, kapa;
  char *name;
  
  /* display source */
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

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: relocate x y [-n]\n");
    return (FALSE);
  }

  /* x & y are pixels for the screen -- this command has no meaning for non-X */

  x = atoi (argv[1]);
  y = atoi (argv[2]);

  KiiRelocate (kapa, x, y);
  return (TRUE);
}

