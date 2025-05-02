# include "fakeastro.h"

static int Ngaussint = 0;
static double *gaussint;

extern double drand48();

double gaussian (double x, double mean, double sigma) {

  double f;

  f = exp (-0.5 * SQ(x - mean) / SQ(sigma)) / sqrt(2 * M_PI * SQ(sigma));

  return (f);

}

/* integrate a gaussian from -5 sigma to +5 sigma */
void gauss_init (int Nbin) {
 
  int i;
  long A, B;
  double val, x, dx, dx1, dx2, dx3, df;
  double mean, sigma;
 
  /* no need to generate this if it already exists */
  if (Ngaussint == Nbin) return;

  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);
 
  Ngaussint = Nbin;
  ALLOCATE (gaussint, double, Ngaussint + 1);

  val = 0;
  dx = 1.0 / Ngaussint;
  dx1 = dx / 3.0;
  dx2 = 2.0*dx/3.0;
  dx3 = dx;
  mean = 0.0;
  sigma = 1.0;
 
  for (i = 0, x = -7.0; (i < Ngaussint) && (x < 7.0); x += dx)  {
    df = (3.0*gaussian(x    , mean, sigma) + 
          9.0*gaussian(x+dx1, mean, sigma) +
          9.0*gaussian(x+dx2, mean, sigma) + 
          3.0*gaussian(x+dx3, mean, sigma)) * (dx1/8.0);
    val += df;
    if (val > (i + 0.5) / (double) Ngaussint) {
      gaussint[i] = x + dx / 2.0;
      i++;
    }
  }
}

double rnd_gauss (double mean, double sigma) {
 
  int i;
  double y;
 
  y = drand48();
  i = Ngaussint*y;
  y = gaussint[i]*sigma + mean;
 
  return (y);
 
}
 
double int_gauss (int i) {
  double y;
  y = gaussint[i];
  return (y);
}
 
