# include "astro.h"

int petrosian (int argc, char **argv) {
  
  int i, above;
  double Fsum, Asum, Area, R_90, rad_90, flux_90;
  Vector *rvec, *avec, *fvec, *Rvec, *Fvec, *Svec, *Avec;

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: petrosian (radius) (area) (flux) (P_ratio) (P_flux) (P_sb) (P_area)\n");
    return (FALSE);
  }

  /* select input vectors */
  if ((rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((avec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((fvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Rvec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fvec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Svec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Avec = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  // difficult work goes into cleaning the input galaxy profile:
  // a) extract radial profiles applying 180 degree symmetry requirement
  // b) determine the axial ratio and position angle (at r_50)
  // c) extract normalized radial profiles applying outlier clipping
  // 
  // the resulting vectors r, f represent the mean surface brightness at a radius r_i, 
  // measured between \alpha*r_i and \beta*r_i: f(r_i) = \sum_\alpha*r_i^\beta*r_i flux(r)

  // given a clean set of vectors representing the f_i, r_i, determine F_P, R_P such that:
  // f(r_i) / \sum_0^r_i f(r) = f_P 

  // generate a vector representing f_P(r_i):

  ResetVector (Fvec, OPIHI_FLT, fvec[0].Nelements);
  ResetVector (Rvec, OPIHI_FLT, fvec[0].Nelements);
  ResetVector (Svec, OPIHI_FLT, fvec[0].Nelements);
  ResetVector (Avec, OPIHI_FLT, fvec[0].Nelements);

  flux_90 = rad_90 = 0.0;
  R_90 = 0.1;
  above = TRUE;
  Fsum = 0.0;
  Asum = 0.0;
  Area = avec[0].elements.Flt[0];
  for (i = 0; i < fvec[0].Nelements; i++) {
    // for nan bins, we keep the area for use with the next valid bin
    if (isnan(fvec[0].elements.Flt[i])) {
      Area += avec[0].elements.Flt[i];
      continue;
    } 
    Fsum += fvec[0].elements.Flt[i] * Area;
    Asum += Area;
    if (i+1 < fvec[0].Nelements) {
      Area = avec[0].elements.Flt[i+1];
    }

    Rvec[0].elements.Flt[i] = Asum * fvec[0].elements.Flt[i] / Fsum;
    Fvec[0].elements.Flt[i] = Fsum;
    Svec[0].elements.Flt[i] = Fsum / Asum;
    Avec[0].elements.Flt[i] = Asum;

    // anytime we transition below the petrosian ratio R_90, calculate the radius and flux
    // we will keep and report the last (largest radius) value
    if (above && (Rvec[0].elements.Flt[i] < R_90)) {
      // interpolate Rvec between i-1 and i to R_90 to get flux (Fvec) and radius (rvec)

      if (i == 0) { 
	// assume Fmax @ R = 0.0
	rad_90  = rvec[0].elements.Flt[i] * (R_90 - 1.0) / (Rvec[0].elements.Flt[i] - 1.0);
	flux_90 = Fvec[0].elements.Flt[i] * (R_90 - 1.0) / (Rvec[0].elements.Flt[i] - 1.0);
      } else {
	rad_90  = rvec[0].elements.Flt[i-1] + (rvec[0].elements.Flt[i] - rvec[0].elements.Flt[i-1]) * (R_90 - Rvec[0].elements.Flt[i-1]) / (Rvec[0].elements.Flt[i] - Rvec[0].elements.Flt[i-1]);
	flux_90 = Fvec[0].elements.Flt[i-1] + (Fvec[0].elements.Flt[i] - Fvec[0].elements.Flt[i-1]) * (R_90 - Rvec[0].elements.Flt[i-1]) / (Rvec[0].elements.Flt[i] - Rvec[0].elements.Flt[i-1]);
      }

      above = FALSE;
    }
    
    // reset on transitions up, but do not re-calculate rad_90, flux_90
    if (!above && (Rvec[0].elements.Flt[i] >= R_90)) {
      above = TRUE;
    }
  }

  set_variable ("P_R90", rad_90);
  set_variable ("P_F90", flux_90);

  return (TRUE);
}
