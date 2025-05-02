# include "Ximage.h"
# define XBorder 10
# define YBorder 2
# define arrow_width 6
# define arrow_height 9
static char arrow_bits[] = {
  0x00, 0x02, 0x06, 0x0e, 0x1e, 0x0e, 0x06, 0x02, 0x00};
#define down_width 9
#define down_height 6
static char down_bits[] = {
   0x00, 0x00, 0xfe, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x10, 0x00, 0x00, 0x00};

/******************* DrawTextBox ****************/
DrawTextBox (graphic, textbox)
Graphic      graphic[];
TextBox      textbox[];
{

  int X, Y, y, dy, Nlines, i;

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].white);
  XFillRectangle (graphic[0].display, 
		  graphic[0].window,
		  graphic[0].gc,
		  textbox[0].x  + 1,  textbox[0].y + 1,
		  XBorder - 2, textbox[0].dy - 2);
  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].black);
  if (textbox[0].outline) {
    XDrawRectangle (graphic[0].display, 
		    graphic[0].window,
		    graphic[0].gc,
		    textbox[0].x,  textbox[0].y,
		    textbox[0].dx, textbox[0].dy);
  }

  dy = graphic[0].font[0].ascent + graphic[0].font[0].descent;
  Nlines = (textbox[0].dy - 2*YBorder) / dy;

  for (i = 0; (i < Nlines) && (i < textbox[0].Nlines); i++) {
    y = textbox[0].y + graphic[0].font[0].ascent + dy*i + YBorder;
    XDrawString (graphic[0].display, 
		 graphic[0].window, 
		 graphic[0].gc, 
		 textbox[0].x + XBorder, y,
		 textbox[0].text[i], 
		 strlen(textbox[0].text[i]));
  }

  if (Nlines < textbox[0].Nlines) {
    DrawBitmap (graphic, 
		textbox[0].x + textbox[0].dx - down_width - 2,
		textbox[0].y + textbox[0].dy - down_height - 2,
		down_width, down_height, down_bits, 1);
  }

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].black);
  if (textbox[0].cursor_line != -1) {
    Y = textbox[0].y + dy*textbox[0].cursor_line + YBorder + (dy - arrow_height)/2;
    X = textbox[0].x + (XBorder - arrow_width) / 2;
    DrawBitmap (graphic, X, Y, arrow_width, arrow_height, arrow_bits, 1);
  }
}

