# include <ohana.h>

// NOTE: this code uses a local static array which must be freed on exit 
static int Ngaussint = 0;
static double *gaussint = NULL;

// extern double drand48();

// return the value of a gaussian at position x with mean,sigma
double ohana_gaussian (double x, double mean, double sigma) {

  double f;

  double S2 = sigma*sigma;

  f = exp (-0.5 * SQ(x - mean) / S2) / sqrt(2 * M_PI * S2);

  return (f);
}

/* integrate a gaussian from -10 sigma to +10 sigma */
// how long does this take with a reasonably high resolution?
void ohana_gaussdev_init (void) {
 
  int i;
  double val, x, dx, dx1, dx2, dx3, df;
  double mean, sigma;
 
  if (gaussint) return;

  // 16k bins takes ~0.1 sec to generate on a ~3GHz core and gives good resolution
  Ngaussint = 0x4000;
  ALLOCATE (gaussint, double, Ngaussint + 1);

  val = 0;
  dx = 1.0 / Ngaussint;
  dx1 = dx / 3.0;
  dx2 = 2.0*dx/3.0;
  dx3 = dx;
  mean = 0.0;
  sigma = 1.0;
 
  for (i = 0, x = -10.0; (i < Ngaussint) && (x <= 10.0); x += dx)  {
    df = (3.0*ohana_gaussian(x    , mean, sigma) + 
          9.0*ohana_gaussian(x+dx1, mean, sigma) +
          9.0*ohana_gaussian(x+dx2, mean, sigma) + 
          3.0*ohana_gaussian(x+dx3, mean, sigma)) * (dx1/8.0);
    val += df;
    if (val > (i + 0.5) / (double) Ngaussint) {
      gaussint[i] = x + dx / 2.0;
      myAssert (i < Ngaussint + 1, "oops");
      i++;
    }
  }
}

void ohana_gaussdev_free (void) {
  free (gaussint);
}

// return a number drawn from a gaussian deviate with mean, sigma
// must first call gaussdev_init()
double ohana_gaussdev_rnd (double mean, double sigma) {
 
  int i;
  double y;
 
  myAssert (gaussint, "need to call gaussdev_init before calling gaussdev");

  y = drand48();
  i = Ngaussint*y;
  y = gaussint[i]*sigma + mean;
 
  return (y);
}
 
// return the value of \int gaussian(x,mean,sigma) 
// double gaussian_integral (double x, double mean, double sigma) {
double ohana_gaussian_integral (int i) {

  // int i = (x - mean) / (sigma * Ngaussint);
  return gaussint[i];
}
