# include "imclean.h"
# define NBLOCK 100
# define HIST_BINS 300 

/* good for sextractor */

SMPData *LoadStarsSex (char *filename, int *nstars, Header *header) {

  FILE *f;
  SMPData *stars;
  int NSTARS, Nstars, i, Nline, N;
  int type, status;
  double x, y, m, dm, sky, lsky, ftype;
  char *buffer;
  int Mhist[HIST_BINS], Shist[HIST_BINS], bin, sum, flags;
  double A, A2, S2, FWHMx, FWHMy, angle, Mgal, Map;
  int satfound, done;
  double saturate, complete;

  CHAR_LINE = 105;
  TYPE_FIELD = 0;
  
  ALLOCATE (buffer, char, CHAR_LINE*NBLOCK);

  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't find object file %s\n", filename);
    exit (1);
  }

  /* zero things that will sum */
  N = A = A2 = S2 = 0;
  for (i = 0; i < HIST_BINS; i++) { Mhist[i] = Shist[i] = 0; }

  Nstars = 0;
  NSTARS = 500;
  ALLOCATE (stars, SMPData, NSTARS);

  /* read in data from obj file */
  while ((Nline = fread (buffer, CHAR_LINE, NBLOCK, f)) > 0) {
    for (i = 0; i < Nline; i++) {
      status = sscanf (&buffer[i*CHAR_LINE + TYPE_FIELD], "%lf %lf %lf %lf %lf   %lf %lf %lf %lf %lf  %lf %d", 
		       &ftype, &x, &y, &m, &dm, &sky, &FWHMx, &FWHMy, &angle, &Mgal, &Map, &flags);
      if (status != 12) {
	fprintf (stderr, "ERROR: format error in file %s, line %d\n", filename, i);
	continue;
      } 

      if (flags > 7) continue;
      /* if (m > 0) continue; skip stars which totally fail on fit */
      if (dm == 0.0) dm = DEFAULT_ERROR;  

      /* sextract can provide values which are -2.5*log(counts) */
      m    += ZERO_POINT;
      Mgal += ZERO_POINT;
      Map  += ZERO_POINT;
      m    = MIN (32.767, MAX (-32.767, m));
      Mgal = MIN (32.767, MAX (-32.767, Mgal));
      Map  = MIN (32.767, MAX (-32.767, Map));

      if (sky < 1) {
	lsky = 0.0;
      } else {
	lsky = log10(sky);
      }

      /* type = MAX (0, MIN (9, 5*log10(ftype) + 10)); */
      switch (flags) {
      case 4:
      case 5:
      case 6:
      case 7:
	type = 10;
	break;
      case 1:
      case 2:
      case 3:
	type = 3;
	break;
      default:
	type = 1;
      }

      if (MIN_SN_FSTAT*dm > 1.0) continue;
      
      bin = MAX (0, MIN (HIST_BINS - 1, 10.0 * m));  /* stick in 0.1 mag bins */
      Mhist[bin] ++;
      
      stars[Nstars].X = x;
      stars[Nstars].Y = y;
      stars[Nstars].M = m;
      stars[Nstars].dM = dm;
      stars[Nstars].dophot = type;
      stars[Nstars].sky = lsky;
      
      stars[Nstars].fx = FWHMx;
      stars[Nstars].fy = FWHMx * (FWHMy/FWHMx);
      stars[Nstars].df = angle;
      stars[Nstars].Mgal = Mgal;
      stars[Nstars].Map = Map;

      Nstars++;
      if (Nstars == NSTARS - 1) {
	NSTARS += 500;
	REALLOCATE (stars, SMPData, NSTARS);
      }
    }

  }    

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
  
  gfits_modify (header, "ZERO_PT", "%lf", 1, ZERO_POINT);
  gfits_modify (header, "FWHM_X", "%lf", 1, FWHMx);
  gfits_modify (header, "FWHM_Y", "%lf", 1, FWHMy);
  gfits_modify (header, "ANGLE", "%lf", 1, angle);
  gfits_modify (header, "FSATUR", "%lf", 1, saturate);
  gfits_modify (header, "FLIMIT", "%lf", 1, complete);
  gfits_modify (header, "NSTARS", "%d", 1, N);

  *nstars = Nstars;
  return (stars);

}
