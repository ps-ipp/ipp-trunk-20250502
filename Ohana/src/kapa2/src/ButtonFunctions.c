# include "Ximage.h"
# include "hms_buttons.h"

int PSfunction (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);
  OHANA_UNUSED_PARAM(image);

  int status;

  status = PSit ("kapa.ps", "default", TRUE, KAPA_PS_NEWPLOT);
  return (status);
}

int PNGfunction (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);
  OHANA_UNUSED_PARAM(image);

  int status;

  status = PNGit ("kapa.png");
  return (status);
}

int JPEGfunction (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);
  OHANA_UNUSED_PARAM(image);

  int status;

  status = JPEGit24 ("kapa.jpg");
  return (status);
}

int greycolors (Graphic *graphic, KapaImageWidget *image) {
  SetColormap ("greyscale");
  CreateColorbar (image, graphic);
  SetColorScale (graphic, image);
  Remap (graphic, image);
  CreateZoom (graphic, image); 
  CreateWide (graphic, image); 
  Refresh ();
  return (TRUE);
}

int heat (Graphic *graphic, KapaImageWidget *image) {
  SetColormap ("heat");
  CreateColorbar (image, graphic);
  SetColorScale (graphic, image);
  Remap (graphic, image);
  CreateZoom (graphic, image); 
  CreateWide (graphic, image); 
  Refresh ();
  return (TRUE);
}

int rainbow (Graphic *graphic, KapaImageWidget *image) {
  SetColormap ("rainbow");
  CreateColorbar (image, graphic);
  SetColorScale (graphic, image);
  Remap (graphic, image);
  CreateZoom (graphic, image); 
  CreateWide (graphic, image); 
  Refresh ();
  return (TRUE);
}

int Recenter (Graphic *graphic, KapaImageWidget *image) {

  image[0].picture.Xc = 0.5*image[0].image[0].matrix.Naxis[0];
  image[0].picture.Yc = 0.5*image[0].image[0].matrix.Naxis[1];
 
  Remap (graphic, image);
  Refresh ();
  return (TRUE);

}

int Rescale (Graphic *graphic, KapaImageWidget *image) {

  image[0].picture.expand = 1;
  image[0].zoom.expand = 5;
  Remap (graphic, image);
  Refresh ();
  return (TRUE);

}

int RecenterRescale (Graphic *graphic, KapaImageWidget *image) {

  image[0].picture.Xc = 0.5*image[0].image[0].matrix.Naxis[0];
  image[0].picture.Yc = 0.5*image[0].image[0].matrix.Naxis[1];
  image[0].picture.expand = 1;
  image[0].zoom.expand = 5;
 
  Remap (graphic, image);
  Refresh ();
  return (TRUE);

}

int FlipImageX (Graphic *graphic, KapaImageWidget *image) {

  image[0].picture.flipx = !image[0].picture.flipx;
  image[0].zoom.flipx 	 = !image[0].zoom.flipx;
  image[0].wide.flipx 	 = !image[0].wide.flipx;
 
  Remap (graphic, image);
  CreateWide (graphic, image);
  Refresh ();
  return (TRUE);
}

int FlipImageY (Graphic *graphic, KapaImageWidget *image) {

  image[0].picture.flipy = !image[0].picture.flipy;
  image[0].zoom.flipy 	 = !image[0].zoom.flipy;
  image[0].wide.flipy 	 = !image[0].wide.flipy;
 
  Remap (graphic, image);
  CreateWide (graphic, image);
  Refresh ();
  return (TRUE);
}

int ToggleDEG (Graphic *graphic, KapaImageWidget *image) {

  image[0].DecimalDegrees = image[0].DecimalDegrees ^ TRUE;
  image[0].hms_button.bitmap = (image[0].DecimalDegrees) ? hms_bits : ddd_bits;
  StatusBox (graphic, image);
  DrawButton (graphic, &image[0].hms_button);
  FlushDisplay ();
  return (TRUE);
}

int ToggleHEX (Graphic *graphic, KapaImageWidget *image) {

  image[0].HexValue = image[0].HexValue ^ TRUE;
  StatusBox (graphic, image);
  FlushDisplay ();
  return (TRUE);
}

/*********** overlay_button functions ************/
int Overlay0 (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);

  image[0].overlay[0].active = image[0].overlay[0].active ^ TRUE;
  Refresh ();
  return (TRUE);

}

int Overlay1 (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);

  image[0].overlay[1].active = image[0].overlay[1].active ^ TRUE;
  Refresh ();
  return (TRUE);

}

int Overlay2 (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);

  image[0].overlay[2].active = image[0].overlay[2].active ^ TRUE;
  Refresh ();
  return (TRUE);

}

int Overlay3 (Graphic *graphic, KapaImageWidget *image) {
  OHANA_UNUSED_PARAM(graphic);

  image[0].overlay[3].active = image[0].overlay[3].active ^ TRUE;
  Refresh ();
  return (TRUE);

}

/* this routine is NOT independent of the number of overlays */


