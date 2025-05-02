# include "Ximage.h"

void InvertButton (Graphic *graphic, Button *button) {

  unsigned long fore, back;

  fore =  graphic[0].fore;
  back =  graphic[0].back;

  graphic[0].fore = back;
  graphic[0].back = fore;

  DrawButton (graphic, button);
  FlushDisplay ();
  
  graphic[0].fore = fore;
  graphic[0].back = back;

}
