# include "astro.h"

int MeanSurfaceBrightness (Vector *rvec, Vector *fvec, Vector *Rvec, Vector *Fvec, Vector *Avec, float Rmin, float Rmax, int bin);

// given a collection of r, f points sampled at pixels, generate a pair of vectors r, f
// where f is defined as the mean surface brightness for \alpha r_i < r < \beta r_i.
// sample r at r_i = i

// this function can be much more efficient if the input vectors are sorted by R and that
// fact is used in generating the radial bins...
int galradbins (int argc, char **argv) {
  
  int i, Nbin;
  double Rmax;
  Vector *fvec, *rvec, *Fvec, *Rvec, *Avec;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: galradius (r_in) (f_in) (r_out) (f_out) (area_out)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((fvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Rvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fvec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Avec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Rmax = rvec[0].elements.Flt[0];
  for (i = 0; i < rvec[0].Nelements; i++) {
    Rmax = MAX(Rmax, rvec[0].elements.Flt[i]);
  }
  Nbin = 0.8*Rmax + 2;
  ResetVector (Rvec, OPIHI_FLT, Nbin);
  ResetVector (Fvec, OPIHI_FLT, Nbin);
  ResetVector (Avec, OPIHI_FLT, Nbin);

  // the first three bins are specially defined:

  // 0 : r < 1.0 -- Area is pi
  MeanSurfaceBrightness (rvec, fvec, Rvec, Fvec, Avec, 0.00, 1.00, 0);

  // area is \int 2 \pi r dr = \pi (r2^2 - r1^2)

  // 1 : 1.0 < r < 1.25 -- Area is \pi (1.25^2 - 1^2)
  MeanSurfaceBrightness (rvec, fvec, Rvec, Fvec, Avec, 1.00, 1.25, 1);

  // 2 : 1.25 < r < 1.60 -- Area is \pi (1.6^2 - 1.25^2)
  MeanSurfaceBrightness (rvec, fvec, Rvec, Fvec, Avec, 1.25, 1.60, 2);

  // from bin 3 on out, r = i - 1 
  for (i = 3; i < Nbin; i++) {
    MeanSurfaceBrightness (rvec, fvec, Rvec, Fvec, Avec, 0.8*(i-1), 1.25*(i-1), i);
  }

  return (TRUE);
}

int MeanSurfaceBrightness (Vector *rvec, Vector *fvec, Vector *Rvec, Vector *Fvec, Vector *Avec, float Rmin, float Rmax, int bin) {

  int i, Npts;
  double Fsum;

  Fsum = Npts = 0;

  for (i = 0; i < rvec[0].Nelements; i++) {
    if (rvec[0].elements.Flt[i] < Rmin) continue;
    if (rvec[0].elements.Flt[i] > Rmax) continue;
    Fsum += fvec[0].elements.Flt[i];
    Npts ++;
  }
  if (Rmin > 0.0) {
    Rvec[0].elements.Flt[bin] = sqrt(Rmin * Rmax); // XXX what is the correct mean radius?
  } else {
    Rvec[0].elements.Flt[bin] = 0.5 * Rmax; // XXX what is the correct mean radius?
  }
  if (Npts > 0) {
    Fvec[0].elements.Flt[bin] = Fsum / Npts; // XXX what is the correct mean radius?
  } else {
    Fvec[0].elements.Flt[bin] = NAN; // XXX what is the correct mean radius?
  }
  Avec[0].elements.Flt[bin] = M_PI * (SQ(Rmax) - SQ(Rmin));
  return (TRUE);
}
