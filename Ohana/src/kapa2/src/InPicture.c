# include "Ximage.h"

int InPicture (XButtonEvent *button_event, Picture *picture) {

  int answer;
  int x, y;

  x = button_event[0].x;
  y = button_event[0].y;
  
  answer = ((x >= picture[0].x) && (x <= picture[0].x + picture[0].dx) &&
	    (y >= picture[0].y) && (y <= picture[0].y + picture[0].dy));

  return (answer);

}
