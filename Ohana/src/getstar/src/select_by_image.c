# include "getstar.h"

Stars *select_by_image (Catalog *catalog, Image *image, int start, int end, Stars *stars, int *Nstar) {

  int i, n, N, NSTARS;
  int photcode;

  if (stars == (Stars *) NULL) {
    N = 0;
    NSTARS = 1000;
    ALLOCATE (stars, Stars, NSTARS);
  } else {
    N = *Nstar;
    NSTARS = N + 1000;
    REALLOCATE (stars, Stars, NSTARS);
  }    

  /* identify selection criteria */
  photcode = -1;
  if ((start == 0) && (end == 0)) {
    start = image[0].tzero;
    end   = image[0].tzero + 1e-4*image[0].NY*image[0].trate;  /* trate is in 0.1 msec / row */
    photcode = image[0].photcode;
  }
  if (VERBOSE) fprintf (stderr, "extracting for range %d to %d (photcode %s)\n", start, end, photcode);

  for (i = 0; (i < catalog[0].Nmeasure); i++) {
    if ((i % 10000) == 0) fprintf (stderr, ". ");
    if ((catalog[0].measure[i].t >= start) && (catalog[0].measure[i].t <= end) && (photcode == catalog[0].measure[i].photcode)) { 
      n = catalog[0].measure[i].averef;
      stars[N].R      = catalog[0].average[n].R - catalog[0].measure[i].dR / 360000.0;
      stars[N].D      = catalog[0].average[n].D - catalog[0].measure[i].dD / 360000.0;

      stars[N].M      = 0.001*(catalog[0].measure[i].M - catalog[0].measure[i].dt);
      stars[N].dM     = catalog[0].measure[i].dM;
      stars[N].dophot = catalog[0].measure[i].dophot;  

      stars[N].Mgal   = 0.001*(catalog[0].measure[i].Mgal - catalog[0].measure[i].dt);

      stars[N].fx     = FromShortPixels(catalog[0].measure[i].FWx);
      stars[N].fy     = FromShortPixels(catalog[0].measure[i].FWy);
      stars[N].df     = FromShortDegrees(catalog[0].measure[i].theta);
      stars[N].found  = catalog[0].measure[i].flags;

      N ++;
      if (N == NSTARS) {
	NSTARS += 1000;
	REALLOCATE (stars, Stars, NSTARS);
      }    

    } 
  }
  fprintf (stderr, "found %d meas\n", N);
  *Nstar = N;
  return (stars);
}

