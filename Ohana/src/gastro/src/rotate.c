# include "gastro.h"

void rotate (SStars *stars, int Nstars, double angle, int Xo, int Yo) {
  
  int i;
  double dX, dY, DX, DY, CS, SN;
  double theta, theta2;

  if (angle == 0.0) {
    return;
  }

  theta = (angle*RAD_DEG);

  if (fabs (angle) < 10) {
    theta2 = 0.5*theta*theta;
    for (i = 0; i < Nstars; i++) {
      dX = (stars[i].X - Xo);
      dY = (stars[i].Y - Yo);
      stars[i].X += -theta*dY - theta2*dX;
      stars[i].Y +=  theta*dX - theta2*dY;
    }
  } else {

    CS = cos (theta);
    SN = sin (theta);

    for (i = 0; i < Nstars; i++) {
      dX = (stars[i].X - Xo);
      dY = (stars[i].Y - Yo);

      DX = dX * CS - dY * SN;
      DY = dX * SN + dY * CS;

      stars[i].X = DX + Xo;
      stars[i].Y = DY + Yo;
    }
  }
    
}


  /* rotate the list 'stars' by an angle ccw from x axis */

