# include "data.h"

int cast (int argc, char **argv) {
  
  int  i, valid;
  Vector *ivec, *ovec;

  ivec = ovec = NULL;

  if (argc < 6) goto usage;

  valid = TRUE;
  valid &= !strcmp(argv[2], "=");
  valid &= !strcmp(argv[4], "as");
  if (!valid) goto usage;

  // out type 
  char outType = OPIHI_NOTYPE;
  if (!strcasecmp(argv[5], "int"))    outType = OPIHI_INT;
  if (!strcasecmp(argv[5], "float"))  outType = OPIHI_FLT;
  if (!strcasecmp(argv[5], "single")) outType = OPIHI_FLT;
  if (!strcasecmp(argv[5], "int64"))  outType = OPIHI_FLT;
  if (outType == OPIHI_NOTYPE) goto usage;

  int ForceSingle = FALSE;
  if (!strcasecmp(argv[5], "single")) ForceSingle = TRUE;

  int ForceI64 = FALSE;
  if (!strcasecmp(argv[5], "int64")) ForceI64 = TRUE;

  if ((ovec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) goto error;
  if ((ivec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto error;

  // ovec matches ivec in type
  ResetVector (ovec, outType, ivec[0].Nelements);

  // we have four cases: (ivec == flt or int) and (tvec == flt or int)
  if ((ivec->type == OPIHI_FLT) && (ovec->type == OPIHI_FLT)) {
    opihi_flt *vi = ivec[0].elements.Flt;
    opihi_flt *vo = ovec[0].elements.Flt;
    for (i = 0; i < ovec[0].Nelements; i++, vi++, vo++) {
      if (ForceSingle) {
	float tmp = *vi;
	*vo = tmp;
      } else if (ForceI64) {
	int64_t tmp = *vi;
	*vo = tmp;
      } else {
	*vo = *vi;
      }
    }
    return (TRUE);
  }
  if ((ivec->type == OPIHI_FLT) && (ovec->type == OPIHI_INT)) {
    opihi_flt *vi = ivec[0].elements.Flt;
    opihi_int *vo = ovec[0].elements.Int;
    for (i = 0; i < ovec[0].Nelements; i++, vi++, vo++) {
      *vo = *vi;
    }
    return (TRUE);
  }
  if ((ivec->type == OPIHI_INT) && (ovec->type == OPIHI_FLT)) {
    opihi_int *vi = ivec[0].elements.Int;
    opihi_flt *vo = ovec[0].elements.Flt;
    for (i = 0; i < ovec[0].Nelements; i++, vi++, vo++) {
      if (ForceSingle) {
	float tmp = *vi;
	*vo = tmp;
      } else {
	*vo = *vi;
      }
    }
    return (TRUE);
  }
  if ((ivec->type == OPIHI_INT) && (ovec->type == OPIHI_INT)) {
    opihi_int *vi = ivec[0].elements.Int;
    opihi_int *vo = ovec[0].elements.Int;
    for (i = 0; i < ovec[0].Nelements; i++, vi++, vo++) {
      *vo = *vi;
    }
    return (TRUE);
  }

error:
  DeleteVector (ovec);
  return (FALSE);

usage:
  gprint (GP_ERR, "SYNTAX: cast vec = vec as (int/float/single)\n");
  gprint (GP_ERR, "  note: (single) forces vector to have single precision\n");
  return (FALSE);
}

