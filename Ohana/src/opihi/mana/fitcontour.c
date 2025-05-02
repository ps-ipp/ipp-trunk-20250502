# include "mana.h"
# define NTERM 3

int fitcontour (int argc, char **argv) {
  
  int i;
  double **C, **B;
  float cs1, sn1, cs, sn, x, y, r, xo, yo;
  float dR, Rmin, Rmaj, Theta, Rx, Ry, Rxy, R1, R2, R3;
  Vector *vecx, *vecy;

  /* USAGE fitcontour x y Xo Yo */
  if (argc < 5) goto usage;

  if ((vecx = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  xo = atof (argv[3]);
  yo = atof (argv[4]);

  // require these to be float vectors
  REQUIRE_VECTOR_FLT (vecx, FALSE);
  REQUIRE_VECTOR_FLT (vecy, FALSE);

  ALLOCATE (B, double *, NTERM);
  ALLOCATE (C, double *, NTERM);
  for (i = 0; i < NTERM; i++) {
    ALLOCATE (C[i], double, NTERM);
    bzero (C[i], NTERM*sizeof(double));
    ALLOCATE (B[i], double, 1);
    bzero (B[i], sizeof(double));
  }

  /* we are fitting r = ro + rs*sin(2theta) + rc*cos(2theta) */
  /* sin(2t) = 2cos(t)sin(t)
     cos(2t) = cos^2(t) - sin^2(t)
  */
  for (i = 0; i < vecx[0].Nelements; i++) {
    x = vecx[0].elements.Flt[i] - xo;
    y = vecy[0].elements.Flt[i] - yo;
    r = hypot (x, y);

    /* calculate sin(2t), cos(2t) using 1/2 angle tri relationship above */
    sn1 = y / r;
    cs1 = x / r;
    sn = 2*sn1*cs1;
    cs = cs1*cs1 - sn1*sn1;

    C[0][0] += 1.0;
    C[1][0] += sn;
    C[1][1] += SQ(sn);
    C[2][0] += cs; 
    C[2][1] += cs*sn; 
    C[2][2] += SQ(cs);

    B[0][0] += r;
    B[1][0] += r*sn;
    B[2][0] += r*cs; 
  }
  C[0][1] = C[1][0];
  C[0][2] = C[2][0];
  C[1][2] = C[2][1];
    
  dgaussjordan (C, B, NTERM, 1);
  
  /** this is somewhat weak: if the object is too elongated, Rmin can be < 0 **/
  dR = hypot (B[1][0], B[2][0]);
  Rmaj = B[0][0] + dR;
  Rmin = B[0][0] - dR;
  Theta = DEG_RAD*atan2 (B[1][0], B[2][0]) / 2;

  sn = B[1][0] / dR;
  cs = B[2][0] / dR;

  R1 = SQ(Rmaj) + SQ(Rmin);
  R2 = SQ(Rmaj) - SQ(Rmin);
  R3 = Rmaj*Rmin;

  Rx = R3 / sqrt (R1 - R2*cs);
  Ry = R3 / sqrt (R1 + R2*cs);
  Rxy = -sn*R2 / SQ(R3);

  set_variable ("Rx", Rx);
  set_variable ("Ry", Ry);
  set_variable ("Rxy", Rxy);

  set_variable ("Rmin", Rmin);
  set_variable ("Rmaj", Rmaj);
  set_variable ("Theta", Theta);

  for (i = 0; i < NTERM; i++) {
    free (B[i]);
    free (C[i]);
  }
  free (B);
  free (C);

  return (TRUE);

 usage:
  gprint (GP_ERR, "fitcontour x y (xo) (yo)\n");
  return (FALSE);
}


/* this routine fits a single sine and cosine to the value of dr as a function of the angle around the central
 * point, theta.  this is NOT an exact fit, an is increasingly in error for more eccentric ellipses.  however,
 * it does a reasonable job of approximating the value of the major and minor axes, and it gets the angle of
 * orientation right at well.  We then assume the values of major and minor axis and angle are correct to get
 * the parameters for the elliptical gaussian fit:
 
 z = (x^2) / (2 Rx^2) + (y^2) / (2 Ry^2) + Rxy x y

 the functions above give the correct relationship between these:

   R1 = SQ(Rmaj) + SQ(Rmin);
   R2 = SQ(Rmaj) - SQ(Rmin);
   R3 = Rmaj*Rmin;

   Rx = R3 / sqrt (R1 - R2*cs);
   Ry = R3 / sqrt (R1 + R2*cs);
   Rxy = -sn*R2 / SQ(R3);

 to derive these relationships, write the equation for an ellipse with major axis in the x-dir and minor in
 the y-dir:

 z = (x^2) / (2 Rmaj^2) + (y^2) / (2 Rmin^2)

 apply the rotation matrix:

 (x)' = (cos, -sin)(x,y)
 (y)' = (sin, +cos)(x,y)  (modulo the sign on the sine)

 then group the x^2 terms, the y^2 terms, and the xy terms to find the three coeffs.

 NOTE: I spent a while having trouble deriving this from the polar form of an ellipse:

 x = Rmaj*cos(theta)
 y = Rmin*sin(theta)

 it turns out that 'theta' above is NOT the angle (0,1)-(0,0)-(x,y). rather, it should be viewed as a
 paraterization of the ellipse.  

*/
