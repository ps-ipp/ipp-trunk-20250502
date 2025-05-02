# include "Ximage"
# include "ScrollBars.h"

int 
xScrollBar (button_event, height, width, SB_x, SB_y, f)
XButtonEvent *button_event;
int height, width, SB_x, SB_y;
double *f;
{

  int answer, m_x, m_y;
  double f1, f2;

  m_x = button_event -> x;
  m_y = button_event -> y;
  answer = ((m_x < SB_x) && 
	    (m_y > SB_x) && 
	    (m_y < (height - SB_x - SB_y)));
  f1 = (double)(m_y - SB_x);
  f2 = (double)(height - 2*SB_x - SB_y);
  *f = f1 / f2;
  return (answer);

}


