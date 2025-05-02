# include "data.h"

int pdf (int argc, char **argv) {

  char filename[1024], pagename[1024], *name;
  int N, kapa, scaleMode, pageMode;
  
  if ((N = get_argument (argc, argv, "--help"))) goto help;
  if ((N = get_argument (argc, argv, "-h"))) goto help;

  pageMode = KAPA_PS_NEWPLOT;

  /* new page? */
  /*
  if ((N = get_argument (argc, argv, "-newpage"))) {
    remove_argument (N, &argc, argv);
    pageMode = KAPA_PS_NEWPAGE;
  }
  if ((N = get_argument (argc, argv, "-raw"))) {
    remove_argument (N, &argc, argv);
    pageMode = KAPA_PS_RAWPAGE;
  }
  */

  /* scale image? */
  scaleMode = TRUE;
  if ((N = get_argument (argc, argv, "-noscale"))) {
    remove_argument (N, &argc, argv);
    scaleMode = FALSE;
  }

  /* what file? */
  filename[0] = 0;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    strcpy (filename, argv[N]);
    remove_argument (N, &argc, argv);
  }
  /* pagename ? */
  strcpy (pagename, "default");
  if ((N = get_argument (argc, argv, "-pagename"))) {
    remove_argument (N, &argc, argv);
    strcpy (pagename, argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* which tool */
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((argc > 1) && filename[0]) goto help;

  if (argc > 1) strcpy (filename, argv[1]);

  // get the connection to kapa, false if none available
  if (!GetGraphdata (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  if (!filename[0]) strcpy (filename, "kapa.ps");
  
  /* tell Ximage/Xgraph to ps the image */
  KapaPDF (kapa, filename, scaleMode, pageMode, pagename);
  return (TRUE);

help:
  gprint (GP_ERR, "USAGE: ps [-name file.ps] [-g | -i] [-n device] [-raw] [-noscale] [-newpage] [-pagename (name]\n");
  return (FALSE);
}

