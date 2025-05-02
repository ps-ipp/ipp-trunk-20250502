# include "imclean.h"
# define NBLOCK 100
# define HIST_BINS 300 
/* the mag histogram has range 0.0 to 30.0 */
/* m = 0.1*Mhist[i] */

SMPData *LoadStarsDophot (char *filename, int *nstars, Header *header) {

  FILE *f;
  SMPData *stars;
  int NSTARS, Nstars, i, Nline, N;
  int type, status;
  double x, y, m, dm, sky, lsky, fx, fy, df, Mgal, Map;
  char *buffer;
  int Mhist[HIST_BINS], Shist[HIST_BINS], n[20], bin, sum;
  double FWHMx, FWHMy, angle;
  int satfound, done;
  double saturate, complete;
  char line[256];
  int MedHist[2002], NMedHist;
  double SMedHist, dMed;

  N = NMedHist = 0;
  bzero (MedHist, 2002*sizeof(int));

  ALLOCATE (buffer, char, CHAR_LINE*NBLOCK);

  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't find object file %s\n", filename);
    exit (1);
  }

  /* zero things that will sum */
  for (i = 0; i < HIST_BINS; i++) { Mhist[i] = Shist[i] = 0; }
  for (i = 1; i <= 9; i++) { n[i] = 0; }

  Nstars = 0;
  NSTARS = 500;
  ALLOCATE (stars, SMPData, NSTARS);

  /* read average values from first line */
  scan_line (f, line);
  sscanf (line, "%*s %*s %*s %lf %lf %lf", &FWHMx, &FWHMy, &angle);

  /* read in data from obj file */
  while ((Nline = fread (buffer, CHAR_LINE, NBLOCK, f)) > 0) {
    for (i = 0; i < Nline; i++) {
      /* we are now using all entries on the *.obj line */
      status = sscanf (&buffer[i*CHAR_LINE], "%d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", 
		       &type, &x, &y, &m, &dm, &sky, &fx, &fy, &df, &Mgal, &Map);
      if (status != 11) {
	fprintf (stderr, "format error in file %s, line %d\n", filename, i);
	continue;
      }

      /* dophot magnitudes can range from 99.999 to -99.999 
	 realistic numbers are between -20 and 0
	 outside, we should set the value to 50.0 to force saturation */
      
      n[type] ++;
      if (type == 6) continue;
      if (type == 9) continue;
      if (type == 8) continue;
      if (type == 16) continue;
      
      if (m > 0) continue;                /* skip stars which totally fail on fit */
      if (dm == 0.0) dm = DEFAULT_ERROR;  /* stars with poor errors, get 25.5% errors */
      dm = MIN (0.999, MAX (0.0, dm));    /* truncate dm to fit in range 0 - 999 on output */

      /* need to accumulate the median histogram thingy */
      dMed = Map - m;
      if ((fabs(m) < 90) && (fabs(Map) < 90) && (fabs(dMed) < 1)) {
	bin = 1000 * (dMed + 1);
	MedHist[bin] ++;
	NMedHist ++;
      }
      
      /* dophot provides values which are -2.5*log(counts) */
      m    = ((m    > -25) && (m    < 0)) ? m + ZERO_POINT    : 50.0;
      Mgal = ((Mgal > -25) && (Mgal < 0)) ? Mgal + ZERO_POINT : 50.0;
      Map  = ((Map  > -25) && (Map  < 0)) ? Map + ZERO_POINT  : 50.0;

      if (sky < 1.0) {
	lsky = 0.0;
      } else {
	lsky = log10(sky);
      }

      if (MIN_SN_FSTAT*dm > 1.0) continue;  /* skip stars with errors too large */
          
      switch (type) {
      case 1:
      case 4:
      case 7:
	bin = MAX (0, MIN (HIST_BINS - 1, 10.0 * m));  /* stick in 0.1 mag bins */
	Mhist[bin] ++;
      default:
	if (df < 0.0) df += 360.0;
	stars[Nstars].X      = x;
	stars[Nstars].Y      = y;
	stars[Nstars].M      = m;
	stars[Nstars].Mgal   = Mgal;
	stars[Nstars].Map    = Map;
	stars[Nstars].dM     = dm;
	stars[Nstars].dophot = type;
	stars[Nstars].sky    = lsky;
	stars[Nstars].fx     = fx;
	stars[Nstars].fy     = fy;
	stars[Nstars].df     = df;
	Nstars++;
	if (Nstars == NSTARS - 1) {
	  NSTARS += 500;
	  REALLOCATE (stars, SMPData, NSTARS);
	}
      }
    }

  }    

  /* look at histogram, find saturation and completion limits */
  sum = 0;
  for (i = 0; i < HIST_BINS; i++) {
    sum += Mhist[i];
    Shist[i] = sum;
  }
  satfound = done = FALSE;
  saturate = complete = 0.0;
  for (i = 0; (i < HIST_BINS) && !done; i++) {
    if ((!satfound) && (Mhist[i] > 0)) {
      saturate = 0.1*(i-1);
      satfound = TRUE;
    }
    if (Shist[i] > 0.9*Shist[HIST_BINS - 1]) {
      complete = 0.1*i;
      done = TRUE;
    }
  }
  
  SMedHist = 0;
  for (i = 0; (i < 2002) && (SMedHist < NMedHist / 2); i++) {
    SMedHist += MedHist[i];
  }
  if (i == 2002) {
    fprintf (stderr, "error finding (Ap - Fit) median\n");
    SMedHist = 0;
  } else {
    SMedHist = 0.001*i - 1;
    fprintf (stderr, "(Ap - Fit) median = %f\n", SMedHist);
  }

  for (i = 0; i < Nstars; i++) {
    stars[i].Mgal += SMedHist;
    stars[i].M += SMedHist;
  }    

  gfits_modify (header, "ZERO_PT", "%lf", 1, ZERO_POINT);
  gfits_modify (header, "FWHM_X", "%lf", 1, FWHMx);
  gfits_modify (header, "FWHM_Y", "%lf", 1, FWHMy);
  gfits_modify (header, "ANGLE", "%lf", 1, angle);
  gfits_modify (header, "FSATUR", "%lf", 1, saturate);
  gfits_modify (header, "FLIMIT", "%lf", 1, complete);
  gfits_modify (header, "APMIFIT", "%lf", 1, SMedHist);
  gfits_modify (header, "NSTARS", "%d", 1, N);
  for (i = 1; i <= 9; i++) {
    sprintf (line, "TDOPHOT%1d", i);
    gfits_modify (header, line, "%d", 1, n[i]);
  }

  *nstars = Nstars;
  return (stars);

}

/* this function should load the stars and immediately convert them to
   have the ZERO_PT zero point */
