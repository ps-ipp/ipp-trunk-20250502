# include "data.h"

int concat (int argc, char **argv) {

  int  i, j, Nin;
  double value;
  opihi_flt *temp;
  Vector *ivec, *ovec;

  /** check basic syntax **/
  if (argc != 3) {
    gprint (GP_ERR, "SYNTAX: concat (vector/value) vector\n");
    gprint (GP_ERR, "  concatanate (vector/value) to vector\n");
    return (FALSE);
  }

  if ((ovec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if (SelectScalar (argv[1], &value)) {
    Nin = ovec[0].Nelements;
    ovec[0].Nelements++;
    if (ovec[0].type == OPIHI_FLT) {
      REALLOCATE (ovec[0].elements.Flt, opihi_flt, ovec[0].Nelements);
      ovec[0].elements.Flt[Nin] = value;
    } else {
      REALLOCATE (ovec[0].elements.Int, opihi_int, ovec[0].Nelements);
      ovec[0].elements.Int[Nin] = value;
    }
    return (TRUE);
  } 
  
  if ((ivec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  
  // if the output vector is empty, inherit the type of the input vector
  if (!ovec->Nelements) { ResetVector (ovec, ivec->type, ovec->Nelements); }

  // we have 3 input and 3 output types : INT, FLT, STR

  // both are strings
  if ((ovec[0].type == OPIHI_STR) && (ivec[0].type == OPIHI_STR)) {
    REALLOCATE (ovec->elements.Str, char *, MAX(1, ovec->Nelements + ivec->Nelements));
    for (j = ovec[0].Nelements, i = 0; i < ivec[0].Nelements; i++, j++) {
      ovec[0].elements.Str[j] = strcreate(ivec[0].elements.Str[i]);
    }
    goto done;
  }

  // if only one is a string, raise an error
  if ((ovec[0].type == OPIHI_STR) || (ivec[0].type == OPIHI_STR)) {
    gprint (GP_ERR, "ERROR: cannot mix string and numerical vectors\n");
    return FALSE;
  }

  // both vectors are floats
  if ((ovec[0].type == OPIHI_FLT) && (ivec[0].type == OPIHI_FLT)) {
    REALLOCATE (ovec[0].elements.Flt, opihi_flt, ovec[0].Nelements + ivec[0].Nelements);
    for (j = ovec[0].Nelements, i = 0; i < ivec[0].Nelements; i++, j++) {
      ovec[0].elements.Flt[j] = ivec[0].elements.Flt[i];
    }
    goto done;
  }
  // both vectors are ints
  if ((ovec[0].type == OPIHI_INT) && (ivec[0].type == OPIHI_INT)) {
    REALLOCATE (ovec[0].elements.Int, opihi_int, ovec[0].Nelements + ivec[0].Nelements);
    for (j = ovec[0].Nelements, i = 0; i < ivec[0].Nelements; i++, j++) {
      ovec[0].elements.Int[j] = ivec[0].elements.Int[i];
    }
    goto done;
  }

  // if output vector is a float and input is an int, cast the int values to the float
  if ((ovec[0].type == OPIHI_FLT) && (ivec[0].type == OPIHI_INT)) {
    REALLOCATE (ovec[0].elements.Flt, opihi_flt, ovec[0].Nelements + ivec[0].Nelements);
    for (j = ovec[0].Nelements, i = 0; i < ivec[0].Nelements; i++, j++) {
      ovec[0].elements.Flt[j] = ivec[0].elements.Int[i];
    }
    goto done;
  }

  // the output vector is an int and input is a float:
  // this case forces ovec to be raised to FLT
  if ((ovec[0].type == OPIHI_INT) && (ivec[0].type == OPIHI_FLT)) {
    ALLOCATE (temp, opihi_flt, ovec[0].Nelements + ivec[0].Nelements);
    for (i = 0; i < ovec[0].Nelements; i++) {
      temp[i] = ovec[0].elements.Int[i];
    }
    ovec[0].type = OPIHI_FLT;
    free (ovec[0].elements.Int);
    ovec[0].elements.Flt = temp;
    for (j = ovec[0].Nelements, i = 0; i < ivec[0].Nelements; i++, j++) {
      ovec[0].elements.Flt[j] = ivec[0].elements.Flt[i];
    }
    goto done;
  }

done:
  ovec[0].Nelements += ivec[0].Nelements;
  return (TRUE);
}
