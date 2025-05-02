# include "ppSim.h"

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
double ppSimRandomGaussian (const psRandom *rnd, double mean, double sigma) {

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
double ppSimRandomGaussianNorm (const psRandom *rnd) {

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
