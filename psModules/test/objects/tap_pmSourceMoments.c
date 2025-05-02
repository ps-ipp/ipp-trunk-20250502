#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

# define GAIN 1.0
# define RDNOISE 10.0

float Gaussian(float Io, float sigma, float Xc, float Yc, int ix, int iy);
bool MakeGaussian (pmSource *source, float Io, float sigma, float Xc, float Yc);
bool MakeGaussianNoNoise (pmSource *source, float Io, float sigma, float Xc, float Yc);
double ppSimRandomGaussianNorm (void);
double ppSimRandomGaussian (double mean, double sigma);

bool GaussiansWindowsAndTopHatsNoNoise (pmSource *source, float Io, float sigma, float Xc, float Yc);
bool GaussiansWindowsAndTopHats (pmSource *source, float Io, float sigma, float Xc, float Yc);
bool CentroidWithGuessOffset (pmSource *source, float Io, float sigma, float Xc, float Yc, float dX, float dY);
bool CentroidWithGuessOffsetIterate (pmSource *source, float Io, float sigma, float Xc, float Yc, float dX, float dY);

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(70);

    // ----------------------------------------------------------------------
    // pmSourceMomentsGetCentroid() tests (no noise)
    {
        psMemId id = psMemGetId();

	// generate a sample source (no noise)
	pmSource *source = pmSourceAlloc();
        ok(source, "source allocated");
	
	bool status = true;

	// need to have: peak, pixels, variance, 
	int Nx = 100;
	int Ny = 100;
	float Xc = 52.0;
	float Yc = 48.0;
	float Io = 100.0;
	float sigma = 4.0 / 2.35;

	source->peak = pmPeakAlloc(Xc, Yc, Io, PM_PEAK_LONE);
	source->moments = pmMomentsAlloc();
	source->pixels = psImageAlloc(Nx, Ny, PS_TYPE_F32);
	source->variance = psImageAlloc(Nx, Ny, PS_TYPE_F32);
	
	// CentroidWithGuessOffset(source, Io, sigma, Xc, Yc, 0.0, 0.0);
	// CentroidWithGuessOffset(source, Io, sigma, Xc, Yc, 0.1, 0.0);
	// CentroidWithGuessOffset(source, Io, sigma, Xc, Yc, 0.0, 0.1);
	// CentroidWithGuessOffset(source, Io, sigma, Xc, Yc, 0.1, 0.1);
	// CentroidWithGuessOffset(source, Io, sigma, Xc, Yc, 0.3, 0.3);
	// CentroidWithGuessOffset(source, Io, sigma, Xc, Yc, 0.5, 0.5);

	psVector *dX = psVectorAlloc(100, PS_TYPE_F32);
	psVector *dY = psVectorAlloc(100, PS_TYPE_F32);

	for (int i = 0; i < dX->n; i++) {
	    CentroidWithGuessOffsetIterate(source, Io, sigma, Xc, Yc, 1.0, 0.0);
	    dX->data.F32[i] = Xc - source->moments->Mx;
	    dY->data.F32[i] = Yc - source->moments->My;
	}
	psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);

	psVectorStats (stats, dX, NULL, 0, 0);
	fprintf (stderr, "dX stats: %f +/- %f\n", stats->sampleMean, stats->sampleStdev);
	psStatsInit(stats);
	
	psVectorStats (stats, dY, NULL, 0, 0);
	fprintf (stderr, "dY stats: %f +/- %f\n", stats->sampleMean, stats->sampleStdev);
	psStatsInit(stats);
	
	ok(status, "measured centroid");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // ----------------------------------------------------------------------
    // pmSourceMomentsGetCentroid() tests (standard noise)
    if (0) {
        psMemId id = psMemGetId();

	// generate a sample source (no noise)
	pmSource *source = pmSourceAlloc();
        ok(source, "source allocated");
	
	bool status = true;

	// need to have: peak, pixels, variance, 
	int Nx = 100;
	int Ny = 100;
	float Xc = 52.0;
	float Yc = 48.0;
	float Io = 100000.0;
	float sigma = 4.0 / 2.35;

	source->peak = pmPeakAlloc(Xc, Yc, Io, PM_PEAK_LONE);
	source->moments = pmMomentsAlloc();
	source->pixels = psImageAlloc(Nx, Ny, PS_TYPE_F32);
	source->variance = psImageAlloc(Nx, Ny, PS_TYPE_F32);
	
	GaussiansWindowsAndTopHats(source, Io, sigma, Xc + 0.0, Yc + 0.0);

	ok(status, "measured centroid (window)");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

bool CentroidWithGuessOffsetIterate (pmSource *source, float Io, float sigma, float Xc, float Yc, float dX, float dY) {

    // pmSourceMomentsGetCentroid(source, radius, sigma, minSN, maskVal)
    source->peak->xf = Xc + dX;
    source->peak->yf = Yc + dY;

    // MakeGaussianNoNoise (source, Io, sigma, Xc, Yc);
    MakeGaussian (source, Io, sigma, Xc, Yc);

    // pmSourceMomentsGetCentroid(source, 10.0, 15.0 / 2.35, 0.0, 0);
    pmSourceMomentsGetCentroid(source, 8.0, 0.0, 0.0, 0);
    source->peak->xf = source->moments->Mx;
    source->peak->yf = source->moments->My;
    // fprintf (stderr, "%f,%f vs %f,%f : %f,%f -> %f,%f (SN = %f) ", Xc, Yc, source->moments->Mx, source->moments->My, dX, dY, Xc - source->moments->Mx, Yc - source->moments->My, source->moments->SN);

    pmSourceMomentsGetCentroid(source, 10.0, 8.0/2.35, 0.0, 0);
    // fprintf (stderr, "  ---> %f,%f\n", Xc - source->moments->Mx, Yc - source->moments->My);
    return true;
}

bool CentroidWithGuessOffset (pmSource *source, float Io, float sigma, float Xc, float Yc, float dX, float dY) {

    // pmSourceMomentsGetCentroid(source, radius, sigma, minSN, maskVal)
    source->peak->xf = Xc + dX;
    source->peak->yf = Yc + dY;

    // MakeGaussianNoNoise (source, Io, sigma, Xc, Yc);
    MakeGaussian (source, Io, sigma, Xc, Yc);

    // pmSourceMomentsGetCentroid(source, 10.0, 15.0 / 2.35, 0.0, 0);
    pmSourceMomentsGetCentroid(source, 8.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f : %f,%f -> %f,%f (SN = %f)\n", Xc, Yc, source->moments->Mx, source->moments->My, dX, dY, Xc - source->moments->Mx, Yc - source->moments->My, source->moments->SN);
    return true;
}

bool GaussiansWindowsAndTopHatsNoNoise (pmSource *source, float Io, float sigma, float Xc, float Yc) {

    // pmSourceMomentsGetCentroid(source, radius, sigma, minSN, maskVal)
    source->peak->xf = Xc;
    source->peak->yf = Yc;

    MakeGaussianNoNoise (source, Io, sigma, Xc, Yc);
    pmSourceMomentsGetCentroid(source, 8.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 10.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 15.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 20.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    ok(true, "measured centroid (tophat)");

    pmSourceMomentsGetCentroid(source,  8.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 10.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 15.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 20.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    ok(true, "measured centroid (window)");
    return true;
}

bool GaussiansWindowsAndTopHats (pmSource *source, float Io, float sigma, float Xc, float Yc) {

    // pmSourceMomentsGetCentroid(source, radius, sigma, minSN, maskVal)
    source->peak->xf = Xc;
    source->peak->yf = Yc;

    MakeGaussian (source, Io, sigma, Xc, Yc);
    pmSourceMomentsGetCentroid(source, 8.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 10.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 15.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 20.0, 0.0, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    ok(true, "measured centroid (tophat)");

    pmSourceMomentsGetCentroid(source,  8.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 10.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 15.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    pmSourceMomentsGetCentroid(source, 20.0, 8.0 / 2.35, 0.0, 0);
    fprintf (stderr, "%f,%f vs %f,%f = %f,%f\n", Xc, Yc, source->moments->Mx, source->moments->My, Xc - source->moments->Mx, Yc - source->moments->My);

    ok(true, "measured centroid (window)");
    return true;
}

bool MakeGaussianNoNoise (pmSource *source, float Io, float sigma, float Xc, float Yc) {

  psImageInit(source->pixels, 0.0);
  psImageInit(source->variance, 0.0);

  // populate the pixels with an object (Gaussian with Io, sigma)
  for (int iy = 0; iy < source->pixels->numRows; iy++) {
    for (int ix = 0; ix < source->pixels->numCols; ix++) {
      source->pixels->data.F32[iy][ix] = Gaussian(Io * GAIN, sigma, Xc, Yc, ix, iy);
      source->variance->data.F32[iy][ix] = source->pixels->data.F32[iy][ix] + PS_SQR(RDNOISE); // RDNOISE and initial signal in electrons
      source->pixels->data.F32[iy][ix] /= GAIN;
      source->variance->data.F32[iy][ix] /= GAIN;
    }
  }
  return true;
}

bool MakeGaussian (pmSource *source, float Io, float sigma, float Xc, float Yc) {

  psImageInit(source->pixels, 0.0);
  psImageInit(source->variance, 0.0);

  // populate the pixels with an object (Gaussian with Io, sigma)
  for (int iy = 0; iy < source->pixels->numRows; iy++) {
    for (int ix = 0; ix < source->pixels->numCols; ix++) {
      source->pixels->data.F32[iy][ix] = Gaussian(Io * GAIN, sigma, Xc, Yc, ix, iy);
      source->variance->data.F32[iy][ix] = source->pixels->data.F32[iy][ix] + PS_SQR(RDNOISE); // RDNOISE and initial signal in electrons
      source->pixels->data.F32[iy][ix] += sqrtf(source->variance->data.F32[iy][ix]) * ppSimRandomGaussianNorm();
      source->pixels->data.F32[iy][ix] /= GAIN;
      source->variance->data.F32[iy][ix] /= GAIN;
    }
  }
  return true;
}

float Gaussian(float Io, float sigma, float Xc, float Yc, int ix, int iy) {
  
  float radius2 = PS_SQR(ix + 0.5 - Xc) + PS_SQR(iy + 0.5 - Yc);
  float exponent = -0.5 * radius2 / PS_SQR(sigma);

  float value = Io * exp(exponent);
  return value;
}

/// **** this stuff should be in psLib -- it is too useful...

static int Ngaussint = 0;
static double *gaussint = NULL;

extern double drand48();

double p_ppSimGaussian (double x, double mean, double sigma) {

  double f;

  f = exp (-0.5 * PS_SQR(x - mean) / PS_SQR(sigma)) / sqrt(2 * M_PI * PS_SQR(sigma));

  return (f);

}

void ppSimRandomGaussianFree()
{
    psFree (gaussint);
    return;
}

void ppSimRandomGaussianAlloc (int Nbin) {

    gaussint = (double *) psAlloc(Nbin*sizeof(double));
    return;
}

/* integrate a gaussian from -5 sigma to +5 sigma */
void p_ppSimRandomGaussianInit (void) {

  int i;
  long A, B;
  double val, x, dx, dx1, dx2, dx3, df;
  double mean, sigma;

  /* no need to generate this if it already exists */
  if (gaussint) return;

  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);

  Ngaussint = 0x1000;
  ppSimRandomGaussianAlloc (Ngaussint + 1);

  val = 0;
  dx = 1.0 / Ngaussint;
  dx1 = dx / 3.0;
  dx2 = 2.0*dx/3.0;
  dx3 = dx;
  mean = 0.0;
  sigma = 1.0;

  for (i = 0, x = -7.0; (i < Ngaussint) && (x < 7.0); x += dx)  {
    df = (3.0*p_ppSimGaussian(x    , mean, sigma) +
          9.0*p_ppSimGaussian(x+dx1, mean, sigma) +
          9.0*p_ppSimGaussian(x+dx2, mean, sigma) +
          3.0*p_ppSimGaussian(x+dx3, mean, sigma)) * (dx1/8.0);
    val += df;
    if (val > (i + 0.5) / (double) Ngaussint) {
      gaussint[i] = x + dx / 2.0;
      i++;
    }
  }
}

// XXX we are using drand48() rather than the random var supplied by rnd
double ppSimRandomGaussian (double mean, double sigma) {

  int i;
  double y;

  if (gaussint == NULL) {
      p_ppSimRandomGaussianInit ();
  }

  y = drand48();
  i = Ngaussint*y;
  y = gaussint[i]*sigma + mean;

  return (y);

}

// XXX we are using drand48() rather than the random var supplied by rnd
double ppSimRandomGaussianNorm (void) {

  int i;
  double y;

  if (gaussint == NULL) {
      p_ppSimRandomGaussianInit ();
  }

  y = drand48();
  i = Ngaussint*y;
  y = gaussint[i];

  return (y);
}
