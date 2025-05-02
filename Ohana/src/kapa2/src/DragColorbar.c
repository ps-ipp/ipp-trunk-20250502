# include "Ximage.h"
/* future expansion: warp mouse to the right spot for the current settings */

void DragColorbar (Graphic *graphic, KapaImageWidget *image, XButtonEvent *mouse_event) {

  double          frac_x, oldfrac_x, frac_y, oldfrac_y, start, slope;
  int             X, Y;
  XEvent          event;
  XMotionEvent   *move;
  int             xstatus, Npix, center;

  if (!graphic[0].dynamicColors) return;

  X = mouse_event[0].x;
  Y = mouse_event[0].y;
  Npix = graphic[0].Npixels;
  slope = image[0].image[0].slope;
  start = image[0].image[0].start;
  oldfrac_x = oldfrac_y = 10;
  
  while (1) {
    frac_x = X / (1.0*graphic[0].dx);
    frac_x = MAX (0, frac_x);
    frac_x = MIN (1, frac_x);
    frac_y = Y / (1.0*graphic[0].dy);
    frac_y = MAX (0, frac_y);
    frac_y = MIN (1, frac_y);
    switch (mouse_event[0].button) {
    case 1:
      center = frac_x*Npix;
      slope = Npix / (1 + 4*frac_y*frac_y*Npix);
      start = Npix/2.0 - center*slope;
      break;
    case 2:
      start = 0;
      slope = 1;
      break;
    case 3:
      center = (Npix/2.0 - start) / slope;
      slope = Npix / (1 + 4*frac_x*frac_x*Npix);
      start = Npix/2.0 - center*slope;
      break;
    }

    if ((frac_x != oldfrac_x) || (frac_y != oldfrac_y)) {
      ResetColorbar (graphic, start, slope);
    }
    oldfrac_x = frac_x;
    oldfrac_y = frac_y;
      
    if ((xstatus = XCheckMaskEvent (graphic[0].display, EVENT_MASK, &event))) {
      
      switch (event.type)  {
	
      case MotionNotify:
	if ((xstatus = XPending (graphic[0].display)) < 2) {
	  move = (XMotionEvent *) &event;
	  X = move[0].x;
	  Y = move[0].y;
	}
	break;
	
      case ButtonPress:
      case ButtonRelease:
	image[0].image[0].start = start;	
	image[0].image[0].slope = slope;
	return;
	break;
	
      }
    }
  }
  
}

void ResetColorbar (Graphic *graphic, double start, double slope) {

  int i, j;
  XColor cmap[256];

  if (!graphic[0].dynamicColors) return;

  for (i = 0; i < graphic[0].Npixels; i++) {
    cmap[i] = graphic[0].cmap[i];
    j = start + i * slope;
    if (j < 0) {
      cmap[i].red = graphic[0].cmap[0].red;
      cmap[i].blue = graphic[0].cmap[0].blue;
      cmap[i].green = graphic[0].cmap[0].green;
      cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    else {
      if (j >= graphic[0].Npixels) {
	cmap[i].red = graphic[0].cmap[graphic[0].Npixels-1].red;
	cmap[i].blue = graphic[0].cmap[graphic[0].Npixels-1].blue;
	cmap[i].green = graphic[0].cmap[graphic[0].Npixels-1].green;
	cmap[i].flags = DoRed | DoGreen | DoBlue;
      }
      else {
	cmap[i].red = graphic[0].cmap[j].red;
	cmap[i].blue = graphic[0].cmap[j].blue;
	cmap[i].green = graphic[0].cmap[j].green;
	cmap[i].flags = DoRed | DoGreen | DoBlue;
      }
    }
  }

  XStoreColors(graphic[0].display, graphic[0].colormap, cmap, graphic[0].Npixels);
}
