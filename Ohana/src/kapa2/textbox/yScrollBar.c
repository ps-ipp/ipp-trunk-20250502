# include "Ximage"
# include "ScrollBars.h"

int
yScrollBar (button_event, height, width, SB_x, SB_y, f)
XButtonEvent *button_event;
int height, width, SB_x, SB_y;
double *f;
{

  int answer, m_x, m_y;
  double f1, f2;

  m_x = button_event -> x;
  m_y = button_event -> y;
  answer = ((m_y > (height - SB_y)) && 
	    (m_x > (SB_x + SB_y)) && 
	    (m_x < (width - SB_y)));
  f1 = (double)(m_x - SB_x - SB_y);
  f2 = (double)(width - 2*SB_y - SB_x);
  *f = f1 / f2;
  return (answer);

}

	
