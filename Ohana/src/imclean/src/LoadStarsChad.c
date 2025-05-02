# include "imclean.h"
# define NBLOCK 100
# define HIST_BINS 150

SMPData *LoadStarsChad (char *filename, int *nstars, Header *header) {

  FILE *f;
  SMPData *stars;
  int NSTARS, Nstars, i, N;
  int status;
  double x, y, m, sky, lsky;
  int Mhist[HIST_BINS], Shist[HIST_BINS], bin, sum;
  double FWHMx, FWHMy, angle, flux;
  int satfound, done;
  double saturate, complete;
  char line[256];

  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't find object file %s\n", filename);
    exit (1);
  }

  /* zero things that will sum */
  for (i = 0; i < HIST_BINS; i++) { Mhist[i] = Shist[i] = 0; }
  
  N = 0;
  Nstars = 0;
  NSTARS = 500;
  ALLOCATE (stars, SMPData, NSTARS);

  /* for now assume file 'header' is fixed-format */
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  if (strncasecmp (line, "#seeing", 7)) {
    fprintf (stderr, "error in header, skipping\n");
    exit (1);
  }
  sscanf (line, "%*s %lf", &FWHMx);
  FWHMy = FWHMx;
  angle = 0;
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);
  scan_line (f, line);

  /* read in data from obj file */
  /* data is not fixed format for lines, read each line one-at-a-time */
  for (i = 0; (status = fscanf (f, "%lf %lf %*f %*f %lf %lf %*f", &x, &y, &sky, &flux)) != EOF; i++) {

    if (status != 4) {
      fprintf (stderr, "format error in file %s, line %d\n", filename, i);
      continue;
    }

    if (flux <= 0) continue;
    m = -2.5*log10 (flux);

    if (sky < 1.0) {
      lsky = 0.0;
    } else {
      lsky = log10(sky);
    }

    bin = MAX (0.0, MIN (HIST_BINS, 10.0 * (m + 15.0)));  /* stick in 0.1 mag bins */
    Mhist[bin] ++;

    m = MIN (50.0, m);
    m = MAX (-24.0, m);

    stars[Nstars].fx = 0;
    stars[Nstars].fy = 0;
    stars[Nstars].df = 0;
    stars[Nstars].Mgal = 50.0;;
    stars[Nstars].Map = 50.0;
    stars[Nstars].X = x;
    stars[Nstars].Y = y;
    stars[Nstars].M = m;
    stars[Nstars].dM = 0.01;
    stars[Nstars].dophot = 1;
    stars[Nstars].sky = lsky;
    Nstars++;
    if (Nstars == NSTARS - 1) {
      NSTARS += 500;
      REALLOCATE (stars, SMPData, NSTARS);
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
      saturate = 0.1*(i-1) - 15.0;
      satfound = TRUE;
    }
    if (Shist[i] > 0.9*Shist[HIST_BINS - 1]) {
      complete = 0.1*i - 15.0;
      done = TRUE;
    }
  }
  
  gfits_modify (header, "ZERO_PT", "%lf", 1, ZERO_POINT);
  gfits_modify (header, "FWHM_X", "%lf", 1, FWHMx);
  gfits_modify (header, "FWHM_Y", "%lf", 1, FWHMy);
  gfits_modify (header, "ANGLE", "%lf", 1, angle);
  gfits_modify (header, "FSATUR", "%lf", 1, (saturate + ZERO_POINT));
  gfits_modify (header, "FLIMIT", "%lf", 1, (complete + ZERO_POINT));
  gfits_modify (header, "NSTARS", "%d", 1, N);
  for (i = 1; i <= 9; i++) {
    sprintf (line, "TDOPHOT%1d", i);
    gfits_modify (header, line, "%d", 1, 0);
  }

  *nstars = Nstars;
  return (stars);

}
