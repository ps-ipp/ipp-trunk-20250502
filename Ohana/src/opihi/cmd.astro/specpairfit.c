# include "astro.h"

int specpairfit (int argc, char **argv) {
  
  int i;
  Vector *flux1, *flux2, *dflux1, *dflux2, *window;

  if (argc != 7) goto usage;

  if ((flux1  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((dflux1 = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((flux2  = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((dflux2 = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((window = SelectVector (argv[5], OLDVECTOR, TRUE)) == NULL) goto escape;
  
  // XXX enforce matching lengths on the 6 vectors

  int mask = atoi (argv[6]);
  CastVector (window, OPIHI_INT);

  // minimize (flux2 - flux1*A - B) in window defined by mask
  // note that the mask is a SELECTION mask not an EXCLUSION mask

  double R = 0.0, F1 = 0.0, F11 = 0.0, F12 = 0.0, F2 = 0.0;
  for (i = 0; i < flux1->Nelements; i++) {
    // if ((mask & window->elements.Int[i]) == 0) continue;
    double weight = 1.0 / (SQ(dflux1->elements.Flt[i]) + SQ(dflux2->elements.Flt[i]));
    R   += weight;
    F1  += flux1->elements.Flt[i] * weight;
    F2  += flux2->elements.Flt[i] * weight;
    F11 += flux1->elements.Flt[i] * flux1->elements.Flt[i] * weight;
    F12 += flux1->elements.Flt[i] * flux2->elements.Flt[i] * weight;
  }

  int nterm = 2;
  double **b = NULL, **c = NULL;
  ALLOCATE (b, double *, nterm);
  ALLOCATE (c, double *, nterm);
  for (i = 0; i < nterm; i++) {
    ALLOCATE (c[i], double, nterm);
    ALLOCATE (b[i], double, 1);
  }
  c[0][0] = F11;
  c[1][0] = c[0][1] = F1;
  c[1][1] = R;

  b[0][0] = F12;
  b[1][0] = F2;

  if (!dgaussjordan (c, b, nterm, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    goto escape;
  }

  double Ao = b[0][0];
  double dA = sqrt(c[0][0]);
  double Bo = b[1][0];
  double dB = sqrt(c[1][1]);

  for (i = 0; i < nterm; i++) {
    free (b[i]);
    free (c[i]);
  }
  free (b);
  free (c);

  int Ndof = -2; // 2 parameter fit
  double chisq = 0.0;
  for (i = 0; i < flux1->Nelements; i++) {
    if ((mask & window->elements.Int[i]) == 0) continue;
    double weight = 1.0 / (SQ(dflux1->elements.Flt[i]) + SQ(dflux2->elements.Flt[i]));
    chisq += SQ(flux1->elements.Flt[i] - Ao * flux2->elements.Flt[i] - Bo) * weight;
    Ndof ++;
  }

  double chisqNu = chisq / Ndof;

  // fprintf (stderr, "R: %f, F1: %f, F11: %f, F2: %f, F12: %f\n", R, F1, F11, F2, F12);
  // fprintf (stderr, "Ao: %f +/- %f, chisq: %f, chisq_nu : %f for %d dof\n", Ao, dA, chisq, chisqNu, Ndof);
  set_variable ("Ao", Ao);
  set_variable ("dA", dA);
  set_variable ("Bo", Bo);
  set_variable ("dB", dB);
  set_variable ("Xv", chisqNu);
  set_variable ("Nd", Ndof);
  return (TRUE);
  
 escape: 
  gprint (GP_ERR, "invalid vector\n");
  return (FALSE);
  
usage:
  gprint (GP_ERR, "USAGE: specpairfit (flux1) (dflux1) (flux2) (dflux2) (window) (mask) [options]\n");
  return FALSE;
}
