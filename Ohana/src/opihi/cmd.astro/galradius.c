# include "astro.h"

// given an image and associated galaxy profile, find:
// a) a characteristic flux level that defines a useful isophot
// b) radius at which the profile crosses that isophot
int galradius (int argc, char **argv) {
  
  int i, j, N, Rbin, Nout, above;
  double Rmax, Rmin;
  double Fmin, Fmax, dF, Fm, Fp, Fo, Rsum, Rnpt, Ro;
  opihi_flt *flux, *radius, *values;
  Vector *fvec, *rvec, *Fvec, *Rvec;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: galradius (radius) (flux) (min_flux) (max_flux)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((fvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if ((Rvec = SelectVector ("rg", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fvec = SelectVector ("fg", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Fmin = atof(argv[3]);
  Fmax = atof(argv[4]);

  // fvec is a noise sample of the galaxy radial profile at points rvec
  // rebin fvec into samples defined by the isophot 

  // base selections on fluxes defined by the flux range dF
  dF = Fmax - Fmin;

  // select all points with flux in the range Fmin + 0.25*dF to Fmin + 0.75*dF
  Fm = Fmin + 0.25*dF;
  Fp = Fmin + 0.75*dF;
  Fo = Fmin + 0.50*dF;
      
  Rsum = 0;
  Rnpt = 0;
  for (i = 0; i < fvec[0].Nelements; i++) {
    if (fvec[0].elements.Flt[i] < Fm) continue;
    if (fvec[0].elements.Flt[i] > Fp) continue;
      
    Rsum += rvec[0].elements.Flt[i];
    Rnpt ++;
  }

  // determine the binned radius values
  if (Rnpt == 0) {
    Rbin = 1;
  } else {
    Rbin = MAX(1, 0.5*(Rsum / Rnpt));
  }

  // do not bother rebinning if the bin size is only 2 or less
  if (Rbin <= 2) {
    Nout   = fvec[0].Nelements;
    ResetVector (Rvec, OPIHI_FLT, Nout);
    ResetVector (Fvec, OPIHI_FLT, Nout);
    memcpy (Fvec[0].elements.Flt, fvec[0].elements.Flt, Fvec[0].Nelements*sizeof(opihi_flt));
    memcpy (Rvec[0].elements.Flt, rvec[0].elements.Flt, Rvec[0].Nelements*sizeof(opihi_flt));
    flux   = fvec[0].elements.Flt;
    radius = rvec[0].elements.Flt;
  } else {
    // rebin the vectors by Rbin values:
    Nout = fvec[0].Nelements / Rbin + 1;
    ALLOCATE (values, opihi_flt, fvec[0].Nelements);
  
    ResetVector (Rvec, OPIHI_FLT, Nout);
    ResetVector (Fvec, OPIHI_FLT, Nout);
    flux   = Fvec[0].elements.Flt;
    radius = Rvec[0].elements.Flt;

    for (i = 0; i < Nout; i++) {
      radius[i] = (i + 0.5)*Rbin;
      Rmin = radius[i] - 0.5*Rbin;
      Rmax = radius[i] + 0.5*Rbin;

      N = 0;
      for (j = 0; j < fvec[0].Nelements; j++) {
	if (rvec[0].elements.Flt[j] < Rmin) continue;
	if (rvec[0].elements.Flt[j] > Rmax) continue;
	values[N] = fvec[0].elements.Flt[j];
	N++;
      }

      // take the median of the values
      dsort (values, N);
      if (N > 1) {
	flux[i] = (N % 2) ? values[(int)(0.5*N)] : 0.5*(values[N/2] + values[N/2 + 1]);
      } else {
	flux[i] = values[0];
      }
    }
    free (values);
  }

  above = TRUE;
  Ro = 0;
  for (i = 0; i < Nout; i++) {

    // find the largest radius that matches the flux transition
    if (above && (flux[i] < Fo)) {
      if (i == 0) { 
	// assume Fmax @ R = 0.0
	Ro = radius[i] * (Fo - Fmax) / (flux[i] - Fmax);
      } else {
	Ro = radius[i-1] + (radius[i] - radius[i-1]) * (Fo - flux[i-1]) / (flux[i] - flux[i-1]);
      }
      above = FALSE;
    }
  
    if (!above && (flux[i] >= Fo)) {
      above = TRUE;
    }
  }

  set_variable ("G_R50", Ro);
  return (TRUE);
}
