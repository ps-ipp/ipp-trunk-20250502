# include "astro.h"

// this function is a hack to mask bad columns / rows in an image.  we assume that there
// are columns or rows which are, as a whole, significant deviant from the others.
// in fact, we measure this by segments

// SIGMA is a robust measure of the sigma, STDEV is the formal stdev of the values in range
int vector_stats (float *vect, int Nvect, float *MEDIAN, float *SIGMA, float *STDEV);

int fixrows (int argc, char **argv) {
  
  int ix, iy, Nx, Ny, Nvect, start, stop;
  float *Vin, *vect, *meds, median, stdev, sigma, Nsigma, value;
  Buffer *in;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fixrows (buffer) (col_start) (col_stop) (Nsigma) (value)\n");
    return (FALSE);
  }

  if ((in = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  start = atoi (argv[2]);
  stop = atoi (argv[3]);
  Nsigma = atof (argv[4]);
  value = atof (argv[5]);

  Nx = in[0].matrix.Naxis[0];
  Ny = in[0].matrix.Naxis[1];

  ALLOCATE (meds, float, Ny);
  ALLOCATE (vect, float, stop - start);

  Vin  = (float *)in[0].matrix.buffer;

  // measure the median for each column segment, accumulate the stats
  for (iy = 0; iy < Ny; iy++) {

    Nvect = 0;
    for (ix = start; ix < stop; ix++) {
      vect[Nvect] = Vin[ix + iy*Nx];
      Nvect ++;
    }

    // get the median, robust sigma, and formal stdev of the column segment
    vector_stats (vect, Nvect, &median, &sigma, &stdev);
    meds[iy] = median;
    // fprintf (stderr, "median : %f +/- %f (%f)\n", median, sigma, stdev);
  }

  // get the median, robust sigma, and formal stdev of the medians
  vector_stats (meds, Nx, &median, &sigma, &stdev);
  // fprintf (stderr, "median of medians : %f +/- %f (%f)\n", median, sigma, stdev);

  // mask values which are more than Nsigma out of range
  for (iy = 0; iy < Ny; iy++) {
    if (fabs(meds[iy] - median) < Nsigma * sigma) continue;
    for (ix = start; ix < stop; ix++) {
      Vin[ix + iy*Nx] = value;
    }
  }
  
  free (meds);
  free (vect);

  return (TRUE);
}

int fixcols (int argc, char **argv) {
  
  int ix, iy, Nx, Nvect, start, stop;
  float *Vin, *vect, *meds, median, stdev, sigma, Nsigma, value;
  Buffer *in;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fixcols (buffer) (row_start) (row_stop) (Nsigma) (value)\n");
    return (FALSE);
  }

  if ((in = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  start = atoi (argv[2]);
  stop = atoi (argv[3]);
  Nsigma = atof (argv[4]);
  value = atof (argv[5]);

  Nx = in[0].matrix.Naxis[0];
  // Ny = in[0].matrix.Naxis[1];

  ALLOCATE (meds, float, Nx);
  ALLOCATE (vect, float, stop - start);

  Vin  = (float *)in[0].matrix.buffer;

  // measure the median for each column segment, accumulate the stats
  for (ix = 0; ix < Nx; ix++) {

    Nvect = 0;
    for (iy = start; iy < stop; iy++) {
      vect[Nvect] = Vin[ix + iy*Nx];
      Nvect ++;
    }

    // get the median, robust sigma, and formal stdev of the column segment
    vector_stats (vect, Nvect, &median, &sigma, &stdev);
    meds[ix] = median;
    // fprintf (stderr, "median : %f +/- %f (%f)\n", median, sigma, stdev);
  }

  // get the median, robust sigma, and formal stdev of the medians
  vector_stats (meds, Nx, &median, &sigma, &stdev);
  // fprintf (stderr, "median of medians : %f +/- %f (%f)\n", median, sigma, stdev);

  // mask values which are more than Nsigma out of range
  for (ix = 0; ix < Nx; ix++) {
    if (fabs(meds[ix] - median) < Nsigma * sigma) continue;
    for (iy = start; iy < stop; iy++) {
      Vin[ix + iy*Nx] = value;
    }
  }
  
  free (meds);
  free (vect);

  return (TRUE);
}

// this macro is accumulating the histogram until the sum passes the desired threshold
// point the fractional position of that threshold is found by interpolation
# define SUMPOINT(POINT, NPOINT, FRACTION)				\
  if ((NPOINT) == -1) {							\
    (POINT) += Nval[i];							\
    if ((POINT) >= FRACTION*N) {					\
      (NPOINT) = i;							\
      value = i * dx + min;						\
      if (Nval[i] != 0) {						\
	/* POINT for the bin center is not exactly FRACTION*N */	\
	/* interpolate to the true value, but limit the scale of the */	\
	/* interpolation to a single bin */				\
	adjust = dx * (FRACTION*N - (POINT)) / Nval[i];			\
	value += MAX (MIN (adjust, dx), -dx);				\
      }									\
      (POINT) = value;							\
    }									\
  }

// for testing, this is a useful line:
// fprintf (stderr, "for point %f (%f): bin: %d = %f, dx: %f, adjust: %f, value: %f, POINT: %f, Nval[i]: %d, Nval[i-1]: %d\n",
// FRACTION, FRACTION*N, i, i*dx + min, dx, adjust, value, POINT, Nval[i], Nval[i-1]);

int vector_stats (float *vect, int Nvect, float *MEDIAN, float *SIGMA, float *STDEV) {
  
  int i, iter, N, done;
  double max, min, sum, var, dvar, mean, stdev, sigma, adjust, value;
  int Nval[1001], bin, Nmode, N_MED, N_LQ, N_UQ, N_SM, N_SP;
  double dx, mode, MED, UQ, LQ, SM, SP;
  float *X;

  /* we need two passes, one for max, min, mean, sum, one for median, stdev, etc */

  /* calculate max, min, mean, sum, npix */
  max = -HUGE_VAL;
  min = HUGE_VAL;
  sum = N = 0;
  X = vect;
  for (i = 0; i < Nvect; i++, X++) {
    if (!finite (*X)) continue;
    max = MAX (*X, max);
    min = MIN (*X, min);
    sum += *X;
    N++;
  }      
  mean = sum / N;

  // calculate median and mode with initial resolution dx = (max - min) / 1000 
  // we adjust the resolution and retry if the resulting robust sigma < dx
  dx = (max - min) / 1000;
  if (dx == 0) {
    MED = mode = min;
    stdev = 0.0;
    sigma = 0.0;
    goto skip;
  }

  done = FALSE;
  for (iter = 0; (iter < 3) && !done; iter ++) {

    bzero (Nval, 1001*sizeof(int));
    var = 0;
    X = vect;
    for (i = 0; i < Nvect; i++, X++) {
      if (!finite (*X)) continue;
      bin = MAX (0, MIN (1000, (*X - min) / dx));
      Nval[bin] ++;
      if (bin == 0) continue;
      if (bin == 1000) continue;
      // only count the values in range for the stdev
      dvar = (*X - mean);
      var += dvar*dvar;
    }      
    stdev = sqrt (var / N);

    Nmode = 0;
    mode = Nval[Nmode];
    MED = SM = SP = LQ = UQ = 0;
    N_MED = N_SM = N_SP = N_LQ = N_UQ = -1;
    for (i = 0; i < 1001; i++) {
      SUMPOINT (MED, N_MED, 0.5000);
      SUMPOINT (LQ,  N_LQ,  0.2500);
      SUMPOINT (UQ,  N_UQ,  0.7500);
      SUMPOINT (SM,  N_SM,  0.1587);
      SUMPOINT (SP,  N_SP,  0.8413);

      if (mode < Nval[i]) {
	Nmode = i;
	mode = Nval[Nmode];
      }
    }

    sigma = 0.5*(SP - SM);
    mode = Nmode * dx + min;

    // if (sigma < dx), we adjust the range to be centered on the median
    if (sigma < dx) {
      dx *= 0.1;
      min = MED - 500*dx;
    } else {
      done = TRUE;
    }
  }

skip:

  *MEDIAN = MED;
  *SIGMA = sigma;
  *STDEV = stdev;

  // set_variable ("MIN",      min);
  // set_variable ("MAX",      max);
  // set_variable ("MEAN",     mean);
  // set_variable ("MODE",     mode);
  // set_variable ("TOTAL",    sum);
  // set_int_variable ("NPIX", N);
  // set_variable ("SIGMA",    stdev);

  return (TRUE);
}

