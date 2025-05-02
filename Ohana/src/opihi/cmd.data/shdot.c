# include "data.h"

int shdot (int argc, char **argv) {
  
  int i, j;
  Vector *Rvec, *Dvec, *Fvec, *Lvec, *Mvec, *Frvec, *Fivec;
  double *Fr, *Fi;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: shdot R D value lmax Lout Mout Fr Fi\n");
    gprint (GP_ERR, "  find the dot product of the scalar field value (at points R,D) to spherical harmonics up to the given lmax\n");
    return (FALSE);
  }

  if ((Rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Dvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  int lmax = atoi(argv[4]);

  if ((Lvec  = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Mvec  = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Frvec = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fivec = SelectVector (argv[8], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  SHterms *terms = SHtermsInit (lmax);

  ALLOCATE_ZERO (Fr, double, terms->Nterms);
  ALLOCATE_ZERO (Fi, double, terms->Nterms);

  ResetVector (Frvec, OPIHI_FLT, terms->Nterms);
  ResetVector (Fivec, OPIHI_FLT, terms->Nterms);
  ResetVector (Lvec,  OPIHI_FLT, terms->Nterms);
  ResetVector (Mvec,  OPIHI_FLT, terms->Nterms);

  // measure the dot product \sum(F_i * Ylm_i)
  for (i = 0; i < Rvec->Nelements; i++) {

    SHtermsForRD (terms, Rvec->elements.Flt[i], Dvec->elements.Flt[i]); 

    double Fv = Fvec->elements.Flt[i];

    for (j = 0; j < terms->Nterms; j++) {
      Fr[j] += Fv * terms->Fr[j];
      Fi[j] += Fv * terms->Fi[j];
    }
  }

  // for (j = 0; j < terms->Nterms; j++) {
  //   fprintf (stderr, "%d : %d %d : %f %f\n", j, terms->l[j], terms->m[j], Fr[j], Fi[j]);
  // }

  for (j = 0; j < terms->Nterms; j++) {
    Fr[j] /= Fvec->Nelements;
    Fi[j] /= Fvec->Nelements;
  }

  for (j = 0; j < terms->Nterms; j++) {
    // fprintf (stderr, "%d : %d %d : %f %f\n", j, terms->l[j], terms->m[j], 4*M_PI*Fr[j], 4*M_PI*Fi[j]);
    Lvec[0].elements.Flt[j]  = terms->l[j];
    Mvec[0].elements.Flt[j]  = terms->m[j];
    Frvec[0].elements.Flt[j] = 4*M_PI*Fr[j];
    Fivec[0].elements.Flt[j] = 4*M_PI*Fi[j];
  }

  SHtermsFree (terms);

  return (TRUE);
}
