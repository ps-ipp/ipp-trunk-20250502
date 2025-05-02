/* 
here is some C code to project from R,D to x,y (all in decimal
degrees) in a SIN projection, with Ro, Do as projection center:
*/

# define FULLPROJECTION 0
# define DEG_RAD 57.295779513082322
# define RAD_DEG  0.017453292519943

RD_to_XY (double *x, double *y, double R, double D, double Ro, double Do) {

  double sdp, cdp, salp, calp, sdel, cdel, stht, sphi, cphi;
                 
  sdp  = sin(RAD_DEG*Do);
  cdp  = cos(RAD_DEG*Do);
  salp = sin(RAD_DEG*(ra - Ro));
  calp = cos(RAD_DEG*(ra - Ro));
  sdel = sin(RAD_DEG*dec);
  cdel = cos(RAD_DEG*dec);
  
  stht = sdel*sdp + cdel*cdp*calp;    /* sin(theta) */
  sphi = cdel*salp;                   /* = cos(theta)*sin(phi) */
  cphi = cdel*sdp*calp - sdel*cdp;    /* = cos(theta)*cos(phi) */
  if (stht < 0) { return 0; /* projection from the wrong side of the sphere */ }
  
  X =  DEG_RAD * sphi;
  Y = -DEG_RAD * cphi;
  
# if (FULLPROJECTION) 
  
  /* 
     these lines allow for a rotation / distortion 2x2 matrix (pci_j),
     a (two direction) plate-scale shift (cdelt1, cdelt2), 
     and a reference center offset of Xo, Yo, if desired. 
  */

  tmp_d = 1.0 / (pc_1_1*pc_2_2 - pc_1_2*pc_2_1); 
  *x = tmp_d * (pc_2_2*X - pc_1_2*Y) / cdelt1 + Xo;
  *y = tmp_d * (pc_1_1*Y - pc_2_1*X) / cdelt2 + Yo;

# else

  *x = X;
  *y = Y;
  
# endif


  return (1);

}


/* 
here is some C code to project from x,y to R,D (all in decimal
degrees) in a SIN projection, with Ro, Do as projection center:
*/

XY_to_RD (double *ra, double *dec, double x, double y, double Ro, double Do) {


  double L, M, X, Y, T, Z;
  double R, sphi, cphi, stht, ctht;
  double alpha, delta, salp, calp, sdel, sdp, cdp;
  
  *ra  = 0;
  *dec = 0;
  stht = ctht = 1;

  
# if (FULLPROJECTION) 
  /* 
     these lines allow for a rotation / distortion 2x2 matrix (pci_j),
     a (two direction) plate-scale shift (cdelt1, cdelt2), 
     and a reference center offset of Xo, Yo, if desired. 
  */
  X = cdelt1 * (x - Xo);
  Y = cdelt2 * (y - Yo);

  L = (X*pc1_1 + Y*pc1_2);
  M = (X*pc2_1 + Y*pc2_2);
# else 
  L = x;
  M = y;
# endif

  R = hypot (L,M);
  if ((L == 0) && (M == 0)) {
    sphi = 0;
    cphi = 1;
  }
  else {
    sphi =  L / R;
    cphi = -M / R;
  }

  ctht = RAD_DEG * R;
  stht = sqrt (1 - ctht*ctht);

  sdp  = sin(RAD_DEG*Do);
  cdp  = cos(RAD_DEG*Do);
  
  sdel = stht*sdp - ctht*cphi*cdp;
  salp = ctht*sphi;
  calp = stht*cdp + ctht*cphi*sdp;
  alpha = atan2 (salp, calp);
  delta = asin (sdel);
  
  *ra  = DEG_RAD*alpha + Ro;
  *dec = DEG_RAD*delta;
  
  return (1);

}

