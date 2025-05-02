# include "gophot.h"

# define HALF 0.5
# define THIRD 0.33333333333333
# define EXPMIN -15

float pseud4d (int ix, int iy, float *a, float *fa) {

  int i, ioff;
  float x, y;
  float denom, T, dt, pexp[2];
  float a2, a5, a6, a7, t5, t6, t7, value;
	
  a5 = 1.0/a[7];
  a7 = 1.0/a[9];
  a6 = a[8];
  
  for (i = 0; i < 2; i++) {

    ioff = 3*i;
    x = ix - a[2 + ioff];
    y = iy - a[3 + ioff];
       
    t5 = a5*x;
    t6 = a6*y;
    t7 = a7*y;
    T = HALF*((t5 + 2*t6)*x + t7*y);
	   
    if (T > 0) {
      denom = 1.0 + T*(1.0 + HALF*beta4*T*(1.0 + THIRD*beta64*T));
      pexp[i] = 1./denom;
    } else {
      denom = MAX (T, EXPMIN);
      denom = fabs (denom);
      pexp[i] = exp (denom);
      denom = 1.0;
    }

    a2 = exp(a[1 + ioff]);
    value = a2*pexp[i];
    pexp[i] = value;
    
    if (fa != (float *) NULL) {
      if (T > 0) {
	dt = 1.0 + beta4*T*(1.0 + HALF*beta64*T);
      } else {
      	dt = 1.0;
      }
      fa[1 + ioff] = value;
      T = pexp[i]*dt/denom;
      fa[2 + ioff] = (a5*x + a6*y)*T;
      fa[3 + ioff] = (a6*x + a7*y)*T;
      
      fa[0] = 1;
    }      
  }

  value = pexp[0] + pexp[1] + a[0];

  return (value);

}

/* this function uses C 0,N-1 for a[], fa[] */

/* a[0] - sky
   a[1] - I1
   a[2], a[3] - X1, Y1
   a[4] - I2
   a[5], a[6] - X2, Y2
   a[7], a[8], a[9] - shape
*/

