# include "Ximage.h"

/******** Here we test the Buttons specific to this program  *******/
Button *CheckButtons (XButtonEvent  *event, KapaImageWidget *image) {

  int i;
  Button *button;
  button = (Button *) NULL;

  if (InButton (event, &image[0].recenter_button)) 
    button = &image[0].recenter_button;

  if (InButton (event, &image[0].grey_button)) 
    button = &image[0].grey_button;

  if (InButton (event, &image[0].rainbow_button)) 
    button = &image[0].rainbow_button;

  if (InButton (event, &image[0].heat_button)) 
    button = &image[0].heat_button;

  if (InButton (event, &image[0].PS_button)) 
    button = &image[0].PS_button;

  if (InButton (event, &image[0].hms_button)) 
    button = &image[0].hms_button;

  if (InButton (event, &image[0].hex_button)) 
    button = &image[0].hex_button;

  if (InButton (event, &image[0].flipx_button)) 
    button = &image[0].flipx_button;

  if (InButton (event, &image[0].flipy_button)) 
    button = &image[0].flipy_button;

  for (i = 0; i < NOVERLAYS; i++) {
    if (InButton (event, &image[0].overlay_button[i])) 
      button = &image[0].overlay_button[i];
  }

  return (button);

}

/* To define a button, you must:

   0) add the button to the Image structure in structures.h
   1) place the info about the button in PositionPicture.c
   2) (make any bitmaps needed and put them in buttons.h
   3) place an entry in CheckButtons.c
   4) Add the button to Refresh.c
   5) create the button's function
   6) add it to the Makefile
   7) add it to the prototypes

*/
