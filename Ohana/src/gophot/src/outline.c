# include "gophot.h"

int Npts;
float *xv, *yv, *zv;
float par[5];
float dpar[5];
float outline_chi (float);

/* fit pars[5] to ellipse at Xo, Yo, dX, dY */
int outline (float Xo, float Yo, float dX, float dY, float Io, float dIo, float *pars) {
  
  int i, j, k, Nx, Ny, NPTS, BigChange, ABigChange;
  float oChi, dchi, Chi, Chi_p, Chi_m, dp, tmp_par, nChi, tmp1, tmp2;
  float curve, frac, value;

  par[0] = Xo;
  par[1] = Yo;
  par[2] = dX;
  par[3] = dY;
  par[4] = 0.0;

  dpar[0] = 10;
  dpar[1] = 10;
  dpar[2] = 10;
  dpar[3] = 10;
  dpar[4] = 10;

  /* find all pixels within range Io-dIo : Io+dIo, in region about center guess */

  Npts = 0;
  NPTS = 1000;
  ALLOCATE (xv, float, NPTS);
  ALLOCATE (yv, float, NPTS);
  ALLOCATE (zv, float, NPTS);
  for (j = par[1]-2*par[3]; j < par[1]+2*par[3]; j++) {
    if (j < 0) continue;
    if (j >= nslow) continue;
    for (i = par[0]-2*par[2]; i < par[0]+2*par[2]; i++) {
      if (i < 0) continue;
      if (i >= nfast) continue;
      value = big[i + nfast*j];
      if (fabs (value - Io) < dIo) {
	xv[Npts] = i;
	yv[Npts] = j;
	zv[Npts] = value;
	Npts ++;
	if (Npts == NPTS) {
	  NPTS += 1000;
	  REALLOCATE (xv, float, NPTS);
	  REALLOCATE (yv, float, NPTS);
	  REALLOCATE (zv, float, NPTS);
	}
      }
    }
  }

  if (Npts == 0) {
    fprintf (stderr, "no valid points in box, try again\n");
    free (xv);
    free (yv);
    free (zv);
    return (FALSE);
  }

  Chi = outline_chi (Io);
  for (j = 0; j < 15; j++) {
    
    oChi = Chi;
    for (i = 0; i < 5; i++) {
      /* find +Chi, -Chi for this par & adjust par as needed */

      for (k = 0, BigChange = TRUE; (k < 3) && BigChange; k++) {
	tmp_par = par[i];
	par[i] = tmp_par + dpar[i];
	Chi_p = outline_chi (Io);
	par[i] = tmp_par - dpar[i];
	Chi_m = outline_chi (Io);
	
	/* have we braketted a minimum? (curve < 0) */
	curve = (Chi_p - Chi) * (Chi - Chi_m);
	if (curve > 0) {
	  dp = 2*dpar[i];
	} else {
	  dp = 0.5 * dpar[i] * (Chi_m - Chi_p) / (Chi_m + Chi_p - 2*Chi);
	}      
	if (Chi_m + Chi_p - 2*Chi == 0) dp = 0;
	/* don't let extrapolation go too far */
	if (fabs (dp) > 2*fabs(dpar[i])) { dp = SIGN(dp) * fabs (2*dpar[i]); }
	
	par[i] = tmp_par + dp;
	Chi = outline_chi (Io);
	
	BigChange = FALSE;
	if (Chi <= 1.001*oChi) {
	  /* got better */
	  dchi = (oChi - Chi) / oChi; 
	  if ((dchi > 0.03) || (curve > 0)) BigChange = TRUE;
	} else {
	  par[i] = tmp_par;
	  Chi = oChi;
	  if (Chi_m < Chi) {
	    Chi = Chi_m;
	    par[i] = tmp_par - dpar[i];
	  }	
	  if (Chi_p < Chi) {
	    Chi = Chi_p;
	    par[i] = tmp_par + dpar[i];
	  }	
	}	
	oChi = Chi;
      }
      if (!BigChange) dpar[i] *= 0.8;
    }

    mprint (0, "try: %d  %f   ", j, Chi);
    for (i = 0; i < 5; i++) {
      mprint (0, "%f ", par[i]);
    }
    mprint (0, "\n");
    for (i = 0; i < 5; i++) {
      mprint (2, "%f ", dpar[i]);
    }
    mprint (2, "\n");
    dchi -= Chi;

  }

  free (xv);
  free (yv);
  free (zv);
  
  pars[0] = par[0];
  pars[1] = par[1];
  pars[2] = par[2];
  pars[3] = par[3];
  pars[4] = par[4];
  return (TRUE);

}

/* par[0] = x
   par[1] = y
   par[2] = dx
   par[3] = dy
   par[4] = dxy
   
    xp = par[2] * cos (t);
    yp = par[3] * sin (t);
    
    x = xp * cos (par[4] * RAD_DEG) - yp * sin (par[4] * RAD_DEG) + par[0];
    y = xp * sin (par[4] * RAD_DEG) + yp * cos (par[4] * RAD_DEG) + par[1];

*/

float outline_chi (float Io) {

  int i;
  float theta, phi;
  float xp, yp, x, y;
  float Chi, dv, Dv, R2, F2;
  float cs, sn;

  Chi = 0;

  cs = cos (par[4] * RAD_DEG);
  sn = sin (par[4] * RAD_DEG);

  for (i = 0; i < Npts; i++) {
    
    phi = atan2 (yv[i] - par[1], xv[i] - par[0]) - RAD_DEG * par[4];
    theta = atan2 (par[2]*sin(phi), par[3]*cos(phi));

    /* this is the point on the ellipse at the same angle as ref point */
    xp = par[2] * cos (theta);
    yp = par[3] * sin (theta);
    
    x = xp * cs - yp * sn + par[0];
    y = xp * sn + yp * cs + par[1];

    R2 = sqrt (SQ (x - xv[i]) + SQ (y - yv[i]));
    Chi += R2;

  }

  Chi = Chi / Npts;
  return (Chi);

}
