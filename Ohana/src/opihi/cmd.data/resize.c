# include "data.h"

int resize (int argc, char **argv) {

  char *end;
  double NX, NY;
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

  if ((N = get_argument (argc, argv, "-by-image"))) {
    remove_argument (N, &argc, argv);
    KiiResizeByImage (kapa);
    return (TRUE);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: resize NX NY [-n] [-g | -i] [-by-image]\n");
    return (FALSE);
  }

  /* NX & NY are pixels for the screen & points for PS 
     convert units to points (1in = 72pt, 1cm = 28pt) */

  /* have kapa convert physical units to screen units 
     for now, fixed at 96 pix / in == 38 pix / cm
  */

  NX = strtod (argv[1], &end);
  if (!strcmp (end, "in")) { NX *= 96; }
  if (!strcmp (end, "cm")) { NX *= 38; }

  NY = strtod (argv[2], &end);
  if (!strcmp (end, "in")) { NY *= 96; }
  if (!strcmp (end, "cm")) { NY *= 38; }

  KiiResize (kapa, NX, NY);
  return (TRUE);
}

