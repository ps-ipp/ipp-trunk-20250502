# include "relphot.h"
# include "lensing.h"

Lensctr *dvo_lensctr_init (int Nsec) {

  ALLOCATE_PTR (lensctr, Lensctr, Nsec);
  return (lensctr);
}

int dvo_lensctr_reset (Lensctr *lensctr, int Nsec) {

  for (int i = 0; i < Nsec; i++) {
    memset (&lensctr[i], 0, sizeof (Lensctr));
  }
  return TRUE;
}

int dvo_lensing_accum (Lensobj *lensobj, Lensctr *lensctr, Lensing *lensing, float Fcal) {

  int hasLensing = FALSE;
  
  if (isfinite(lensing-> F_ApR5)) {
    lensobj-> F_ApR5 +=    Fcal * lensing-> F_ApR5;
    lensobj->dF_ApR5 += SQ(Fcal * lensing->dF_ApR5);
    lensobj->sF_ApR5 += SQ(Fcal * lensing-> F_ApR5);
    lensobj->fF_ApR5 +=           lensing->fF_ApR5;
    lensctr->N5_C0 ++;
    hasLensing = TRUE;
  }
  
  if (isfinite(lensing-> F_ApR6)) {
    lensobj-> F_ApR6 +=    Fcal * lensing-> F_ApR6;
    lensobj->dF_ApR6 += SQ(Fcal * lensing->dF_ApR6);
    lensobj->sF_ApR6 += SQ(Fcal * lensing-> F_ApR6);
    lensobj->fF_ApR6 +=           lensing->fF_ApR6;
    lensctr->N6_C0 ++;
    hasLensing = TRUE;
  }

  if (isfinite(lensing-> F_ApR7)) {
    lensobj-> F_ApR7 +=    Fcal * lensing-> F_ApR7;
    lensobj->dF_ApR7 += SQ(Fcal * lensing->dF_ApR7);
    lensobj->sF_ApR7 += SQ(Fcal * lensing-> F_ApR7);
    lensobj->fF_ApR7 +=           lensing->fF_ApR7;
    lensctr->N7_C0 ++;
    hasLensing = TRUE;
  }

  if (isfinite(lensing-> F_ApR5_C1)) {
    lensobj-> F_ApR5_C1 +=    Fcal * lensing-> F_ApR5_C1;
    lensobj->dF_ApR5_C1 += SQ(Fcal * lensing->dF_ApR5_C1);
    lensctr->N5_C1 ++;
    hasLensing = TRUE;
  }
  if (isfinite(lensing-> F_ApR6_C1)) {
    lensobj-> F_ApR6_C1 +=    Fcal * lensing-> F_ApR6_C1;
    lensobj->dF_ApR6_C1 += SQ(Fcal * lensing->dF_ApR6_C1);
    lensctr->N6_C1 ++;
    hasLensing = TRUE;
  }
  if (isfinite(lensing-> F_ApR7_C1)) {
    lensobj-> F_ApR7_C1 +=    Fcal * lensing-> F_ApR7_C1;
    lensobj->dF_ApR7_C1 += SQ(Fcal * lensing->dF_ApR7_C1);
    lensctr->N7_C1 ++;
    hasLensing = TRUE;
  }

  if (isfinite(lensing-> F_ApR5_C2)) {
    lensobj-> F_ApR5_C2 +=    Fcal * lensing-> F_ApR5_C2;
    lensobj->dF_ApR5_C2 += SQ(Fcal * lensing->dF_ApR5_C2);
    lensctr->N5_C2 ++;
    hasLensing = TRUE;
  }
  if (isfinite(lensing-> F_ApR6_C2)) {
    lensobj-> F_ApR6_C2 +=    Fcal * lensing-> F_ApR6_C2;
    lensobj->dF_ApR6_C2 += SQ(Fcal * lensing->dF_ApR6_C2);
    lensctr->N6_C2 ++;
    hasLensing = TRUE;
  }
  if (isfinite(lensing-> F_ApR7_C2)) {
    lensobj-> F_ApR7_C2 +=    Fcal * lensing-> F_ApR7_C2;
    lensobj->dF_ApR7_C2 += SQ(Fcal * lensing->dF_ApR7_C2);
    lensctr->N7_C2 ++;
    hasLensing = TRUE;
  }
  if (hasLensing) { lensctr->Nmeas ++; } 
  return TRUE;
}

// return TRUE if any counter is non-zero:
int dvo_lensctr_has_values (Lensctr *lensctr) {
  if (lensctr->N5_C0) return TRUE;
  if (lensctr->N6_C0) return TRUE;
  if (lensctr->N7_C0) return TRUE;

  if (lensctr->N5_C1) return TRUE;
  if (lensctr->N6_C1) return TRUE;
  if (lensctr->N7_C1) return TRUE;

  if (lensctr->N5_C2) return TRUE;
  if (lensctr->N6_C2) return TRUE;
  if (lensctr->N7_C2) return TRUE;

  return FALSE;
}

int dvo_lensobj_stat (float *mean, float *error, float *stdev, float *fill, int count) {

  if (count) {
    *mean  /= (float) count;
    *error  = sqrt(*error / (float) count);
			     
    if (fill) { *fill  /= (float) count; }

    if (stdev) {
      if (count < 2) {
	*stdev = NAN;
      } else {
	double S1 = SQ(*mean); // <f>^2
	double S2 =    *stdev / (float) count; // sum(f^2) / N
	*stdev    = sqrt(S2 - S1) * (count / (count - 1.0)); // correct to sample stdev
      }
    }
  } else {
    *mean  = NAN;
    *error = NAN;
    if (stdev) { *stdev = NAN; }
    if (fill)  { *fill  = NAN; }
  }
  return TRUE;
}

int dvo_lensobj_aves (Lensobj *lensobj, Lensctr *lensctr) {

  dvo_lensobj_stat (&lensobj-> F_ApR5,    &lensobj->dF_ApR5,    &lensobj->sF_ApR5,    &lensobj->fF_ApR5,    lensctr->N5_C0);
  dvo_lensobj_stat (&lensobj-> F_ApR6,    &lensobj->dF_ApR6,    &lensobj->sF_ApR6,    &lensobj->fF_ApR6,    lensctr->N6_C0);
  dvo_lensobj_stat (&lensobj-> F_ApR7,    &lensobj->dF_ApR7,    &lensobj->sF_ApR7,    &lensobj->fF_ApR7,    lensctr->N7_C0);

  dvo_lensobj_stat (&lensobj-> F_ApR5_C1, &lensobj->dF_ApR5_C1, NULL,                 NULL,                 lensctr->N5_C1);
  dvo_lensobj_stat (&lensobj-> F_ApR6_C1, &lensobj->dF_ApR6_C1, NULL,                 NULL,                 lensctr->N6_C1);
  dvo_lensobj_stat (&lensobj-> F_ApR7_C1, &lensobj->dF_ApR7_C1, NULL,                 NULL,                 lensctr->N7_C1);

  dvo_lensobj_stat (&lensobj-> F_ApR5_C2, &lensobj->dF_ApR5_C2, NULL,                 NULL,                 lensctr->N5_C2);
  dvo_lensobj_stat (&lensobj-> F_ApR6_C2, &lensobj->dF_ApR6_C2, NULL,                 NULL,                 lensctr->N6_C2);
  dvo_lensobj_stat (&lensobj-> F_ApR7_C2, &lensobj->dF_ApR7_C2, NULL,                 NULL,                 lensctr->N7_C2);

  lensobj->Nmeas = lensctr->Nmeas;
      
  return TRUE;
}

