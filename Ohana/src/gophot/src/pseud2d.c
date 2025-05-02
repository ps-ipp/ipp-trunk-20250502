# include "gophot.h"

# define HALF 0.5
# define THIRD 0.333
# define EXPMIN -15.0

float pseud2d (int ix, int iy, float *a, float *fa) {

  double x, y, t5, t6, t7, T;
  double denom, dt, pexp, R;
  double Q;
  float value;
  int i;

  x = ix - a[2];
  y = iy - a[3];
       
  t6 = a[5]*y;
  t5 = x/a[4];
  t7 = y/a[6];

  T = 0.5*((t5 + 2.0*t6)*x + t7*y);
  
  if (!finite (a[1])) fprintf (stderr, "bad star!\n");

  if (T >= 0) { 
    denom = 1.0 + T*(beta4 + HALF*beta64*T*(1.0 + THIRD*T));
    pexp = 1.0 / denom; 
    dt = beta4 + beta64*T*(1.0 + HALF*T);
    if (!finite (pexp)) {
      fprintf (stderr, "error in pseud2d: %f %f %f %d %d\n", T, denom, pexp, ix, iy); 
      denom = 1.0 + T + HALF*beta4*T*T + THIRD*beta64*T*T*T;
      pexp = 1.0 / denom;
      if (!finite (pexp)) { 
	fprintf (stderr, "error in pseud2d: %f %f %f %d %d\n", T, denom, pexp, ix, iy);
	fprintf (stderr, "%f %f   %f %f %f\n", x, y, t5, t6, t7); 
	fprintf (stderr, "%f %f %f\n", T, T*HALF*beta4*T, T*HALF*beta4*T*THIRD*beta64*T);
	for (i = 0; i < 7; i++) fprintf (stderr, "%d %f\n", i, a[i]);
	fprintf (stderr, "nstot: %d\n", nstot);
	fprintf (stderr, "invalid value!\n");
	exit (1);
      } 
    }
   } else {
    denom = MAX (T, EXPMIN);
    denom = fabs (denom);
    pexp = exp (denom);
    denom = 1.0;
    dt = 1.0;
  }

  value = a[1]*pexp;

  if (fa != (float *) NULL) {
    R = value*dt/denom;
    fa[1] = pexp;
    fa[2] = R*(t5 + t6);
    fa[3] = R*(a[5]*x + t7);
    fa[4] = R*t5*t5*0.5;
    fa[6] = R*t7*t7*0.5;
    
    fa[5] = -R*x*y;
    fa[0] = 1.0;
  }

  value += + a[0];

  if (!finite(value)) {
    fprintf (stderr, "error in pseud2d: %f %f %f %f\n", T, denom, pexp, a[1]);
    exit (1);
  }

  return (value);

}

/* this function uses C 0,N-1 for a[], fa[] */

/* a[0] - sky
   a[1] - Io
   a[2], a[3] - X, Y
   a[4], a[5], a[6] - shape
*/

