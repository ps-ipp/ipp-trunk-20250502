# include "data.h"

int vshfit (int argc, char **argv) {
  
  int i, j;
  Vector *Rvec, *Dvec, *dRvec, *dDvec;
  double *Re, *Rb, *De, *Db;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: vshfit Rvec Dvec dRvec dDvec lmax\n");
    gprint (GP_ERR, "  fit the vector field dRvec, dDvec (at points Rvec,Dvec) to vector spherical harmonics up to the given lmax\n");
    return (FALSE);
  }

  if ((Rvec  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Dvec  = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dRvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dDvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  int lmax = atoi(argv[5]);

  VSHterms *terms = VSHtermsInit (lmax);

  ALLOCATE_ZERO (Re, double, terms->Nterms);
  ALLOCATE_ZERO (Rb, double, terms->Nterms);
  ALLOCATE_ZERO (De, double, terms->Nterms);
  ALLOCATE_ZERO (Db, double, terms->Nterms);

  for (i = 0; i < Rvec->Nelements; i++) {

    VSHtermsForRD (terms, Rvec->elements.Flt[i], Dvec->elements.Flt[i]); 

    double dR = dRvec->elements.Flt[i];
    double dD = dDvec->elements.Flt[i];

    for (j = 0; j < terms->Nterms; j++) {
      Rb[j] += dR * terms->dR_B[j];
      Re[j] += dR * terms->dR_E[j];
      Db[j] += dD * terms->dD_B[j];
      De[j] += dD * terms->dD_E[j];
    }
  }

  for (j = 0; j < terms->Nterms; j++) {
    fprintf (stderr, "%d : %d %d : %f %f %f %f\n", j, terms->l[j], terms->m[j], Rb[j], Re[j], Db[j], De[j]);
  }

  for (j = 0; j < terms->Nterms; j++) {
    Rb[j] /= terms->Nterms;
    Re[j] /= terms->Nterms;
    Db[j] /= terms->Nterms;
    De[j] /= terms->Nterms;
  }

  for (j = 0; j < terms->Nterms; j++) {
    fprintf (stderr, "%d : %d %d : %f %f %f %f\n", j, terms->l[j], terms->m[j], Rb[j], Re[j], Db[j], De[j]);
  }

  VSHtermsFree (terms);

  return (TRUE);
}
