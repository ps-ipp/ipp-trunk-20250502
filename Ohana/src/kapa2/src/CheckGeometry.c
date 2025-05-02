# include "Ximage.h"

/************** CheckGeometry *************/
void CheckGeometry (Graphic *graphic, int *argc, char **argv) {

  int status, N;
  char *temp_name;
  
  temp_name = XGetDefault (graphic->display, argv[0], "geometry");
  if ((N = get_argument (*argc, argv, "-geom"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is -geom DisplayName\n");
      exit (2);
    }
    remove_argument (N, argc, argv);
    temp_name = argv[N];
    remove_argument (N, argc, argv);
  }
  if (temp_name == NULL) return;

  status = XParseGeometry (temp_name, &graphic->x, &graphic->y, &graphic->dx, &graphic->dy);
  if (status & XNegative) graphic->x += DisplayWidth  (graphic->display, graphic->screen) - graphic->dx;
  if (status & YNegative) graphic->y += DisplayHeight (graphic->display, graphic->screen) - graphic->dy;

  return;
}
