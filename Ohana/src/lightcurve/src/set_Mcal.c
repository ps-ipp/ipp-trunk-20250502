# include "lightcurve.h"

void set_Mcal (images, Nimages)
Image    *images;
int       Nimages;
{

  int i;
  double clouds, Clouds, dClouds, R, R2, N;
  
  R = R2 = N = 0.0;
  for (i = 0; i < Nimages; i++) {
    images[i].clouds = images[i].Mcal + C_LAMBDA - A_LAMBDA * images[i].airmass + images[i].AmF;
    if (!images[i].empty && images[i].fixed) {
      R  +=     images[i].clouds;
      R2 += SQ (images[i].clouds);
      N  += 1.0;
    }
  }

  Clouds  = R / N;
  dClouds = sqrt (R2 / N - Clouds*Clouds);

  fprintf (stderr, "Clouds = %f, dClouds = %f, N / Nimages: %f / %d\n", Clouds, dClouds, N, Nimages);

  dClouds = MAX (0.0001, dClouds);

  for (i = 0; i < Nimages; i++) {
    if (fabs(images[i].clouds - Clouds) > SIG*dClouds) 
      images[i].fixed = FALSE;
    else {
      images[i].fixed  = TRUE;
      images[i].Mcal   = - C_LAMBDA + A_LAMBDA * images[i].airmass - images[i].AmF;
      images[i].clouds = 0.0;
    }
  }
}

/*  BIG IMPORTANT NOTE:  equations (3) and (7) in Magnier et al 1992, A&A 96, 379
    are not consistent with positive defined A_LAMBDA.  

    the equations should read:

    (3)  M_app = c_lambda + m - a_lambda*\zeta ... + clouds    (... is color term)
    (7)  M_cal = -c_lambda + a_lambda*\zeta

    The point is that a_lambda is a positive coeff, so as \zeta increases, the
    image is *less* sensitive, so M_cal goes up.  conversely, c_lambda is also
    a positive defined number, so as c_lambda increases, the image is *more* 
    sensitive.  Since the a_lambda and c_lambda have opposite senses, they 
    must have opposite signs.  

*/
