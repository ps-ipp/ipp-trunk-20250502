# include "astro.h"

int parallax_factor (int argc, char **argv) {

  Vector *rvec = NULL;
  Vector *dvec = NULL;
  Vector *tvec = NULL;
  Vector *pRvec = NULL;
  Vector *pDvec = NULL;

  if ((argc != 4) && (argc != 7)) {
    gprint (GP_ERR, "USAGE:  parallax_factor (ra) (dec) (time) [to (parR) (parD)]\n");
    return (FALSE);
  }
  if ((argc == 7) && strcasecmp (argv[4], "to")) {
    gprint (GP_ERR, "USAGE:  parallax_factor (ra) (dec) (time) to (parR) (parD)\n");
    return (FALSE);
  }

  double time, ra, dec;

  if (SelectScalar (argv[1], &ra)) {
    if (!SelectScalar (argv[2], &dec)) goto escape;
    if (!SelectScalar (argv[3], &time)) goto escape;

    double parR, parD;

    ParFactor (&parR, &parD, ra, dec, time);

    if (argc == 4) {
      gprint (GP_LOG, "%10.6f %10.6f\n", parR, parD);
    } else {
      set_variable (argv[5], parR);
      set_variable (argv[6], parD);
    }

    return TRUE;
  }

  if (argc == 4) { 
    gprint (GP_ERR, "must supply output vectors for input vectors\n");
    return (FALSE);
  }
  if ((argc == 7) && strcasecmp (argv[4], "to")) {
    gprint (GP_ERR, "USAGE:  parallax_factor (ra) (dec) (time) to (parR) (parD)\n");
    return (FALSE);
  }

  if ((rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((dvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((tvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;

  if (rvec[0].Nelements != dvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (rvec[0].Nelements != tvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[3]);
    return (FALSE);
  }
  
  if (rvec->type != OPIHI_FLT) {
    gprint (GP_ERR, "input vectors must be floating point\n");
    return (FALSE);
  }
  if (dvec->type != OPIHI_FLT) {
    gprint (GP_ERR, "input vectors must be floating point\n");
    return (FALSE);
  }
  if (tvec->type != OPIHI_FLT) {
    gprint (GP_ERR, "input vectors must be floating point\n");
    return (FALSE);
  }

  if ((pRvec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((pDvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) goto escape;

  ResetVector (pRvec, OPIHI_FLT, rvec[0].Nelements);
  ResetVector (pDvec, OPIHI_FLT, rvec[0].Nelements);

  opihi_flt *Rv = rvec[0].elements.Flt;
  opihi_flt *Dv = dvec[0].elements.Flt;
  opihi_flt *Tv = tvec[0].elements.Flt;
  opihi_flt *pRv = pRvec[0].elements.Flt;
  opihi_flt *pDv = pDvec[0].elements.Flt;

  for (int i = 0; i < rvec[0].Nelements; i++, Rv++, Dv++, Tv++, pRv++, pDv++) {
    ParFactor (pRv, pDv, *Rv, *Dv, *Tv);
  }

  return (TRUE);

escape:
  FREE (rvec);
  FREE (dvec);
  FREE (tvec);
  FREE (pRvec);
  FREE (pDvec);
  return FALSE;
}  

int parallax_factor_3d (int argc, char **argv) {

  Vector *rvec = NULL;
  Vector *dvec = NULL;
  Vector *tvec = NULL;
  Vector *pRvec = NULL;
  Vector *pDvec = NULL;
  Vector *pZvec = NULL;

  if ((argc != 4) && (argc != 8)) {
    gprint (GP_ERR, "USAGE:  parallax_factor (ra) (dec) (time) [to (parR) (parD) (parZ)]\n");
    return (FALSE);
  }
  if ((argc == 8) && strcasecmp (argv[4], "to")) {
    gprint (GP_ERR, "USAGE:  parallax_factor (ra) (dec) (time) to (parR) (parD) (parZ)\n");
    return (FALSE);
  }

  double time, ra, dec;

  if (SelectScalar (argv[1], &ra)) {
    if (!SelectScalar (argv[2], &dec)) goto escape;
    if (!SelectScalar (argv[3], &time)) goto escape;

    double parR, parD, parZ;

    ParFactor_3d (&parR, &parD, &parZ, ra, dec, time);

    if (argc == 4) {
      gprint (GP_LOG, "%10.6f %10.6f %10.6f\n", parR, parD, parZ);
    } else {
      set_variable (argv[5], parR);
      set_variable (argv[6], parD);
      set_variable (argv[7], parZ);
    }

    return TRUE;
  }

  if (argc == 4) { 
    gprint (GP_ERR, "must supply output vectors for input vectors\n");
    return (FALSE);
  }
  if ((argc == 8) && strcasecmp (argv[4], "to")) {
    gprint (GP_ERR, "USAGE:  parallax_factor (ra) (dec) (time) to (parR) (parD) (parZ)\n");
    return (FALSE);
  }

  if ((rvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((dvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((tvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;

  if (rvec[0].Nelements != dvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (rvec[0].Nelements != tvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[3]);
    return (FALSE);
  }
  
  if (rvec->type != OPIHI_FLT) {
    gprint (GP_ERR, "input vectors must be floating point\n");
    return (FALSE);
  }
  if (dvec->type != OPIHI_FLT) {
    gprint (GP_ERR, "input vectors must be floating point\n");
    return (FALSE);
  }
  if (tvec->type != OPIHI_FLT) {
    gprint (GP_ERR, "input vectors must be floating point\n");
    return (FALSE);
  }

  if ((pRvec = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((pDvec = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) goto escape;
  if ((pZvec = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) goto escape;

  ResetVector (pRvec, OPIHI_FLT, rvec[0].Nelements);
  ResetVector (pDvec, OPIHI_FLT, rvec[0].Nelements);
  ResetVector (pZvec, OPIHI_FLT, rvec[0].Nelements);

  opihi_flt *Rv = rvec[0].elements.Flt;
  opihi_flt *Dv = dvec[0].elements.Flt;
  opihi_flt *Tv = tvec[0].elements.Flt;
  opihi_flt *pRv = pRvec[0].elements.Flt;
  opihi_flt *pDv = pDvec[0].elements.Flt;
  opihi_flt *pZv = pZvec[0].elements.Flt;

  for (int i = 0; i < rvec[0].Nelements; i++, Rv++, Dv++, Tv++, pRv++, pDv++, pZv++) {
    ParFactor_3d (pRv, pDv, pZv, *Rv, *Dv, *Tv);
  }

  return (TRUE);

escape:
  FREE (rvec);
  FREE (dvec);
  FREE (tvec);
  FREE (pRvec);
  FREE (pDvec);
  FREE (pZvec);
  return FALSE;
}  
