# include "Ximage.h"
# define XBorder 5
# define YBorder 2

int 
TextLineEntry (graphic, textline, keyevent)
Graphic        graphic[];
TextLine       textline[];
XKeyEvent     *keyevent;
{

  int i, status, N;
  char dummy[3];
  XComposeStatus compose_status;
  KeySym keysym;

  status = TRUE;
  strcpy (textline[0].old_text, textline[0].text);
  bzero (&textline[0].text[strlen(textline[0].text)], 
	 1024 - strlen(textline[0].text));
  i = textline[0].cursor;


  N = XLookupString (keyevent, dummy, 1, &keysym, &compose_status);
  switch (keysym) {
  case XK_BackSpace:
  case XK_Delete:
    if (textline[0].cursor > 0) {
      bcopy (&textline[0].text[i], &textline[0].text[i - 1], 
	     strlen(&textline[0].text[i]) + 1);
      RedrawString (graphic, textline);
      textline[0].cursor --;
      DrawCursor (graphic, textline);
    }
    break;
  case XK_Left:
    RedrawString (graphic, textline);
    textline[0].cursor --;
    if (textline[0].cursor < 0)
      textline[0].cursor = 0;
    DrawCursor (graphic, textline);
    break;
  case XK_Right:
    RedrawString (graphic, textline);
    textline[0].cursor ++;
    if (textline[0].cursor > strlen (textline[0].text))
      textline[0].cursor = strlen (textline[0].text);
    DrawCursor (graphic, textline);
    break;
  case XK_Return:
    RedrawString (graphic, textline);
    textline[0].cursor = -1;
    status = FALSE;
    break;
  case XK_d:
  case XK_D: 
    if (dummy[0] == 4) { /* a control-d was typed! */
      if (textline[0].cursor < strlen(textline[0].text)) {
	bcopy (&textline[0].text[i + 1], &textline[0].text[i], 
	       strlen(&textline[0].text[i]));
	RedrawString (graphic, textline);
	DrawCursor (graphic, textline);
      }
      break;  /* WARNING: this case MUST come before default.
		 anything between this and default will be executed
		 if no cntl-d is typed! */
    }
  default:
    if (N != 0) {
      bcopy (&textline[0].text[i], &textline[0].text[i + 1], 
	     strlen(&textline[0].text[i]));
      textline[0].text[i] = dummy[0];
      RedrawString (graphic, textline);
      textline[0].cursor ++;
      DrawCursor (graphic, textline);
    }
  }
  return (status);
}


