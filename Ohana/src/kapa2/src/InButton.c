# include "Ximage.h"

int InButton (XButtonEvent *button_event, Button *button) {

  int answer;
  int x, y;

  x = button_event[0].x;
  y = button_event[0].y;
  
  answer = ((x >= button[0].x) && (x <= button[0].x + button[0].dx) &&
	    (y >= button[0].y) && (y <= button[0].y + button[0].dy));

  return (answer);

}

/*** make this a macro? ***/
