# include "data.h"

int cumulative (int argc, char **argv) {
  
  int i;
  opihi_flt *Vo;
  Vector *ivec, *ovec;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: cumulative invec outvec\n");
    return (FALSE);
  }

  if ((ivec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (ovec, OPIHI_FLT, ivec[0].Nelements);
  bzero (ovec[0].elements.Flt, sizeof(opihi_flt)*ovec[0].Nelements);

  Vo = ovec[0].elements.Flt;

  if (ivec[0].type == OPIHI_FLT) {
    opihi_flt *Vi = ivec[0].elements.Flt;
    *Vo = *Vi;
    Vi++; 
    Vo++;
    for (i = 1; i < ivec[0].Nelements; i++, Vi++, Vo++) {
      *Vo = Vo[-1] + *Vi;
    }      
  } else {
    opihi_int *Vi = ivec[0].elements.Int;
    *Vo = *Vi;
    Vi++;
    Vo++;
    for (i = 1; i < ivec[0].Nelements; i++, Vi++, Vo++) {
      *Vo = Vo[-1] + *Vi;
    }      
  }
  return (TRUE);
}
