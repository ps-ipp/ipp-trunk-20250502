# include "data.h"

int shapply (int argc, char **argv) {
  
  int i, j;
  Vector *Rvec, *Dvec, *Frvec, *Fivec, *Vrvec, *Vivec;

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: shapply R D lmax Fr Fi Vr Vi\n");
    gprint (GP_ERR, "  apply the Ylm values (Fr,Fi) at the given R,D values to get the fit at those positions\n");
    return (FALSE);
  }

  if ((Rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Dvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (Rvec->Nelements != Dvec->Nelements) {
    gprint (GP_ERR, "R and D sizes do not match\n");
    return FALSE;
  }

  int lmax = atoi(argv[3]);

  if ((Frvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fivec = SelectVector (argv[5], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (Frvec->Nelements != Fivec->Nelements) { 
    gprint (GP_ERR, "Fr and Fi sizes do not match\n");
    return FALSE;
  }

  if ((Vrvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Vivec = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  SHterms *terms = SHtermsInit (lmax);
  if (Frvec->Nelements != terms->Nterms) {
    gprint (GP_ERR, "Lmax (%d) terms (%d) does not match size of Fr,Fi vectors (%d) \n", lmax, terms->Nterms, Frvec->Nelements);
    return FALSE;
  }

  ResetVector (Vrvec, OPIHI_FLT, Rvec->Nelements);
  ResetVector (Vivec, OPIHI_FLT, Rvec->Nelements);

  // measure the dot product \sum(F_i * Ylm_i)
  for (i = 0; i < Rvec->Nelements; i++) {

    SHtermsForRD (terms, Rvec->elements.Flt[i], Dvec->elements.Flt[i]); 

    double *Fr = Frvec->elements.Flt;
    double *Fi = Fivec->elements.Flt;

    double Vr = 0.0;
    double Vi = 0.0;
    for (j = 0; j < terms->Nterms; j++) {
      Vr += Fr[j] * terms->Fr[j];
      Vi += Fi[j] * terms->Fi[j];
    }
    Vrvec->elements.Flt[i] = Vr;
    Vivec->elements.Flt[i] = Vi;
  }

  SHtermsFree (terms);

  return (TRUE);
}
