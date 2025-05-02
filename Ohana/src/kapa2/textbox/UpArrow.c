# include "Ximage"
# include "ScrollBars.h"

int
UpArrow (button_event, height, width, SB_x, SB_y) 
XButtonEvent *button_event;
int height, width, SB_x, SB_y;
{

  int answer, m_x, m_y;

  m_x = button_event -> x;
  m_y = button_event -> y;
  answer = ((m_y > (height - SB_y)) && 
	    (m_x > SB_x) && 
	    (m_x < (SB_x + SB_y)));
  return (answer);

}

