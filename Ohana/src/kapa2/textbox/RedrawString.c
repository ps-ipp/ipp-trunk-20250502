# include "Ximage.h"
# define XBorder 5
# define YBorder 2


#define arrow_width 6
#define arrow_height 9
RedrawString (graphic, textline)
Graphic       graphic[];
TextLine      textline[];
{

  int label_width, text_width, y, i, DX;
  static char arrow_bits[] = {
    0x00, 0x10, 0x18, 0x1c, 0x1e, 0x1c, 0x18, 0x10, 0x00};
  
  label_width  = XTextWidth (graphic[0].font, 
			     textline[0].label,
			     strlen(textline[0].label));
  
  text_width  = XTextWidth (graphic[0].font, 
			    textline[0].text,
			    strlen(textline[0].text));

  if (label_width + text_width + 2*XBorder > textline[0].dx) {
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].white);
    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, 
		    textline[0].x + XBorder + label_width, 
		    textline[0].y + YBorder, 
		    textline[0].dx - XBorder - label_width, 
		    textline[0].dy - 2*YBorder + 1);
    
    DX = textline[0].dx - label_width - 2*XBorder - arrow_width;
    for (i = 1; (i < strlen (textline[0].text)) && (text_width > DX); i++) {
      text_width  = XTextWidth (graphic[0].font, 
				&textline[0].text[i],
				strlen(&textline[0].text[i]));
    }

    y = textline[0].y + (textline[0].dy + graphic[0].font[0].ascent) / 2;
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].black);
    DrawBitmap (graphic, 
		textline[0].x + XBorder + label_width,
		textline[0].y + YBorder + (textline[0].dy - arrow_height)/2,
		arrow_width, arrow_height, arrow_bits, 1);
    XDrawString (graphic[0].display, 
		 graphic[0].window, 
		 graphic[0].gc, 
		 textline[0].x + XBorder + label_width + arrow_width, y,
		 &textline[0].text[i - 1], 
		 strlen(&textline[0].text[i - 1]));
    XFlush (graphic[0].display);
  }
  else {
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].white);
    XFillRectangle (graphic[0].display, graphic[0].window, graphic[0].gc, 
		    textline[0].x + XBorder + label_width, 
		    textline[0].y + YBorder, 
		    textline[0].dx - XBorder - label_width, 
		    textline[0].dy - 2*YBorder + 1);
    
    y = textline[0].y + (textline[0].dy + graphic[0].font[0].ascent) / 2;
    
    XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].black);
    XDrawString (graphic[0].display, 
		 graphic[0].window, 
		 graphic[0].gc, 
		 textline[0].x + XBorder + label_width, y,
		 textline[0].text, 
		 strlen(textline[0].text));
  }

}
