# include "data.h"

/* fast operation to find an entry just below (0) or above (1) value */
/* input list must be sorted */
int bracket (double *list, int Nlist, int mode, double value) {

  int Nlo, Nhi, N;

  if (mode == 0) {
    Nlo = 0; Nhi = Nlist;
    while (Nhi - Nlo > 10) {
      N = 0.5*(Nlo + Nhi);
      if (list[N] < value) {
	Nlo = N;
      } else {
	Nhi = N + 1;
      }
    }
    return (Nlo);
  }
  if (mode == 1) {
    Nlo = 0; Nhi = Nlist;
    while (Nhi - Nlo > 10) {
      N = 0.5*(Nlo + Nhi);
      if (list[N] > value) {
	Nhi = N;
      } else {
	Nlo = N - 1;
      }
    }
    return (Nhi);
  }
  return (0);
}

int ibracket (int *list, int Nlist, int mode, double value) {

  int Nlo, Nhi, N;

  if (mode == 0) {
    Nlo = 0; Nhi = Nlist;
    while (Nhi - Nlo > 10) {
      N = 0.5*(Nlo + Nhi);
      if (list[N] < value) {
	Nlo = N;
      } else {
	Nhi = N + 1;
      }
    }
    return (Nlo);
  }
  if (mode == 1) {
    Nlo = 0; Nhi = Nlist;
    while (Nhi - Nlo > 10) {
      N = 0.5*(Nlo + Nhi);
      if (list[N] > value) {
	Nhi = N;
      } else {
	Nlo = N - 1;
      }
    }
    return (Nhi);
  }
  return (0);
}
