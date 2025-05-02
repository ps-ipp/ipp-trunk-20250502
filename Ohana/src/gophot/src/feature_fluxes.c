# include "gophot.h"

float feature_fluxes () {

  int i, j, npix, n;
  float theta, phi, cs, sn;
  float xp, yp, x, y, dx, dy, Dx, Dy;
  float r1, r2;
  float flux, mean, sky;

  for (n = 0; n < nregion; n++) {

    npix = flux = 0;
    
    dx = MAX (region[n][4], region[n][5]) + 2;
    dy = MAX (region[n][4], region[n][5]) + 2;
    cs = cos (region[n][6] * RAD_DEG);
    sn = sin (region[n][6] * RAD_DEG);
    sky = region[n][0];      

    for (j = region[n][3] - dy; j < region[n][3] + dy; j++) {
      if (j < 0) continue;
      if (j >= nslow) continue;
      for (i = region[n][2] - dx; i < region[n][2] + dx; i++) {
	if (i < 0) continue;
	if (i >= nfast) continue;
	
	Dx = i - region[n][2];
	Dy = j - region[n][3];
	phi = atan2 (Dy, Dx) - RAD_DEG * region[n][6];
	theta = atan2 (region[n][4]*sin(phi), region[n][5]*cos(phi));
	
	/* this is the point on the ellipse at the same angle as ref point */
	xp = region[n][4] * cos (theta);
	yp = region[n][5] * sin (theta);
	
	x = xp * cs - yp * sn;
	y = xp * sn + yp * cs;
	
	r1 = hypot (Dx, Dy);
	r2 = hypot (x, y);
	
	if (r1 < r2) {
	  flux += big[i + nfast*j] - sky;
	  npix ++;
	}
	
      }
    }
    /* existing image has mean subtracted = flux*Npix */
    flux += region[n][1];
    mean = flux / npix;
    fprintf (stderr, "sub flux: %f, mag: %f, mean: %f\n", flux, -2.5*log10(flux), flux / npix);
    region[n][7] = flux;
  }
}
