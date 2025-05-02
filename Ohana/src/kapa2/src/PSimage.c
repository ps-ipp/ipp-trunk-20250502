# include "Ximage.h"

int PSimage (KapaImageWidget *image, FILE *f) {
  
  int i;
  Graphic *graphic;

  if (!USE_XWINDOW) {
    fprintf (stderr, "PSimage not working yet for no X mode\n");
    return (TRUE);
  }

  graphic = GetGraphic();

  // Update this to generate color PS images: I need to test if the image if color or BW (based
  // on the colormap).  If it is a color image, I need to generate a 24 bit image. the header
  // should look the same (Nx Ny 8 [1 0 0 1 0 0]) but then it should finish with "false 3
  // colorimage" instead of just "image" (see ../doc/color.image.s for an example).  the hex
  // string needs to have RGB represented as chars (2 hex chars per R,G,B). 
  // the values come from the following map: image[0].pixmap[i] -> cmap[pixel].red -> R (0-255)

  if (0) {
    fprintf (f, " newpath %d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath\n\n", 
	     (int) image[0].picture.x,                       graphic->dy - (int) image[0].picture.y, 
	     (int) image[0].picture.x + image[0].picture.dx, graphic->dy - (int) image[0].picture.y, 
	     (int) image[0].picture.x + image[0].picture.dx, graphic->dy - (int) image[0].picture.y - image[0].picture.dy, 
	     (int) image[0].picture.x,                       graphic->dy - (int) image[0].picture.y - image[0].picture.dy); 
  }

  fprintf (f, "gsave %% encloses image\n");
  fprintf (f, "%d %d translate\n", (int) image[0].picture.x, graphic->dy - (int) image[0].picture.y - image[0].picture.dy);
  fprintf (f, "%d %d 8\n", image[0].picture.dx, image[0].picture.dy);
  fprintf (f, "[1 0 0 -1 0 %d]\n", image[0].picture.dy);
  // write out the image in normal order, but flip in PS

# if (0)

  fprintf (f, "{currentfile %d string readhexstring pop} image\n\n", image[0].picture.dx);
  PSPixmap_1byte (graphic, image, f);

# else

  fprintf (f, "{currentfile %d string readhexstring pop} false 3 colorimage\n\n", 3*image[0].picture.dx);
  PSPixmap_3byte (graphic, image, f);

# endif

  fprintf (f, "grestore %% end of image\n");
  fprintf (f, "stroke\n");

  // should we have a 'gsave / grestore' container here?
  fprintf (f, "%% plot overlay objects\n");
  fprintf (f, "1 setgray\n");

  for (i = 0; i < NOVERLAYS; i++) {
    if (image[0].overlay[i].active) {
      fprintf (f, "%% overlay %d\n", i);
      PSOverlay (image, i, f, 0);
    }
  }
  fprintf (f, "0 setgray\n");
  for (i = 0; i < NOVERLAYS; i++) {
    if (image[0].overlay[i].active) {
      fprintf (f, "%% overlay %d\n", i);
      PSOverlay (image, i, f, 1);
    }
  }

  return (TRUE);
}
