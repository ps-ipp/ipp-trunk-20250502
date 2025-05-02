# include <math.h>
# include <stdio.h>
# define SQ(X)    (double) (((double)(X))*((double)(X)))

int ellipse (float Sx, float Sxy, float Sy,
	     float *area, float *amaj, float *amin, float *angle) {

  float A1, A2, A3, R, root1, root2;

  if (fabs(Sxy) >= 1.0 / sqrt(fabs(Sx*Sy))) {
    *area = 0.0;
    *amaj = 0.0;
    *amin = 0.0;
    *angle = 0.0;
    /* this is a poor fit - not an ellipse but a hyperbola */
    return (0);
  }

  A1 = 1/(2*Sx);
  A2 = 1/(2*Sy);
  A3 = Sxy;
  
  *angle = atan2(-A3, A2 - A1) / 2.0;
  R = sqrt( SQ(A2 - A1) + SQ(A3));
  root1 = (A1 + A2 + R);
  root2 = (A1 + A2 - R);

  *area = 2.0*M_PI/sqrt(root1*root2);
  *amaj = 2.35482*sqrt(1.0/root2);
  *amin = 2.35482*sqrt(1.0/root1);

  return (1);

}


/* In this function, Sx & Sy represent sx^2, sy^2 

   given an elliptical Gaussian of the form:

   exp (-z); z = x^2 / 2 sx^2 + y^2 / 2 sy^2 + xy Sxy

   Sa, Sb, and angle can be found by:

   A1 = 1/(2 Sx)
   A2 = 1/(2 Sy)
   A3 = Sxy
   
   R = sqrt ((A2 - A1)^2 + A3^2)
   root1 = (A1 + A2 + R)
   root2 = (A1 + A2 - R)
   
   angle = atan2 (-A3, A2 - A1) / 2.0
   Sa = sqrt (1.0/root2)
   Sb = sqrt (1.0/root1)
   
*/
