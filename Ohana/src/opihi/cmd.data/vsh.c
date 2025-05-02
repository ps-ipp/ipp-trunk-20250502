# include "data.h"

int vsh (int argc, char **argv) {
  
  int i;
  Vector *Rbvec, *Revec, *Dbvec, *Devec;

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: vsh dRb dRe dDb dDe lmax R D\n");
    gprint (GP_ERR, "  set the dRb, dRe, dDb, dDe values for vsh terms up to lmax at R,D\n");
    return (FALSE);
  }

  if ((Rbvec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Revec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Dbvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Devec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  int lmax = atoi(argv[5]);

  VSHterms *terms = VSHtermsInit (lmax);

  double R = atof(argv[6]);
  double D = atof(argv[7]);

  ResetVector (Rbvec, OPIHI_FLT, terms->Nterms);
  ResetVector (Revec, OPIHI_FLT, terms->Nterms);
  ResetVector (Dbvec, OPIHI_FLT, terms->Nterms);
  ResetVector (Devec, OPIHI_FLT, terms->Nterms);

  VSHtermsForRD (terms, R, D); 

  for (i = 0; i < terms->Nterms; i++) {
    Rbvec[0].elements.Flt[i] = terms->dR_B[i];
    Revec[0].elements.Flt[i] = terms->dR_E[i];
    Dbvec[0].elements.Flt[i] = terms->dD_B[i];
    Devec[0].elements.Flt[i] = terms->dD_E[i];
  }

  VSHtermsFree (terms);

  return (TRUE);
}
