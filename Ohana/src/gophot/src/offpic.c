# include "gophot.h"

bool offpic (float *star, int ix, int iy, float *dx, float *dy) {

  bool nogood;
  float x, y;

  x = ix + star[2];
  y = iy + star[3];
  
  *dx = 0;
  if (x < 0) *dx = -x;
  if (x > nfast) *dx = x - nfast;
	
  *dy = 0;
  if (y < 0) *dy = -y;
  if (y > nslow) *dy = y - nslow;
	
  nogood = (*dx != 0) || (*dy != 0);

  if (!fixpos) nogood = nogood || (star[1] < 0);

  return (nogood);
}
