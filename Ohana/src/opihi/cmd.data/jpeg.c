# include "data.h"

int jpeg (int argc, char **argv) {

  char filename[1024];
  int N, kapa, IsPNG, IsPPM;
  char *name;
  
  if ((N = get_argument (argc, argv, "--help"))) {
    gprint (GP_ERR, "USAGE: jpeg [-name file] [-g | -i] [-n device] [-ppm]\n");
    return (FALSE);
  }

  /* image type */
  IsPPM = IsPNG = FALSE;
  if ((N = get_argument (argc, argv, "-ppm"))) {
    remove_argument (N, &argc, argv);
    IsPPM = TRUE;
  }
  if ((N = get_argument (argc, argv, "-png"))) {
    remove_argument (N, &argc, argv);
    IsPNG = TRUE;
  }
  if (!strcmp (argv[0], "png")) IsPNG = TRUE;
  if (!strcmp (argv[0], "ppm")) IsPPM = TRUE;

  /* file name */
  filename[0] = 0;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    strcpy (filename, argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* display source */
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* XXX output png / jpeg needs to include both graph and image
     if available.  this is a poor mix of data representations 
     (png for image / jpeg for plots)
  if ((N = get_argument (argc, argv, "-g"))) {
  if ((N = get_argument (argc, argv, "-i"))) {
  */

  if (!GetGraph (NULL, &kapa, name)) return (FALSE);
  if (!IsPNG && !IsPPM) {
    if (!filename[0]) strcpy (filename, "kapa.jpg");
    KiiJPEG (kapa, filename);
  }
  if (IsPNG) {
    if (!filename[0]) strcpy (filename, "kapa.png");
    KapaPNG (kapa, filename);
  } 
  if (IsPPM) {
    if (!filename[0]) strcpy (filename, "kapa.ppm");
    KapaPPM (kapa, filename);
  }
  return (TRUE);
}

/* jpeg converts graph to png or ppm
   jpeg converts image to jpeg */
