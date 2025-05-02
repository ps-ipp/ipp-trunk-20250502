# include <dvo.h>

int dvo_match_image (Image *image, int Nimage, int T, short int S) {

  int N, Nlo, Nhi, N1, N2;

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

// if necessary, determine chip coordinates for all measures
int dvo_catalog_chipcoords (Catalog *catalog, Image *image, int Nimage) {

  int i, j, m, N;
  double ra, dec, x, y;
  Average *average;
  Measure *measure;

  if (catalog[0].catformat == DVO_FORMAT_LONEOS) goto do_convert;  // special conversion for LONEOS
  if (catalog[0].catformat == DVO_FORMAT_ELIXIR) goto do_convert;  // special conversion for ELIXIR
  return TRUE;

do_convert:
  average = catalog[0].average;
  measure = catalog[0].measure;
  
  for (i = 0; i < catalog[0].Naverage; i++) {
    m = average[i].measureOffset;
    for (j = 0; j < average[i].Nmeasure; j++, m++) {
      ra  = measure[m].R;
      dec = measure[m].D;
      N = dvo_match_image (image, Nimage, measure[m].t, measure[m].photcode);
      if (N == -1) continue;
      RD_to_XY (&x, &y, ra, dec, &image[N].coords);
      measure[m].Xccd = x;
      measure[m].Yccd = y;
    }
  }
  return TRUE;
}
