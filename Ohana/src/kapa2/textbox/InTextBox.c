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

/******************* InTextBox ****************/
int
InTextBox (graphic, event, textbox)
Graphic          graphic[];
XButtonEvent    *event;
TextBox          textbox[];
{

  int answer;
  int i, x, y, Y, X, dy;
  int minX, maxX, minY, maxY;
  
  x = event[0].x;
  y = event[0].y;
  dy = graphic[0].font[0].ascent + graphic[0].font[0].descent;
  
  minX = textbox[0].x;
  maxX = textbox[0].x + textbox[0].dx;
  minY = textbox[0].y;
  maxY = textbox[0].y + textbox[0].dy;
  if (dy*textbox[0].Nlines + 2*YBorder < textbox[0].dy)
    maxY = textbox[0].y + dy*textbox[0].Nlines + 2*YBorder;

  answer = ((x >= minX) && (x <= maxX) && (y >= minY) && (y <= maxY));
  
  /* find the cursor position, erase the old arrow and draw a new arrow */
  if (answer) {
    if (textbox[0].cursor_line != -1) {
      Y = textbox[0].y + dy*textbox[0].cursor_line + YBorder + (dy - arrow_height)/2;
      X = textbox[0].x + (XBorder - arrow_width) / 2;
      DrawBitmap (graphic, X, Y, arrow_width, arrow_height, arrow_bits, 0);
    }
    textbox[0].cursor_line = (y - textbox[0].y - YBorder)/dy;
    X = textbox[0].x + (XBorder - arrow_width) / 2;
    Y = textbox[0].y + dy*textbox[0].cursor_line + YBorder + (dy - arrow_height)/2;
    DrawBitmap (graphic, X, Y, arrow_width, arrow_height, arrow_bits, 1);
    textbox[0].cursor_x = x;
    textbox[0].cursor_y = y;
    XFlush (graphic[0].display);
  }
  
  return (answer);
  
}

