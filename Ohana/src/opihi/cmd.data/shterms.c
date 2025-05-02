# include "data.h"

int shterms (int argc, char **argv) {
  
  int i;
  Vector *Frvec, *Fivec, *lvec, *mvec;

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: shterms Fr Fi l m lmax R D\n");
    gprint (GP_ERR, "  set the Fr and Fi values for sh terms up to lmax at R,D\n");
    return (FALSE);
  }

  if ((Frvec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fivec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((lvec  = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((mvec  = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  int lmax = atoi(argv[5]);

  SHterms *terms = SHtermsInit (lmax);

  double R = atof(argv[6]);
  double D = atof(argv[7]);

  ResetVector (Frvec, OPIHI_FLT, terms->Nterms);
  ResetVector (Fivec, OPIHI_FLT, terms->Nterms);
  ResetVector (lvec,  OPIHI_FLT, terms->Nterms);
  ResetVector (mvec,  OPIHI_FLT, terms->Nterms);

  SHtermsForRD (terms, R, D); 

  for (i = 0; i < terms->Nterms; i++) {
    Frvec[0].elements.Flt[i] = terms->Fr[i];
    Fivec[0].elements.Flt[i] = terms->Fi[i];
    lvec[0].elements.Flt[i]  = terms->l[i];
    mvec[0].elements.Flt[i]  = terms->m[i];
  }

  SHtermsFree (terms);

  return (TRUE);
}
