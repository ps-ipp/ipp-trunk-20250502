# include <ohana.h>

// return the index of the last value < threshold 
int ohana_bisection_double (double *values, int Nvalues, double threshold) {

  int Nlo = 0; 
  int Nhi = Nvalues - 1;

  if (Nvalues < 1) return (-1);
  if (values[Nlo] > threshold) return (-1);

  if (Nvalues < 2) return (0);
  if (values[Nhi] < threshold) return (Nhi);

  int N;
  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (values[N] < threshold) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nvalues - 1);
    }
  }
  // values[Nlo] < threshold 
  // values[Nhi] >= threshold 

  for (N = Nlo; N < Nhi; N++) {
    if (values[N] >= threshold) {
      return (N-1);
    }
  }
  return (N);
}

// return the index of the last value < threshold 
int ohana_bisection_int (int *values, int Nvalues, int threshold) {

  int Nlo = 0; 
  int Nhi = Nvalues - 1;

  if (Nvalues < 1) return (-1);
  if (values[Nlo] > threshold) return (-1);

  if (Nvalues < 2) return (0);
  if (values[Nhi] < threshold) return (Nhi);

  int N;
  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (values[N] < threshold) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nvalues - 1);
    }
  }
  // values[Nlo] < threshold 
  // values[Nhi] >= threshold 

  for (N = Nlo; N < Nhi; N++) {
    if (values[N] >= threshold) {
      return (N-1);
    }
  }
  return (N);
}
