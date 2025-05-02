# include "astro.h"

int Npts;
float *xs, *ys, *zs;
float par[5];
float dpar[5];
float Dpar[5];
float outline_chi (float, float *);
int plot_outline ();

int outline (int argc, char **argv) {
  
  int i, j, k, Nx, Ny, NPTS, BigChange;
  float dIo, Io, ochisq, dchi, chisq, chisq_p, chisq_m, dp;
  float tmp_par, curve, value;
  float *in;
  Buffer *buf;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: outline x y dx dy dxy Io dIo (buffer)\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[8], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  par[0] = atof(argv[1]);
  par[1] = atof(argv[2]);
  par[2] = atof(argv[3]);
  par[3] = atof(argv[4]);
  par[4] = atof(argv[5]);
  Io = atof(argv[6]);
  dIo = atof(argv[7]);

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

  /* find all pixels within range Io-dIo : Io+dIo, in region about center guess */

  Nx = buf[0].matrix.Naxis[0];  
  Ny = buf[0].matrix.Naxis[1];
  Npts = 0;
  NPTS = 1000;
  ALLOCATE (xs, float, NPTS);
  ALLOCATE (ys, float, NPTS);
  ALLOCATE (zs, float, NPTS);
  for (j = par[1]-2*par[3]; j < par[1]+2*par[3]; j++) {
    if (j < 0) continue;
    if (j >= buf[0].matrix.Naxis[1]) continue;
    for (i = par[0]-2*par[2]; i < par[0]+2*par[2]; i++) {
      if (i < 0) continue;
      if (i >= buf[0].matrix.Naxis[0]) continue;
      value = in[i + Nx*j];
      if (fabs (value - Io) < dIo) {
	xs[Npts] = i;
	ys[Npts] = j;
	zs[Npts] = value;
	Npts ++;
	if (Npts == NPTS) {
	  NPTS += 1000;
	  REALLOCATE (xs, float, NPTS);
	  REALLOCATE (ys, float, NPTS);
	  REALLOCATE (zs, float, NPTS);
	}
      }
    }
  }

  if (Npts == 0) {
    gprint (GP_ERR, "no valid points in box, try again\n");
    free (xs);
    free (ys);
    free (zs);
    return (FALSE);
  }

  plot_outline ();
  chisq = outline_chi (Io, in);
  gprint (GP_ERR, "starting chisq: %f for %d pts\n", chisq, Npts);

# if (1)
  for (j = 0; j < 15; j++) {
    
    ochisq = chisq;
    for (i = 0; i < 5; i++) {
      /* find +chisq, -chisq for this par & adjust par as needed */

      for (k = 0, BigChange = TRUE; (k < 3) && BigChange; k++) {
	tmp_par = par[i];
	par[i] = tmp_par + dpar[i];
	chisq_p = outline_chi (Io, in);
	par[i] = tmp_par - dpar[i];
	chisq_m = outline_chi (Io, in);
	
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
	chisq = outline_chi (Io, in);
	
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
	ochisq = chisq;
      }
      if (!BigChange) dpar[i] *= 0.8;
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
# endif

  free (xs);
  free (ys);
  free (zs);
  
  plot_outline ();
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

# if (1)

float outline_chi (float Io, float *in) {

  int i;
  float theta, phi;
  float xp, yp, x, y;
  float chisq, R2;

  chisq = 0;

  for (i = 0; i < Npts; i++) {
    
    phi = atan2 (ys[i] - par[1], xs[i] - par[0]) - RAD_DEG * par[4];
    /* find a point:

       xp, yp such that atan (r2 sin(phi), r1 cos(phi)) == theta 

       tan (theta) = r2 sin(phi) / r1 cos (phi)

       (r1/r2) tan(theta) = sin(phi) / cos (phi);
       (r1/r2) tan(theta) = tan (phi)

       phi = atan2 (r1 sin(theta), r2 cos(theta))
    */

    theta = atan2 (par[2]*sin(phi), par[3]*cos(phi));

    /* this is the point on the ellipse at the same angle as ref point */
    /* this is wrong, but close -- tends to make ellipses too fat */
    xp = par[2] * cos (theta);
    yp = par[3] * sin (theta);
    
    x = xp * cos (par[4] * RAD_DEG) - yp * sin (par[4] * RAD_DEG) + par[0];
    y = xp * sin (par[4] * RAD_DEG) + yp * cos (par[4] * RAD_DEG) + par[1];

    R2 = sqrt (SQ (x - xs[i]) + SQ (y - ys[i]));

    /*
    Dv = zs[i] - Io;
    dv = fabs(zs[i]);
    F2 = Dv * Dv / dv;
    */

    chisq += R2;

  }

  chisq = chisq / Npts;
  return (chisq);

}

# else 

float outline_chi (float Io, float *in) {

  int i;
  float theta, theta1, theta2;
  float xp, yp, x, y;
  float chisq, dv, Dv, R2, F2, R, dR;

  int Nvec, Nvec2, Nvecx, Nvecy, Nv, nv;

  chisq = 0;

  nv = 0;
  Nv = 1000;
  if (!SelectVector (&Nvec, "dR", ANYVECTOR)) return (FALSE);
  if (!SelectVector (&Nvec2, "dF", ANYVECTOR)) return (FALSE);
  if (!SelectVector (&Nvecx, "x", ANYVECTOR)) return (FALSE);
  if (!SelectVector (&Nvecy, "y", ANYVECTOR)) return (FALSE);
  REALLOCATE (vectors[Nvec].elements, float, Nv);
  REALLOCATE (vectors[Nvec2].elements, float, Nv);
  REALLOCATE (vectors[Nvecx].elements, float, Nv);
  REALLOCATE (vectors[Nvecy].elements, float, Nv);

  for (i = 0; i < Npts; i++) {
    
    theta1 = atan2 (ys[i] - par[1], xs[i] - par[0]) - RAD_DEG * par[4];
    theta = atan2 (par[2]*sin(theta1), par[3]*cos(theta1));

    xp = par[2] * cos (theta);
    yp = par[3] * sin (theta);
    
    x = xp * cos (par[4] * RAD_DEG) - yp * sin (par[4] * RAD_DEG) + par[0];
    y = xp * sin (par[4] * RAD_DEG) + yp * cos (par[4] * RAD_DEG) + par[1];

    R2 = SQ (x - xs[i]) + SQ (y - ys[i]);

    /* 
    Dv = fabs (zs[i] - Io) + 1;
    dv = zs[i] + 0.2 * fabs (Dv);
    F2 = Dv * Dv / dv;
    */

    vectors[Nvec].elements[nv] = x;
    vectors[Nvec2].elements[nv] = y;
    vectors[Nvecx].elements[nv] = xs[i];
    vectors[Nvecy].elements[nv] = ys[i];
    nv ++;
    if (nv == Nv - 1) {
      Nv += 1000;
      REALLOCATE (vectors[Nvec].elements, float, Nv);
      REALLOCATE (vectors[Nvec2].elements, float, Nv);
      REALLOCATE (vectors[Nvecx].elements, float, Nv);
      REALLOCATE (vectors[Nvecy].elements, float, Nv);
    }

    /* typical distance might be 1 - 10 pix,
       typical z error might be 100 cts */
    chisq += R2; 

  }
  vectors[Nvec].Nelements = nv;
  vectors[Nvec2].Nelements = nv;
  vectors[Nvecx].Nelements = nv;
  vectors[Nvecy].Nelements = nv;

  chisq = chisq / Npts;
  return (chisq);

}
# endif

int plot_outline () {
  
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
  return (TRUE);
}
