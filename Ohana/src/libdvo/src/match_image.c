# include "dvo.h"

off_t match_image (Image *image, off_t Nimage, e_time T, short int S) {

  off_t N, Nlo, Nhi, N1, N2;

  /* bracket first value of interest */
  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (image[N].tzero < T) {
      Nlo = N;
    } else {
      Nhi = N + 1;
    }
  }
  N1 = Nlo;

  /* bracket last value of interest */
  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (image[N].tzero > T) {
      Nhi = N;
    } else {
      Nlo = N - 1;
    }
  }
  N2 = Nhi;

  for (N = N1; N < N2; N++) {
    if ((image[N].tzero == T) && (image[N].photcode == S)) {
      return (N);
    }
  }
  return (-1);
}

off_t match_image_subset (Image *image, off_t *subset, off_t Nsubset, e_time T, short int S) {

  off_t N, Nlo, Nhi, N1, N2;

  /* bracket first value of interest */
  Nlo = 0; Nhi = Nsubset;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (image[subset[N]].tzero < T) {
      Nlo = N;
    } else {
      Nhi = N + 1;
    }
  }
  N1 = Nlo;

  /* bracket last value of interest */
  Nlo = 0; Nhi = Nsubset;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (image[subset[N]].tzero > T) {
      Nhi = N;
    } else {
      Nlo = N - 1;
    }
  }
  N2 = Nhi;

  for (N = N1; N < N2; N++) {
    if ((image[subset[N]].tzero == T) && (image[subset[N]].photcode == S)) {
      return (subset[N]);
    }
  }
  return (-1);
}
