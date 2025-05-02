# include "photdbc.h"

/* check if line between points 0 and 1 of x1
   crosses line between points 0 and 1 of x2 */
int edge_check (double *x1, double *y1, double *x2, double *y2) {

  double theta1, theta2;
  double Theta1, Theta2;

  theta1 = opening_angle (x1[0], y1[0], x2[0], y2[0], x1[1], y1[1]); 
  theta2 = opening_angle (x1[0], y1[0], x2[0], y2[0], x2[1], y2[1]); 

  if (theta1*theta2 < 0.0) {
    return (FALSE);
  }

  if (fabs(theta1) < fabs(theta2)) {
    return (FALSE);
  }

  Theta1 = theta1;
  Theta2 = theta2;
  theta1 = opening_angle (x2[0], y2[0], x1[1], y1[1], x2[1], y2[1]); 
  theta2 = opening_angle (x2[0], y2[0], x1[1], y1[1], x1[0], y1[0]); 
  
 
  if (theta1*theta2 < 0.0) {
    return (FALSE);
  }

  if (fabs(theta1) < fabs(theta2)) {
    return (FALSE);
  }

  return (TRUE);

}

/* check if point x1,y1 is in box formed by x2[0-4] */
int corner_check (double *x1, double *y1, double *x2, double *y2) {

  int i;
  double theta;

  theta = 0;

  for (i = 0; i < 4; i++) {
    theta += opening_angle (x2[i], y2[i], x1[0], y1[0], x2[i+1], y2[i+1]); 
  }
  if (fabs(theta) > 6) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

/* returns the opening angle between the three points (2 is in middle) 
   in range -pi to pi */

double opening_angle (double x1, double y1, double x2, double y2, double x3, double y3) {

  double dx1, dy1, dx2, dy2, ct, st, theta;

  dx1 = x1 - x2;
  dy1 = y1 - y2;
  
  dx2 = x3 - x2;
  dy2 = y3 - y2;
  
  ct = (dx1*dx2 + dy1*dy2);
  st = (dx1*dy2 - dx2*dy1);

  theta = atan2 (st, ct);

  return (theta);

}

