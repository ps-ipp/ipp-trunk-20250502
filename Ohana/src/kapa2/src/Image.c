# include "Ximage.h"
# include "buttons.h"
# include "hms_buttons.h"

int InitImageChannel (KapaImageChannel *channel) {

  /** set up a bunch of default things **/
  channel->zero = 0;
  channel->range = 1;
  channel->start = 0;
  channel->slope = 1;

  InitCoords (&channel->coords, "DEC--LIN");

  channel->matrix.datasize = 0; /* a flag to show there is no data in the matrix */
  ALLOCATE (channel->matrix.buffer, char, 1);  /* allocate so later free will not crash! */

  return (TRUE);
}

/* initialization for things not specific to X */
KapaImageWidget *InitImageWidget () {

  int i;
  KapaImageWidget *image;
  Graphic *graphic;

  graphic = GetGraphic ();

  ALLOCATE (image, KapaImageWidget, 1);
  memset (image, 0, sizeof(KapaImageWidget));

  for (i = 0; i < NCHANNELS; i++) {
    InitImageChannel (&image[0].channel[i]);
  }

  image[0].currentChannel = 0;
  image[0].image = &image[0].channel[image[0].currentChannel];

  image[0].nPixels = 0;
  ALLOCATE (image[0].pixmap, unsigned short, 1);  /* allocate so later free will not crash! */

  // XXXX this has been moved to graphic, which may be wrong
  // image[0].ColorScaleMode = KAPA_SCALE_1D;

  for (i = 0; i < NOVERLAYS; i++) {
    image[0].overlay[i].Nobjects = 0;
    ALLOCATE (image[0].overlay[i].objects, KiiOverlay, 1);  /* allocate so later free will not crash! */
    image[0].overlay[i].active = FALSE;
    image[0].overlay[i].color = graphic[0].overlay_color[i];
  }

  // set the center and expansion for the pictures:
  image[0].picture.Xc     = 0.0;
  image[0].picture.Yc     = 0.0;
  image[0].picture.expand = 1;
  image[0].picture.flipx  = FALSE;
  image[0].picture.flipy  = FALSE;

  image[0].zoom.Xc     	  = 0.0;
  image[0].zoom.Yc     	  = 0.0;
  image[0].zoom.expand 	  = 5;
  image[0].zoom.flipx  	  = FALSE;
  image[0].zoom.flipy  	  = FALSE;

  image[0].wide.Xc     	  = 0.0;
  image[0].wide.Yc     	  = 0.0;
  image[0].wide.expand 	  = -5;
  image[0].wide.flipx  	  = FALSE;
  image[0].wide.flipy  	  = FALSE;

  image[0].location = 4;

  image[0].MovePointer = TRUE;
  image[0].DecimalDegrees  = TRUE;
  image[0].HexValue  = FALSE;

  ALLOCATE (image[0].picture.data, char, 1);   /* allocate so later free will not crash! */
  ALLOCATE (image[0].cmapbar.data, char, 1);   /* allocate so later free will not crash! */
  ALLOCATE (image[0].zoom.data, char, 1);      /* allocate so later free will not crash! */
  ALLOCATE (image[0].wide.data, char, 1);      /* allocate so later free will not crash! */

  InitButtonSize (&image[0].PS_button, PS_width, PS_height, PS_bits);
  // InitButtonFunc (&image[0].PS_button, PSfunction);
  image->PS_button.function_1 = PSfunction;
  image->PS_button.function_2 = PNGfunction;
  image->PS_button.function_3 = JPEGfunction;

  InitButtonSize (&image[0].grey_button, grey_width, grey_height, grey_bits);
  InitButtonFunc (&image[0].grey_button, greycolors);

  InitButtonSize (&image[0].rainbow_button, rainbow_width, rainbow_height, rainbow_bits);
  InitButtonFunc (&image[0].rainbow_button, rainbow);

  InitButtonSize (&image[0].heat_button, heat_width, heat_height, heat_bits);
  InitButtonFunc (&image[0].heat_button, heat);

  InitButtonSize (&image[0].recenter_button, recenter_width, recenter_height, recenter_bits);
  image[0].recenter_button.function_1 = Recenter;
  image[0].recenter_button.function_2 = RecenterRescale;
  image[0].recenter_button.function_3 = Rescale;

  InitButtonSize (&image[0].overlay_button[0], red_width, red_height, red_bits);
  InitButtonFunc (&image[0].overlay_button[0], Overlay0);

  InitButtonSize (&image[0].overlay_button[1], green_width, green_height, green_bits);
  InitButtonFunc (&image[0].overlay_button[1], Overlay1);

  InitButtonSize (&image[0].overlay_button[2], blue_width, blue_height, blue_bits);
  InitButtonFunc (&image[0].overlay_button[2], Overlay2);

  InitButtonSize (&image[0].overlay_button[3], yellow_width, yellow_height, yellow_bits);
  InitButtonFunc (&image[0].overlay_button[3], Overlay3);

  if (image[0].DecimalDegrees) {
    InitButtonSize (&image[0].hms_button, hms_width, hms_height, hms_bits);
  } else {
    InitButtonSize (&image[0].hms_button, ddd_width, ddd_height, ddd_bits);
  }
  InitButtonFunc (&image[0].hms_button, ToggleDEG);

  InitButtonSize (&image[0].hex_button, hex_width, hex_height, hex_bits);
  InitButtonFunc (&image[0].hex_button, ToggleHEX);

  InitButtonSize (&image[0].flipx_button, flipx_width, flipx_height, flipx_bits);
  InitButtonFunc (&image[0].flipx_button, FlipImageX);

  InitButtonSize (&image[0].flipy_button, flipy_width, flipy_height, flipy_bits);
  InitButtonFunc (&image[0].flipy_button, FlipImageY);

  return (image);
}

void InitButtonSize (Button *button, int width, int height, unsigned char *bitmap) {
  button->dx = width + 2;
  button->dy = height + 2;
  button->width = width;
  button->height = height;
  button->bitmap = bitmap;
}

void InitButtonFunc (Button *button, int (*function)()) {
  button->function_1 = function;
  button->function_2 = function;
  button->function_3 = function;
}

void DrawImage (KapaImageWidget *image) {

  Graphic *graphic;

  if (image == NULL) return;

  graphic = GetGraphic ();

  if (image[0].picture.pix) {
    XPutImage (graphic[0].display, graphic[0].window, graphic[0].gc,
	       image[0].picture.pix, 0, 0, 
	       image[0].picture.x + 1, image[0].picture.y + 1, 
	       image[0].picture.dx, image[0].picture.dy);
  }
}

// add the zoom, pan, crosshairs, status box, buttons
void DrawImageTool (KapaImageWidget *image) {

  int i;
  Graphic *graphic;

  if (image == NULL) return;

  graphic = GetGraphic ();

  XSetForeground (graphic[0].display,  graphic[0].gc, graphic[0].fore);
  XDrawRectangle (graphic[0].display,  graphic[0].window, graphic[0].gc, 
		  image[0].picture.x,  image[0].picture.y, 
		  image[0].picture.dx+1, image[0].picture.dy+1);
  
  if (image[0].location) {
    if (image[0].cmapbar.pix) {
      XPutImage (graphic[0].display, graphic[0].window, graphic[0].gc,
		 image[0].cmapbar.pix, 0, 0, 
		 image[0].cmapbar.x, image[0].cmapbar.y, 
		 image[0].cmapbar.dx, image[0].cmapbar.dy);
    }
    if (image[0].wide.pix) {
	XPutImage (graphic[0].display, graphic[0].window, graphic[0].gc,
		   image[0].wide.pix, 0, 0, 
		   image[0].wide.x, image[0].wide.y, 
		   image[0].wide.dx, image[0].wide.dy);
    }

    CrossHairs (graphic, &image[0].zoom);
  
    /* erase everything below zoom box, then draw */
    /*
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, 
		    image[0].text_x, image[0].text_x, image[0].text_dx, image[0].text_dy); 
    */    
    DrawButton (graphic, &image[0].PS_button);
    DrawButton (graphic, &image[0].recenter_button);
    DrawButton (graphic, &image[0].grey_button);
    DrawButton (graphic, &image[0].rainbow_button);
    DrawButton (graphic, &image[0].heat_button);
    DrawButton (graphic, &image[0].hms_button);
    DrawButton (graphic, &image[0].hex_button);

    DrawButton (graphic, &image[0].flipx_button);
    DrawButton (graphic, &image[0].flipy_button);

    for (i = 0; i < NOVERLAYS; i++) {
      DrawButton (graphic, &image[0].overlay_button[i]);
    }
    StatusBox (graphic, image);
  }
}

void FreeImage (KapaImageWidget *image) {

  int i;

  if (image == NULL) return;

  for (i = 0; i < NOVERLAYS; i++) {
    free (image[0].overlay[i].objects);
  }
  for (i = 0; i < NCHANNELS; i++) {
    free (image[0].channel[i].matrix.buffer);
  }
  free (image[0].pixmap);
  free (image[0].picture.data);
  free (image[0].cmapbar.data);
  free (image[0].zoom.data);
  free (image[0].wide.data);

  free (image);
}
