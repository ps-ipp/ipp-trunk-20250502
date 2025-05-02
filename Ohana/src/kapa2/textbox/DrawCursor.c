# include "Ximage.h"
# define XBorder 5
# define YBorder 2



DrawCursor (graphic, textline)    
Graphic       graphic[];
TextLine      textline[];
{

  int dx, label_width, DX, text_width, i;

  label_width  = XTextWidth (graphic[0].font, 
			     textline[0].label,
			     strlen(textline[0].label));
  
  text_width  = XTextWidth (graphic[0].font, 
			    textline[0].text,
			    strlen(textline[0].text));

  XSetForeground (graphic[0].display, graphic[0].gc, graphic[0].black);

  if (label_width + text_width + 2*XBorder > textline[0].dx) {
    DX = textline[0].dx - label_width - 2*XBorder - arrow_width;
    for (i = 1; (i < strlen (textline[0].text)) && (text_width > DX); i++) {
      text_width  = XTextWidth (graphic[0].font, 
				&textline[0].text[i],
				strlen(&textline[0].text[i]));
    }
    dx  = XTextWidth (graphic[0].font, &textline[0].text[i - 1], 
		      textline[0].cursor - i + 1) + arrow_width;
  }
  else 
    dx  = XTextWidth (graphic[0].font, textline[0].text, textline[0].cursor);

  XDrawLine (graphic[0].display, 
	     graphic[0].window, 
	     graphic[0].gc, 
	     textline[0].x + label_width + XBorder + dx, 
	     textline[0].y + YBorder,
	     textline[0].x + label_width + XBorder + dx, 
	     textline[0].y + textline[0].dy - YBorder);

}
