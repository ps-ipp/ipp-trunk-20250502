# include "Ximage.h"
# include "icons.h"

static Graphic *graphic = NULL;

/************** SetUpDisplay *************/
void SetUpGraphic (int *argc, char **argv) {

  Icon icon;
  char *name;
  int Ncolors;

  ALLOCATE (graphic, Graphic, 1);
  graphic->x  = 10;
  graphic->y  = 10;
  graphic->dx = 512;
  graphic->dy = 512; 

  // default values for the region window
  graphic->xwin  = 0;
  graphic->ywin  = 0;
  graphic->dxwin = graphic->dx;
  graphic->dywin = graphic->dy; 

  graphic->display      = NULL;
  graphic->visual       = NULL;
  graphic->font         = NULL;
  graphic->cube         = NULL;
  graphic->cmap         = NULL;
  graphic->color        = NULL;
  graphic->colormapName = NULL;
  graphic->pixels       = NULL;

  graphic->smooth_sigma = 0.0;

  name = CheckDisplayName (argc, argv);

  if (USE_XWINDOW) {
    graphic->display  = OpenDisplay     (name,    &graphic->screen);
    if (!graphic->display) USE_XWINDOW = FALSE;
  }

  if (!USE_XWINDOW) {
    ALLOCATE (graphic[0].pixels, unsigned long, NPIXELS_STATIC);
    ALLOCATE (graphic[0].cmap,   XColor,        NPIXELS_STATIC);
    graphic[0].Npixels = NPIXELS_STATIC;
    graphic[0].cube = NULL;
    return;
  }

  graphic->colormap = DefaultColormap (graphic->display, graphic->screen);
  graphic->depth    = DefaultDepth    (graphic->display, graphic->screen);
  graphic->visual   = DefaultVisual   (graphic->display, graphic->screen);
  if (name != NULL) free (name);

  CheckVisual (graphic, argc, argv);
  CheckColors (graphic, argc, argv);

  icon.width = icon_width;
  icon.height = icon_height;
  icon.bits = icon_bits;

  CheckGeometry (graphic, argc, argv);
  TopWindow (graphic, &icon);
  LoadFont (graphic, argc, argv, "fixed"); 

  XSetWindowBackground (graphic->display, graphic->window, graphic->back);

  SetNormalHints (graphic);
  SetWMHints (graphic, &icon);

  if (NAME_WINDOW == NULL) {
    NameWindow (graphic, "Kapa");
  } else {
    ALLOCATE (name, char, strlen(NAME_WINDOW) + 10);
    sprintf (name, "Kapa %s", NAME_WINDOW);
    NameWindow (graphic, name);
    free (name);
  }

  graphic->color = KapaX11colors (graphic->display, graphic->colormap, graphic->fore, &Ncolors);
  if (MAP_WINDOW) MapWindow (graphic);

  return;
}

Graphic *GetGraphic () {
  return graphic;
}

int SetSmoothSigma (int sock) {

  float sigma;
  KiiScanMessage (sock, "%f", &sigma);

  if (isfinite(sigma)) {
    if ((sigma <= 1.1) && (sigma >= 0.0)) {
      graphic->smooth_sigma = sigma;
    }
  }
  return TRUE;
}

void FreeGraphic () {
  free (graphic->pixels);
  free (graphic->cmap);
  free (graphic->color);
  free (graphic->colormapName);
  free (graphic);
}
