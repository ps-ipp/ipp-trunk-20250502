# include "data.h"

int shfit (int argc, char **argv) {
  
  int i, j, k;
  Vector *Rvec, *Dvec, *Fvec, *Lvec, *Mvec, *Frvec, *Fivec;
  double *Fr, *Fi;

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: shfit R D value lmax Lout Mout Fr Fi\n");
    gprint (GP_ERR, "  fit the scalar field value (at points R,D) to spherical harmonics up to the given lmax\n");
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

  // we only fit the linearlly independent terms: Re(m >= 0), Im(m > 0)
  int Nre = 0;
  int Nim = 0;
  for (i = 0; i < terms->Nterms; i++) {
    if (terms->m[i] >= 0) Nre ++;
    if (terms->m[i] >  0) Nim ++;
  }
  int *Jre = NULL;
  int *Jim = NULL;
  ALLOCATE (Jre, int, Nre);
  ALLOCATE (Jim, int, Nim);

  Nre = Nim = 0;
  for (i = 0; i < terms->Nterms; i++) {
    if (terms->m[i] >= 0) {
      Jre[Nre] = i;
      Nre ++;
    }
    if (terms->m[i] >  0) {
      Jim[Nim] = i;
      Nim ++;
    }
  }

  double **Are, **bre, **Aim, **bim;
  ALLOCATE (Are, double *, Nre);
  ALLOCATE (bre, double *, Nre);
  ALLOCATE (Aim, double *, Nim);
  ALLOCATE (bim, double *, Nim);
  for (i = 0; i < Nre; i++) {
    ALLOCATE_ZERO (Are[i], double, Nre);
    ALLOCATE_ZERO (bre[i], double, 1);
  }
  for (i = 0; i < Nim; i++) {
    ALLOCATE_ZERO (Aim[i], double, Nim);
    ALLOCATE_ZERO (bim[i], double, 1);
  }

  // measure the dot product \sum(F_i * Ylm_i) and the cross terms (\sum(Y_lm * Y_jk))
  for (i = 0; i < Rvec->Nelements; i++) {

    SHtermsForRD (terms, Rvec->elements.Flt[i], Dvec->elements.Flt[i]); 

    double Fv = Fvec->elements.Flt[i];

    for (j = 0; j < Nre; j++) {
      int jre = Jre[j];
      bre[j][0] += Fv * terms->Fr[jre];
    }
    for (j = 0; j < Nim; j++) {
      int jim = Jim[j];
      bim[j][0] += Fv * terms->Fi[jim];
    }

    for (j = 0; j < Nre; j++) {
      int jre = Jre[j];
      for (k = j; k < Nre; k++) {
	int kre = Jre[k];
	Are[j][k] += terms->Fr[jre] * terms->Fr[kre];
      }
    }

    for (j = 0; j < Nim; j++) {
      int jim = Jim[j];
      for (k = j; k < Nim; k++) {
	int kim = Jim[k];
	Aim[j][k] += terms->Fi[jim] * terms->Fi[kim];
      }
    }
  }

  for (j = 1; j < Nre; j++) {
    for (k = 0; k < j; k++) {
      Are[j][k] = Are[k][j];
    }	
  }
  for (j = 1; j < Nim; j++) {
    for (k = 0; k < j; k++) {
      Aim[j][k] = Aim[k][j];
    }	
  }

  fprintf (stderr, "--- Are --- : bre \n");
  for (j = 0; j < Nre; j++) {
    fprintf (stderr, "%10.6f ", Are[j][j]);
    fprintf (stderr, " : %10.6f\n", bre[j][0]);
  }

  fprintf (stderr, "--- Aim --- : bim \n");
  for (i = 0; i < Nim; i++) {
    fprintf (stderr, "%10.6f : ", Aim[i][i]);
    fprintf (stderr, " : %10.6f\n", bim[i][0]);
  }

  if (!dgaussjordan (Are, bre, Nre, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }
  if (!dgaussjordan (Aim, bim, Nim, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }

  for (j = 0; j < terms->Nterms; j++) { 
    Fr[j] = 0.0;
    Fi[i] = 0.0;
  }
  for (j = 0; j < Nre; j++) {
    int jre = Jre[j];
    Fr[jre] = bre[j][0];
  }
  for (j = 0; j < Nim; j++) {
    int jim = Jim[j];
    Fi[jim] = bim[j][0];
  }

  for (j = 0; j < terms->Nterms; j++) {
    // fprintf (stderr, "%d : %d %d : %f %f\n", j, terms->l[j], terms->m[j], 4*M_PI*Fr[j], 4*M_PI*Fi[j]);
    Lvec[0].elements.Flt[j]  = terms->l[j];
    Mvec[0].elements.Flt[j]  = terms->m[j];
    Frvec[0].elements.Flt[j] = Fr[j];
    Fivec[0].elements.Flt[j] = Fi[j];
  }

  SHtermsFree (terms);

  return (TRUE);
}
