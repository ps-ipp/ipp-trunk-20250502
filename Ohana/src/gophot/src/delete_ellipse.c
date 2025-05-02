# include "gophot.h"

float delete_ellipse (float *par, float sky) {

  int i, j, npix;
  float theta, phi;
  float xp, yp, x, y, dx, dy, Dx, Dy;
  float Chi, dv, Dv, R2, F2, r1, r2;
  float flux, mean;

  npix = flux = 0;

  dx = MAX (par[2], par[3]) + 2;
  dy = MAX (par[2], par[3]) + 2;

  for (j = par[1] - dy; j < par[1] + dy; j++) {
    if (j < 0) continue;
    if (j >= nslow) continue;
    for (i = par[0] - dx; i < par[0] + dx; i++) {
      if (i < 0) continue;
      if (i >= nfast) continue;
    
      Dx = i - par[0];
      Dy = j - par[1];
      phi = atan2 (Dy, Dx) - RAD_DEG * par[4];
      theta = atan2 (par[2]*sin(phi), par[3]*cos(phi));
      
      /* this is the point on the ellipse at the same angle as ref point */
      xp = par[2] * cos (theta);
      yp = par[3] * sin (theta);
    
      x = xp * cos (par[4] * RAD_DEG) - yp * sin (par[4] * RAD_DEG);
      y = xp * sin (par[4] * RAD_DEG) + yp * cos (par[4] * RAD_DEG);
      
      r1 = hypot (Dx, Dy);
      r2 = hypot (x, y);
      
      if (r1 < r2) {
	flux += big[i + nfast*j] - sky;
	npix ++;
      }

    }
  }
  mean = flux / npix;
  fprintf (stderr, "flux: %f, mag: %f, mean: %f\n", flux, -2.5*log10(flux), flux / npix);

  for (j = par[1] - dy; j < par[1] + dy; j++) {
    if (j < 0) continue;
    if (j >= nslow) continue;
    for (i = par[0] - dx; i < par[0] + dx; i++) {
      if (i < 0) continue;
      if (i >= nfast) continue;
    
      Dx = i - par[0];
      Dy = j - par[1];
      phi = atan2 (Dy, Dx) - RAD_DEG * par[4];
      theta = atan2 (par[2]*sin(phi), par[3]*cos(phi));
      
      /* this is the point on the ellipse at the same angle as ref point */
      xp = par[2] * cos (theta);
      yp = par[3] * sin (theta);
    
      x = xp * cos (par[4] * RAD_DEG) - yp * sin (par[4] * RAD_DEG);
      y = xp * sin (par[4] * RAD_DEG) + yp * cos (par[4] * RAD_DEG);
      
      r1 = hypot (Dx, Dy);
      r2 = hypot (x, y);
      
      if (r1 < r2) {
	big[i + nfast*j] -= mean;
      }
      if (r1 < 1.2*r2) {
	noise[i + nfast*j] += fac*(mean + sky);
      }

    }
  }
  fprintf (stderr, "flux: %f, mag: %f, mean: %f\n", flux, -2.5*log10(flux), flux / npix);
  return (flux);
}
