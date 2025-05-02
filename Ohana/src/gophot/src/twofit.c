# include "gophot.h"

/****************************************
 there are big problems here with the values of b and probably 
 other things.  fix please! */

/* we are fitting two stars at the location of the input star */
/* starraw has the real X,Y coords, star is relative to centroid */

float twofit (float *star, float *starraw, float *star1, float *star2) {

  int i;
  float b[2*NPMAX], err[2*NPMAX], bacc[2*NPMAX], bparlim[2*NPMAX], tstar[NPMAX];
  bool badfit, conv;
  float a5, a7, angle, root, root1, root2, dx2, dy2, dx, dy, value;
  float garea, sarea, dx74, dy74, dot, fac1, fac2, dxmax, dymax;

  badfit = FALSE;
  conv = TRUE;

  /* fills in values 1, 5, 6, 7, 8 */
  parinterp (starraw[2], starraw[3], tstar);

  /* this all seems way too obtuse. */
  a5 = 1.0/star[4];
  a7 = 1.0/star[6];
  angle = atan2 (-2*star[5], a7 - a5)/2.0;
  root = sqrt (SQ(a5 - a7) + 4*SQ(star[5]));
  root1 = (a5 + a7 + root)/2.0;
  root2 = root1 - root;
  dx2 = star[4] - tstar[4];
  dy2 = star[6] - tstar[6];
  dx = sqrt (MAX (dx2, 0.0)) / 2.0;
  dy = sqrt (MAX (dy2, 0.0)) / 2.0;
  badfit = (MAX (dx, dy) == 0);
  /* we can use these since dx & dy are always >= 0 */
  dx = (cos(angle) < 0) ? -dx : dx;
  dy = (sin(angle) < 0) ? -dy : dy;
  garea = 1.0 / sqrt (fabs (root1*root2));
  sarea = sqrt (fabs (tstar[4]*tstar[6]));
  dx74 = starraw[2] - star[2];	
  dy74 = starraw[3] - star[3];		
  dot = dx74*dx + dy74*dy;
  if (dot > 0) {
    fac1 = 0.6666666;
    fac2 = 1.3333333;
  } else {
    fac1 = 1.3333333;
    fac2 = 0.6666666;
  }
	   
  /* 
     b[0] - sky
     b[1] - I1
     b[2], b[3] - X1, Y1
     b[4] - I2
     b[5], b[6] - X2, Y2
     b[7], b[8], b[9] - shape 
  */

  b[0] = star[0];
  b[1] = (star[1]*garea*fac2 / (sarea*2));
  b[2] = star[2] - dx*fac1;
  b[3] = star[3] - dy*fac1;
  b[4] = (star[1]*garea*fac1 / (sarea*2));		
  b[5] = star[2] + dx*fac2;
  b[6] = star[3] + dy*fac2;

  /* for reasons unclear, the fit is done in ln(Io), not Io */
  b[1] = log(b[1]);
  b[4] = log(b[4]); 

  b[7] = tstar[4];
  b[8] = tstar[5];
  b[9] = tstar[6];

  bacc[0] = acc[0];
  bacc[1] = bacc[4] = acc[1];
  bacc[2] = bacc[5] = acc[2];
  bacc[3] = bacc[6] = acc[3];

  bparlim[0] = parlim[0];
  bparlim[1] = bparlim[4] = parlim[1];
  bparlim[2] = bparlim[5] = parlim[2];
  bparlim[3] = bparlim[6] = parlim[3];

  dxmax = MAX (fabs(b[2]), fabs(b[5]));
  dymax = MAX (fabs(b[3]), fabs(b[6]));
  badfit = badfit || (dxmax > irect[1]/2.0);
  badfit = badfit || (dymax > irect[2]/2.0);

  if (!badfit) {
    value = chisq (twostar, xs, ys, zs, dzs, npt, b, err, NFIT2, bacc, bparlim, 2*nit);
    conv = finite (value);
  }
	
  dxmax = MAX (fabs(b[2]), fabs(b[5]));
  dymax = MAX (fabs(b[3]), fabs(b[6]));
  badfit = badfit || (dxmax > 0.4*irect[1]);
  badfit = badfit || (dymax > 0.4*irect[2]);

  if (conv) {
    if (b[1] < b[4]) {
      SWAP (b[1], b[4]);
      SWAP (b[2], b[5]);
      SWAP (b[3], b[6]);
    }
    
    star1[0] = star2[0] = b[0];
    star1[4] = star2[4] = b[7];
    star1[5] = star2[5] = b[8];
    star1[6] = star2[6] = b[9];
    star1[1] = exp(b[1]);
    star1[2] = b[2];
    star1[3] = b[3];
    star2[1] = exp(b[4]);
    star2[2] = b[5];
    star2[3] = b[6];
  }

  badfit = badfit || centertest (star1, xs, ys);
  badfit = badfit || centertest (star2, xs, ys);
  if (!conv || badfit) value = MAGIC;

  return (value);
}
/* this function uses C 0,N-1 for a[], fa[] */
