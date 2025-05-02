# include "Ximage.h"

/* DirectColor doesn't seem to work, even though it is available:
   I cannot use XAllocColorCells to get pixels under DirectColor
*/

/* static int try_visual[] = {5, 3, 1, 4, 2, 0}; */

void CheckVisual (Graphic *graphic, int *argc, char **argv) {

  int i, Nfound, N;
  int isDefault;
  XVisualInfo *visual_list, visual_temp;
  unsigned long planes[3];
  XPixmapFormatValues *pixmaps;
  int Npixmaps, Npixels;

  if (DEBUG) {
    fprintf (stderr, "DirectColor: %d\n", DirectColor);
    fprintf (stderr, "PseudoColor: %d\n", PseudoColor);
    fprintf (stderr, "TrueColor:   %d\n", TrueColor);
    fprintf (stderr, "GrayScale:   %d\n", GrayScale);
    fprintf (stderr, "StaticColor: %d\n", StaticColor);
    fprintf (stderr, "StaticGray:  %d\n", StaticGray);
  }

  visual_temp.screen = graphic[0].screen;
  
  /* find available visuals */
  visual_list = XGetVisualInfo (graphic[0].display, VisualScreenMask, &visual_temp, &Nfound);
  if (Nfound == 0) {
    fprintf (stderr, "error finding useful visual\n");
    exit (2);
  }
  
  // set these based on selected visual
  isDefault = FALSE;

  // attempt to select the most desirable type of visual: TrueColor
  for (i = 0; (i < Nfound) && (visual_list[i].class != TrueColor); i++);
  if (i != Nfound) {
    // isColor = TRUE;
    if (DEBUG) fprintf (stderr, "visual class is %d\n", visual_list[i].class);
    isDefault = (graphic[0].visual == visual_list[i].visual);
    graphic[0].visual = visual_list[i].visual;
    graphic[0].dynamicColors = FALSE;
    if (DEBUG) fprintf (stderr, "got TrueColor visual\n");
    goto test_pixels;
  }

  // attempt to select the most desirable type of visual: DirectColor
  for (i = 0; (i < Nfound) && (visual_list[i].class != DirectColor); i++);
  if (i != Nfound) {
    // isColor = TRUE;
    if (DEBUG) fprintf (stderr, "visual class is %d\n", visual_list[i].class);
    isDefault = (graphic[0].visual == visual_list[i].visual);
    graphic[0].visual = visual_list[i].visual;
    graphic[0].dynamicColors = TRUE;
    if (DEBUG) fprintf (stderr, "got DirectColor visual\n");
    goto test_pixels;
  }

  // attempt to select the most desirable type of visual: PseudoColor (XXX is it still true?)
  for (i = 0; (i < Nfound) && (visual_list[i].class != PseudoColor); i++);
  if (i != Nfound) {
    // isColor = TRUE;
    if (DEBUG) fprintf (stderr, "selected visual class is %d\n", visual_list[i].class);
    isDefault = (graphic[0].visual == visual_list[i].visual);
    graphic[0].visual = visual_list[i].visual;
    graphic[0].dynamicColors = TRUE;
    if (DEBUG) fprintf (stderr, "got PseudoColor visual\n");
    goto test_pixels;
  }

  // attempt to select the most desirable type of visual: GrayScale (XXX is it still true?)
  for (i = 0; (i < Nfound) && (visual_list[i].class != GrayScale); i++);
  if (i != Nfound) {
    if (DEBUG) fprintf (stderr, "selected visual class is %d\n", visual_list[i].class);
    isDefault = (graphic[0].visual == visual_list[i].visual);
    graphic[0].visual = visual_list[i].visual;
    graphic[0].dynamicColors = TRUE;
    if (DEBUG) fprintf (stderr, "got GrayScale visual\n");
    goto test_pixels;
  }

  // attempt to select the most desirable type of visual: TrueColor (XXX is it still true?)
  for (i = 0; (i < Nfound) && (visual_list[i].class != StaticColor); i++);
  if (i != Nfound) {
    if (DEBUG) fprintf (stderr, "visual class is %d\n", visual_list[i].class);
    isDefault = (graphic[0].visual == visual_list[i].visual);
    graphic[0].visual = visual_list[i].visual;
    graphic[0].dynamicColors = FALSE;
    if (DEBUG) fprintf (stderr, "got StaticColor visual\n");
    goto test_pixels;
  }

  // attempt to select the most desirable type of visual: TrueColor (XXX is it still true?)
  for (i = 0; (i < Nfound) && (visual_list[i].class != StaticGray); i++);
  if (i != Nfound) {
    if (DEBUG) fprintf (stderr, "visual class is %d\n", visual_list[i].class);
    isDefault = (graphic[0].visual == visual_list[i].visual);
    graphic[0].visual = visual_list[i].visual;
    graphic[0].dynamicColors = FALSE;
    if (DEBUG) fprintf (stderr, "got StaticGray visual\n");
    goto test_pixels;
  }

  test_pixels:

  /* need to make a colormap if 
     1) the selected visual is not the default 
        XXX not sure if I need to / am allowed to alloc a colormap if I've grabbed the default.
     2) there are not enough colors available */

  /* NEED TO ADD A COUPLE LINES DEFINING THE VARIOUS HARD COLORS (BLACK, WHITE, AND THE OVERLAYS) */

  // allow user to force a private colormap
  if ((N = get_argument (*argc, argv, "-private"))) {
    remove_argument(N, argc, argv);
    isDefault = FALSE;
  } 

  // dynamic visual classes can accept AllocAll and can use XAllocColorCells while AllocNone must use XAllocColor
  // int allocMode = graphic[0].dynamicColors ? AllocAll : AllocNone;
  int allocMode = AllocNone;

  // allocate a private colormap, if desired or needed
  if (!isDefault) {
    if (DEBUG) fprintf (stderr, "allocated private colormap\n");
    graphic[0].colormap = XCreateColormap (graphic[0].display, RootWindow (graphic[0].display, graphic[0].screen), graphic[0].visual, allocMode);
  }

  Npixels = NPIXELS_STATIC;
  if (graphic[0].dynamicColors) {
    Npixels = NPIXELS_DYNAMIC;

    /* allocate color cells */
    ALLOCATE (graphic[0].pixels, unsigned long, Npixels);
    ALLOCATE (graphic[0].cmap,   XColor,        Npixels);
    for (graphic[0].Npixels = Npixels; graphic[0].Npixels >= 16; graphic[0].Npixels -= 4) {
      if (DEBUG) fprintf (stderr, "trying %d colors\n", (int) graphic[0].Npixels);
      if (XAllocColorCells (graphic[0].display, graphic[0].colormap, FALSE, planes, 0, graphic[0].pixels, graphic[0].Npixels)) {
	break;
      }
      for (i = 0; i < graphic[0].Npixels; i++) {
	graphic[0].cmap[i].pixel = graphic[0].pixels[i];
      }
    }

    /* insufficient cells, can we make a private colormap? */
    if (graphic[0].Npixels < 16) {
      if (!isDefault) {
	// We've already tried to allocate a colormap above...
	fprintf (stderr, "can't allocate enough cells in private colormap\n");
	exit (2);
      }
      if (DEBUG) fprintf (stderr, "can't allocate enough cells, using private colormap\n");
      graphic[0].colormap = XCreateColormap (graphic[0].display, RootWindow (graphic[0].display, graphic[0].screen), graphic[0].visual, allocMode);

      for (graphic[0].Npixels = NPIXELS_DYNAMIC; graphic[0].Npixels >= 16; graphic[0].Npixels -= 4) {
	if (DEBUG) fprintf (stderr, "trying %d colors\n", (int) graphic[0].Npixels);
	if (XAllocColorCells (graphic[0].display, graphic[0].colormap, FALSE, planes, 1, graphic[0].pixels, graphic[0].Npixels)) {
	  break;
	}
	for (i = 0; i < graphic[0].Npixels; i++) {
	  graphic[0].cmap[i].pixel = graphic[0].pixels[i];
	}
      }
      if ((N = get_argument (*argc, argv, "-colorcount"))) {
	fprintf (stderr, "kapa can grab %d colors\n", graphic[0].Npixels);
	exit (1);
      } 
      if (graphic[0].Npixels < 16) {
	fprintf (stderr, "can't even allocate enough cells in private colormap\n");
	exit (2);
      }
    }
  } else {
    // XXX allocate the unsigned long *pixels here and above
    ALLOCATE (graphic[0].pixels, unsigned long, Npixels);
    ALLOCATE (graphic[0].cmap,   XColor,        Npixels);
    graphic[0].Npixels = Npixels;
  }

  if ((N = get_argument (*argc, argv, "-colorcount"))) {
    fprintf (stderr, "kii can grab %d colors\n", graphic[0].Npixels);
    exit (1);
  } 
  
  pixmaps = XListPixmapFormats (graphic[0].display, &Npixmaps);
  for (i = 0; i < Npixmaps; i++) {
    if (pixmaps[i].depth == graphic[0].depth) {
      graphic[0].Nbits = pixmaps[i].bits_per_pixel;
    }
  }
}
