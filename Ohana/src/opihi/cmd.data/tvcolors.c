# include "data.h"

int tvcolors (int argc, char **argv) {
  
  int N, kapa;
  char *name;
  // KapaImageData data;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  int SetNAN = FALSE;
  int red, green, blue;
  if ((N = get_argument (argc, argv, "-nan"))) {
    if (N > argc - 4) { 
      gprint (GP_ERR, "ERROR: -nan (red) (green) (blue) does not have enough arguments\n"); 
      return FALSE; 
    }
    remove_argument (N, &argc, argv);
    red = strtol (argv[N], NULL, 0);
    remove_argument (N, &argc, argv);
    green = strtol (argv[N], NULL, 0);
    remove_argument (N, &argc, argv);
    blue = strtol (argv[N], NULL, 0);
    remove_argument (N, &argc, argv);
    SetNAN = TRUE;
  }

  // use the currently-set zero,range values if not supplied
  if (!SetNAN && (argc != 2)) {
    gprint (GP_ERR, "USAGE: tvcolors (colormap) [-nan red green blue]\n");
    gprint (GP_ERR, " colormap options : greyscale, -greyscale, rainbow, heat, fullcolor, ruffcolor (also grayscale, -grayscale)\n");
    gprint (GP_ERR, " colormap file: if colormap name is given as file:path/to/file, the colormap is read from the file\n");
    gprint (GP_ERR, "   the colormap file must contain 4 columns: f R G B; each line defines a color transition.\n");
    gprint (GP_ERR, "   f: 0 - 1 defines the scale value for the transition\n");
    gprint (GP_ERR, "   R,G,B: 0 - 1 define the value of the color at the transition point\n");

    gprint (GP_ERR, "   There are 5 colormap file format options:\n");
    gprint (GP_ERR, "   * file:filename : index Red Green Blue\n");
    gprint (GP_ERR, "   * lgcy:filename : index Red Blue Green\n");
    gprint (GP_ERR, "   * csvf:filename : index,Red,Green,Blue\n");
    gprint (GP_ERR, "   * cetf:filename : fRed,fGreen,fBlue \n");
    gprint (GP_ERR, "   * cetr:filename : fRed,fGreen,fBlue \n\n");
    gprint (GP_ERR, "   The index value goes from 0.0 to 1.0 and sets transitions (must be monotonic).\n");
    gprint (GP_ERR, "   In the CET cases, the index is implicit, with 256 entries expected.\n");
    gprint (GP_ERR, "   For cetr: the index is reversed.\n");

    return (FALSE);
  }

  if (SetNAN) {
    KiiSetNanColor (kapa, red, green, blue);
  }
  if (argc == 2) {
    KiiSetColormap (kapa, argv[1]);
  }
  return (TRUE);
}
