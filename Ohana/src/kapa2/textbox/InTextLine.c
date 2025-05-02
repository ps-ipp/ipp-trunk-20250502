# include "Ximage.h"
# define XBorder 5
# define YBorder 2

int
InTextLine (graphic, event, textline)
Graphic          graphic[];
XButtonEvent    *event;
TextLine         textline[];
{

  int answer, done;
  int i, x, dx, y, text_width, label_width;
  int minX, maxX, minY, maxY;
  char testline[1000];

  x = event[0].x;
  y = event[0].y;
  
  label_width  = XTextWidth (graphic[0].font, 
			     textline[0].label,
			     strlen(textline[0].label));
  
  minX = textline[0].x + XBorder + label_width;
  maxX = textline[0].x + textline[0].dx;
  minY = textline[0].y;
  maxY = textline[0].y + textline[0].dy;

  answer = ((x >= minX) && (x <= maxX) && (y >= minY) && (y <= maxY));

  /* find the cursor position and draw a vertical bar */
  if (answer) {
    text_width  = XTextWidth (graphic[0].font, 
			      textline[0].text,
			      strlen(textline[0].text));
    
    if (x >= minX + text_width) {
      textline[0].cursor = strlen (textline[0].text);
    }
    else {
      for (i = 1, done = FALSE; (i <= strlen (textline[0].text)) && !done; i++) {
	dx  = XTextWidth (graphic[0].font, 
			  textline[0].text, i);
	if (x < minX + dx) {
	  textline[0].cursor = i - 1;
	  done = TRUE;
	}
      }
    }
    DrawCursor (graphic, textline);
  }    
  XFlush (graphic[0].display);
  
  return (answer);
  
}


