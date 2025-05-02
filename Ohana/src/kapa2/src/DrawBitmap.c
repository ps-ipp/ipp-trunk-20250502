# include "Ximage.h"

void DrawBitmap (Graphic *graphic, int x, int y, int dx, int dy, unsigned char *bitmap, int mode) {

  int i, j, byte_line, byte, bit, flag;
  unsigned long int fore, back;

  fore = graphic[0].fore;
  back = graphic[0].back;

  if (mode == 0) {
    fore = graphic[0].fore;
    back = graphic[0].back;
    graphic[0].fore = back;
    graphic[0].back = back;
  }
    
  if (mode == 2) {
    fore = graphic[0].fore;
    back = graphic[0].back;
    graphic[0].fore = back;
    graphic[0].back = fore;
  }
    
  
  byte_line = (int) ((dx + 7) / 8);
  for (i = 0; i < dy; i++) {
    for (j = 0; j < dx; j++) {
      byte = byte_line * i + (j / 8);
      bit = j % 8;
      flag = 0x01 & (bitmap[byte] >> bit);
      if (flag)
	XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
      else 
	XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].back);
      XDrawPoint (graphic[0].display, graphic[0].window, 
		  graphic[0].gc, x + j, y + i);
    }
  }
  if (mode == 0) {
    graphic[0].fore = fore;
    graphic[0].back = back;
  }
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].fore);
}

