# include "imclean.h"

star_stats (Header *header, SMPData *stars, int Nstars) {

  int i;
  
  /* zero things that will sum */
  N = A = A2 = S2 = 0;
  gotFWHM = FALSE;

  for (i = 0; i < HIST_BINS; i++) { Mhist[i] = Shist[i] = 0; }
  for (i = 1; i <= 9; i++) { n[i] = 0; }

  for (i = 0; i < Nstars; i++) {
    n[type] ++;
    switch (type) {
    case 6:  /* just throw these ones out */
    case 8:
    case 9:
      break;
    case 1:
      sscanf (&buffer[i*CHAR_LINE + AP_FIELD], "%lf", &ap);
      if (ap < 99) {
	apmifit = ap - m;
	A += apmifit / SQ(df);
	A2 += SQ(apmifit) / SQ(df);
	S2 += 1.0 / SQ(df);
      } 
      if (!gotFWHM) {
	sscanf (&buffer[i*CHAR_LINE + PSF_FIELD], "%lf %lf %lf ", &FWHMx, &FWHMy, &angle);
	gotFWHM = TRUE;
      }
    case 4:
    case 7:
      bin = MAX (0.0, MIN (HIST_BINS, 10.0 * (m + 15.0)));  /* stick in 0.1 mag bins */
      Mhist[bin] ++;
    case 2:
    case 3:
    case 5:
      fprintf (g, "%6.1f %6.1f %6.3f %03d %1d %3.1f\n", x, y, m+ZERO_POINT, (int)(1000*df), type, lsky);
      N ++; 
    }
  }

  Ap = A / S2;
  Ap2 = sqrt(A2 / (S2) - Ap*Ap);
  sum = 0.0;
  for (i = 0; i < HIST_BINS; i++) {
    sum += Mhist[i];
    Shist[i] = sum;
  }
  satfound = done = FALSE;
  for (i = 0; (i < HIST_BINS) && !done; i++) {
    if ((!satfound) && (Mhist[i] > 0)) {
      saturate = 0.1*(i-1) - 15.0;
      satfound = TRUE;
    }
    if (Shist[i] > 0.9*Shist[HIST_BINS - 1]) {
      complete = 0.1*i - 15.0;
      done = TRUE;
    }
  }
  
  gfits_modify (&header, "ZERO_PT", "%lf", 1, ZERO_POINT);
  gfits_modify (&header, "FWHM_X", "%lf", 1, FWHMx);
  gfits_modify (&header, "FWHM_Y", "%lf", 1, FWHMy);
  gfits_modify (&header, "ANGLE", "%lf", 1, angle);
  gfits_modify (&header, "APMIFIT", "%lf", 1, Ap);
  gfits_modify (&header, "dAPMIFIT", "%lf", 1, Ap2);
  gfits_modify (&header, "FSATUR", "%lf", 1, (saturate + ZERO_POINT));
  gfits_modify (&header, "FLIMIT", "%lf", 1, (complete + ZERO_POINT));
  gfits_modify (&header, "NSTARS", "%d", 1, N);
  for (i = 1; i <= 9; i++) {
    sprintf (line, "TDOPHOT%1d\0", i);
    gfits_modify (&header, line, "%d", 1, n[i]);
  }
