# include "Ximage.h"

static char default_cmap[] = "grayscale";
// static char default_cmap[] = "heat";

void MakeColormap (int argc, char **argv) {

  int i, N, status;
  char *temp_name;
  Graphic *graphic;

  graphic = GetGraphic();

  /* hardwired colors - white, black -- for drawing text and overlay graphics */
  temp_name = XGetDefault (graphic[0].display, argv[0], "ROverlay");
  graphic[0].overlay_color[0] = GetColor (graphic[0].display, (temp_name == (char *) NULL ? "red"    : temp_name), graphic[0].colormap, graphic[0].fore);
  temp_name = XGetDefault (graphic[0].display, argv[0], "GOverlay");
  graphic[0].overlay_color[1] = GetColor (graphic[0].display, (temp_name == (char *) NULL ? "green"  : temp_name), graphic[0].colormap, graphic[0].fore);
  temp_name = XGetDefault (graphic[0].display, argv[0], "BOverlay");
  graphic[0].overlay_color[2] = GetColor (graphic[0].display, (temp_name == (char *) NULL ? "blue"   : temp_name), graphic[0].colormap, graphic[0].fore);
  temp_name = XGetDefault (graphic[0].display, argv[0], "YOverlay");
  graphic[0].overlay_color[3] = GetColor (graphic[0].display, (temp_name == (char *) NULL ? "yellow" : temp_name), graphic[0].colormap, graphic[0].fore);

  for (i = 0; i < graphic[0].Npixels; i++) {
    graphic[0].cmap[i].pixel = graphic[0].pixels[i];
  }

  /* decide on a color map */
  temp_name = XGetDefault (graphic[0].display, argv[0], "Colormap");
  if ((N = get_argument (argc, argv, "-cm"))) {
    if (N + 1 < argc) {
      temp_name = argv[N+1];
    } else {
      fprintf (stderr, "error: usage is -cm ColormapName\n");
      exit (1);
    }
  }
  if (temp_name == (char *) NULL) temp_name = default_cmap;

  status = SetColormap (temp_name);
  if (!status) {
    fprintf (stderr, "invalid colormap, using default %s\n", default_cmap);
    temp_name = default_cmap;
    status = SetColormap (temp_name);
    if (!status) {
      fprintf (stderr, "problem with default colormap\n");
      exit (1);
    }
  }
}

/* this routine is NOT independent of the number of overlays */

