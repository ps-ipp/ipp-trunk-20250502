# include "data.h"

int tvchannel (int argc, char **argv) {
  
  int N, kapa, Nchannel;
  char *name;
  KapaImageData data;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (&data, &kapa, name)) return (FALSE);
  FREE (name);

  // use the currently-set zero,range values if not supplied
  if (argc != 2) {
    gprint (GP_ERR, "USAGE: tvchannel (channel)\n");
    return (FALSE);
  }

  Nchannel = GetKapaChannelFromString (argv[1]);
  if (!Nchannel) return FALSE;

  KiiSetChannel (kapa, Nchannel - 1);
  return (TRUE);
}

int GetKapaChannelFromString (char *string) {

  int Nchannel = atoi (string);
  if (Nchannel == 0) {
    // try the string values R/Red, G/Green, B/Blue
    if (!strcasecmp (string, "R") || !strcasecmp (string, "RED")) {
      Nchannel = 1;
    }
    if (!strcasecmp (string, "G") || !strcasecmp (string, "GREEN")) {
      Nchannel = 2;
    }
    if (!strcasecmp (string, "B") || !strcasecmp (string, "BLUE")) {
      Nchannel = 3;
    }
  }
  if ((Nchannel < 1) || (Nchannel > 10)) {
    gprint (GP_ERR, "invalid channel : use 1 - 10 or (R)ed, (G)reen, (B)lue\n");
    gprint (GP_ERR, "   (R)ed, (G)reen, (B)lue == (1,2,3)\n");
    return (0);
  }
  return Nchannel;
}
