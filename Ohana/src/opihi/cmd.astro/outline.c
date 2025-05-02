# include "astro.h"

float par[5];
float dpar[5];
float Dpar[5];
float outline_chi (float, float *, int, int, float *);

int outline (int argc, char **argv) {
  
  int i, j, k, BigChange, ABigChange;
  float Io, *in, ochisq, dchi, chisq, chisq_p, chisq_m, dp, tmp_par;
  float curve, frac;
  Buffer *buf;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: outline x y dx dy dxy Io (buffer)\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[7], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  par[0] = atof(argv[1]);
  par[1] = atof(argv[2]);
  par[2] = atof(argv[3]);
  par[3] = atof(argv[4]);
  par[4] = atof(argv[5]);
  Io = atof(argv[6]);
  // int Npar = atof (argv[8]);

  dpar[0] = 10;
  dpar[1] = 10;
  dpar[2] = 10;
  dpar[3] = 10;
  dpar[4] = 10;

  Dpar[0] = 10;
  Dpar[1] = 10;
  Dpar[2] = 10;
  Dpar[3] = 10;
  Dpar[4] = 10;

  in = (float *) buf[0].matrix.buffer;

  chisq = outline_chi (Io, in, buf[0].matrix.Naxis[0], buf[0].matrix.Naxis[1], &frac);
  gprint (GP_ERR, "chisq (1): %f\n", chisq);
  
  for (j = 0; j < 15; j++) {

    /*
    if (!(j % 3)) {
      chisq = outline_chi (Io, in, buf[0].matrix.Naxis[0], buf[0].matrix.Naxis[1], &frac);
      for (k = 0; (k < 3) && (fabs (frac) > 0.3); k++) {
	tmp1 = par[2];
	tmp2 = par[3];
	par[2] *= 1 + 0.1*frac;
	par[3] *= 1 + 0.1*frac;
	nchisq = outline_chi (Io, in, buf[0].matrix.Naxis[0], buf[0].matrix.Naxis[1], &frac);
	if (nchisq > chisq) {
	  par[2] = tmp1;
	  par[3] = tmp2;
	  k = 3;
	} else {
	  chisq = nchisq;
	}
	gprint (GP_ERR, "frac: %f  %f %f   %f\n", frac, par[2], par[3], chisq);
      }
    }
    */
    
    ABigChange = FALSE;
    ochisq = chisq;
    for (i = 4; i >= 0; i--) {
      /* find +chisq, -chisq for this par & adjust par as needed */

      for (k = 0, BigChange = TRUE; (k < 3) && BigChange; k++) {
	tmp_par = par[i];
	par[i] = tmp_par + dpar[i];
	chisq_p = outline_chi (Io, in, buf[0].matrix.Naxis[0], buf[0].matrix.Naxis[1], &frac);
	par[i] = tmp_par - dpar[i];
	chisq_m = outline_chi (Io, in, buf[0].matrix.Naxis[0], buf[0].matrix.Naxis[1], &frac);
	
	/* have we braketted a minimum? (curve < 0) */
	curve = (chisq_p - chisq) * (chisq - chisq_m);
	if (curve > 0) {
	  dp = 2*dpar[i];
	} else {
	  dp = 0.5 * dpar[i] * (chisq_m - chisq_p) / (chisq_m + chisq_p - 2*chisq);
	}      
	if (chisq_m + chisq_p - 2*chisq == 0) dp = 0;
	/* don't let extrapolation go too far */
	if (fabs (dp) > 2*fabs(dpar[i])) { dp = SIGN(dp) * fabs (2*dpar[i]); }
	
	par[i] = tmp_par + dp;
	chisq = outline_chi (Io, in, buf[0].matrix.Naxis[0], buf[0].matrix.Naxis[1], &frac);
	
	BigChange = FALSE;
	if (chisq <= 1.001*ochisq) {
	  /* got better */
	  dchi = (ochisq - chisq) / ochisq; 
	  if ((dchi > 0.03) || (curve > 0)) BigChange = TRUE;
	} else {
	  par[i] = tmp_par;
	  chisq = ochisq;
	  if (chisq_m < chisq) {
	    chisq = chisq_m;
	    par[i] = tmp_par - dpar[i];
	  }	
	  if (chisq_p < chisq) {
	    chisq = chisq_p;
	    par[i] = tmp_par + dpar[i];
	  }	
	}	
	/*
	gprint (GP_ERR, "try: %d  %f   ", i, chisq);
	for (k = 0; k < 5; k++) {
	  gprint (GP_ERR, "%f ", par[k]);
	}
	gprint (GP_ERR, "\n");
	*/
	ochisq = chisq;
	ABigChange |= BigChange;
      }
      if (!BigChange) dpar[i] *= 0.8;
    }

    if (ABigChange) {
      for (i = 0; i < 5; i++) {
	dpar[i] = Dpar[i];
      }
    }

    gprint (GP_ERR, "try: %d  %f   ", j, chisq);
    for (i = 0; i < 5; i++) {
      gprint (GP_ERR, "%f ", par[i]);
    }
    gprint (GP_ERR, "\n          ");
    for (i = 0; i < 5; i++) {
      gprint (GP_ERR, "%f ", dpar[i]);
    }
    gprint (GP_ERR, "\n");
    dchi -= chisq;

  }

    /* code to draw dots on Ximage */
    {
      int kapa;
      float xp, yp, x, y;
      float dx, dy, theta, t, dt;
      int Noverlay, NOVERLAY;
      KiiOverlay *overlay;
      
      if (!GetImage (NULL, &kapa, NULL)) return (FALSE);
      
      Noverlay = 0;
      NOVERLAY = 1000;
      ALLOCATE (overlay, KiiOverlay, Noverlay);
  
      dx = par[2];
      dy = par[3];
      dt = 1 / MAX (dx, dy);
      theta = par[4];
      
      for (t = 0; t < 6.3; t += dt) {
	xp = dx * cos (t);
	yp = dy * sin (t);
	
	x = xp * cos (theta * RAD_DEG) - yp * sin (theta * RAD_DEG) + par[0];
	y = xp * sin (theta * RAD_DEG) + yp * cos (theta * RAD_DEG) + par[1];
	
	overlay[Noverlay].type = KII_OVERLAY_BOX;
	overlay[Noverlay].x = x;
	overlay[Noverlay].y = y;
	overlay[Noverlay].dx = 1.0;
	overlay[Noverlay].dy = 1.0;

	Noverlay ++;
	CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000);
      }
      KiiLoadOverlay (kapa, overlay, Noverlay, "red");
      free (overlay);
    }

  return (TRUE);

}

/* par[0] = x
   par[1] = y
   par[2] = dx
   par[3] = dy
   par[4] = dxy
   
   ellipse is:  

   ((X-x)/dx)^2 + ((Y-y)/dy)^2 + (X-x)(Y-y)dxy = 1
  
   (yp/dy)^2 + xp yp dxy + (xp/dx)^2 - 1 = 0

   yp^2 + yp xp dxy dy^2 + xp^2 (dy/dx)^2 - dy^2 = 0

*/

float outline_chi (float Io, float *in, int Nx, int Ny, float *frac) {

  int npts, xo, yo, x, y;
  float xp, yp, dx, dy, theta;
  float t, dt, dv, Dv;
  float chisq, v, Frac;

  /* 
  if (!SelectVector (&Nvec, "diffs", ANYVECTOR)) return (FALSE);
  if (!SelectVector (&Nvec2, "angle", ANYVECTOR)) return (FALSE);
  if (!SelectVector (&Nvecx, "xdif", ANYVECTOR)) return (FALSE);
  if (!SelectVector (&Nvecy, "ydif", ANYVECTOR)) return (FALSE);
  */

  dx = par[2];
  dy = par[3];
  theta = par[4];
  dt = 1 / MAX (dx, dy);

  Frac = 0;
  chisq = 0;
  npts = 0;
  xo = -1; yo = -1;  /* an impossible coordinate */

  /*
  Npts = 1000;
  REALLOCATE (vectors[Nvec].elements, float, Npts);
  REALLOCATE (vectors[Nvec2].elements, float, Npts);
  REALLOCATE (vectors[Nvecx].elements, float, Npts);
  REALLOCATE (vectors[Nvecy].elements, float, Npts);
  */

  for (t = 0; t < 6.3; t += dt) {
    xp = dx * cos (t);
    yp = dy * sin (t);
    
    x = xp * cos (theta * RAD_DEG) - yp * sin (theta * RAD_DEG) + par[0];
    y = xp * sin (theta * RAD_DEG) + yp * cos (theta * RAD_DEG) + par[1];
    
    if ((x == xo) && (y == yo)) continue;
    xo = x; yo = y;

    if ((x >= 0) && (x < Nx) && (y >= 0) && (y < Ny)) {
      v = in[y*Nx + x];
      if (v > 0) {
	Dv = v - Io;
	dv = v + 0.2 * fabs (Dv);
	chisq += Dv * Dv / dv;
	if (Dv > sqrt(dv)) Frac += 1.0;
	if (Dv < sqrt(dv)) Frac -= 1.0;
	/*
	vectors[Nvec].elements[npts] = Dv;
	vectors[Nvec2].elements[npts] = t*DEG_RAD;
	vectors[Nvecx].elements[npts] = x;
	vectors[Nvecy].elements[npts] = y;
	*/
	npts ++;
	/* 
	if (npts == Npts - 1) {
	  Npts += 1000;
	  REALLOCATE (vectors[Nvec].elements, float, Npts);
	  REALLOCATE (vectors[Nvec2].elements, float, Npts);
	  REALLOCATE (vectors[Nvecx].elements, float, Npts);
	  REALLOCATE (vectors[Nvecy].elements, float, Npts);
	}
	*/
      }
    }
  }
  /* 
  vectors[Nvec].Nelements = npts;
  vectors[Nvec2].Nelements = npts;
  vectors[Nvecx].Nelements = npts;
  vectors[Nvecy].Nelements = npts;
  */

  chisq = chisq / npts;
  *frac = Frac / npts;
  if (npts == 0) {
    chisq = 1e8;
    *frac = -1.0;
  }

  return (chisq);

}
