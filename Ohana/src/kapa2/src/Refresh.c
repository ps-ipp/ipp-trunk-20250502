# include "Ximage.h"

# define USE_BUFFERED_DRAW 1

void bDrawXimage (bDrawBuffer *buffer);
void Refresh_Buffered (void);
void Refresh_Unbuffered (void);

void Refresh (void) {
  if (USE_BUFFERED_DRAW) {
    Refresh_Buffered();
  } else {
    Refresh_Unbuffered();
  }
}

void Refresh_Buffered (void) {

  int Npalette;
  Graphic *graphic;

  if (!USE_XWINDOW) return;
  // if (HAVE_BACKING) return;

  graphic = GetGraphic();
  
  // limit the png window to the min needed to contain the active graphic regions
  SectionMinBoundary (graphic);

  // is the palette reasonable in modern context?
  png_color *palette = KapaPNGPalette (&Npalette);

  bDrawBuffer *buffer = bDrawIt (palette, Npalette, 3);
  
  /* XClearWindow   (graphic.display, graphic.window); */
  XSetForeground (graphic->display, graphic->gc, graphic->back);
  XFillRectangle (graphic->display, graphic->window, graphic->gc, 0, 0, graphic->dx, graphic->dy);
  XSetForeground (graphic->display, graphic->gc, graphic->fore);
  
  // copy buffer to Xwindow as image?
  bDrawXimage (buffer);
  bDrawBufferFree (buffer);
  free (palette);

  // draw image tool for all sections
  int Nsection = GetNumberOfSections ();
  for (int i = 0; i < Nsection; i++) {
    Section *section = GetSectionByNumber (i);

    KapaImageWidget *image = section->image;
    DrawImageTool (image);

    /*** PaintOverlay is called in DrawImage ***
    if (!image) continue;
    for (int j = 0; j < NOVERLAYS; j++) {
      if (image[0].overlay[j].active) {
	PaintOverlay (graphic, image, j);
      }
    }
    */
  }

  FlushDisplay ();
}

void Refresh_Unbuffered (void) {

  int i, Nsection;
  Graphic *graphic;
  Section *section;

  if (!USE_XWINDOW) return;
  // if (HAVE_BACKING) return;

  graphic = GetGraphic();
  
  /* XClearWindow   (graphic.display, graphic.window); */
  XSetForeground (graphic->display, graphic->gc, graphic->back);
  XFillRectangle (graphic->display, graphic->window, graphic->gc, 0, 0, graphic->dx, graphic->dy);
  XSetForeground (graphic->display, graphic->gc, graphic->fore);
  
  // reset the sizes for all sections
  Nsection = GetNumberOfSections ();
  for (i = 0; i < Nsection; i++) {
    section = GetSectionByNumber (i);
    DrawSectionBG (graphic, section);

    KapaImageWidget *image = section->image;
    DrawImage (image);
    DrawImageTool (image);

    /*** the overlay is added in DrawImage  ***/
    if (image) {
      for (int j = 0; j < NOVERLAYS; j++) {
	if (image[0].overlay[j].active) {
	  PaintOverlay (graphic, image, j);
	}
      }
    }

    DrawGraph (section->graph);
  }

  FlushDisplay ();
}

void DrawSectionBG (Graphic *graphic, Section *section) {

  int Ncolors;
  int X0, Y0, dX, dY;

  if (section->bg == -1) return;

  Ncolors = KapaColormapSize();

  if (section->bg < 0) return;
  if (section->bg >= Ncolors) return;

  XSetForeground (graphic->display, graphic->gc, graphic->color[section->bg]);

  dY = graphic[0].dy * section[0].dy;
  dX = graphic[0].dx * section[0].dx;
  Y0 = graphic[0].dy - graphic[0].dy * section[0].y - dY;
  X0 = graphic[0].dx * section[0].x;

  XFillRectangle (graphic[0].display,  graphic[0].window, graphic[0].gc, X0, Y0, dX, dY);
  return;
}
