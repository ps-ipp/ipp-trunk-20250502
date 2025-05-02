# include "data.h"

int vect_select (int argc, char **argv) {
  
  int  i, size;
  char *out;
  Vector *in1, *in2, *tvec, *ovec;

  out = NULL;
  in1 = in2 = ovec = tvec = NULL;

  /** check basic syntax **/
  if ((argc != 8) || strcmp(argv[2], "=") || strcmp (argv[4], "if") || strcmp (argv[6], "else")) {
    gprint (GP_ERR, "SYNTAX: select vec = vec if (logic expression) else vec\n");
    return (FALSE);
  }
  if ((in1  = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((in2  = SelectVector (argv[7], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  out = dvomath (argc - 5, &argv[5], &size, 1);
  if (out == NULL) {
    print_error ();
    goto error;
  }
  if ((tvec = SelectVector (out, OLDVECTOR, TRUE)) == NULL) goto error;
  /* check size of in1, in2, tvec: must match */

  if ((in1->type == OPIHI_INT) && (in2->type == OPIHI_INT)) {
    ResetVector (ovec, OPIHI_INT, tvec[0].Nelements);
  } else {
    ResetVector (ovec, OPIHI_FLT, tvec[0].Nelements);
  }

  // (in1 and in2) == (flt or int) and tvec == (flt or int)
  if ((in1->type == OPIHI_FLT) && (in2->type == OPIHI_FLT) && (tvec->type == OPIHI_FLT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Flt[i] = tvec[0].elements.Flt[i] ? in1[0].elements.Flt[i] : in2[0].elements.Flt[i];
    }
  }
  if ((in1->type == OPIHI_FLT) && (in2->type == OPIHI_FLT) && (tvec->type == OPIHI_INT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Flt[i] = tvec[0].elements.Int[i] ? in1[0].elements.Flt[i] : in2[0].elements.Flt[i];
    }
  }
  if ((in1->type == OPIHI_INT) && (in2->type == OPIHI_FLT) && (tvec->type == OPIHI_FLT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Flt[i] = tvec[0].elements.Flt[i] ? in1[0].elements.Int[i] : in2[0].elements.Flt[i];
    }
  }
  if ((in1->type == OPIHI_INT) && (in2->type == OPIHI_FLT) && (tvec->type == OPIHI_INT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Flt[i] = tvec[0].elements.Int[i] ? in1[0].elements.Int[i] : in2[0].elements.Flt[i];
    }
  }
  if ((in1->type == OPIHI_FLT) && (in2->type == OPIHI_INT) && (tvec->type == OPIHI_FLT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Flt[i] = tvec[0].elements.Flt[i] ? in1[0].elements.Flt[i] : in2[0].elements.Int[i];
    }
  }
  if ((in1->type == OPIHI_FLT) && (in2->type == OPIHI_INT) && (tvec->type == OPIHI_INT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Flt[i] = tvec[0].elements.Int[i] ? in1[0].elements.Flt[i] : in2[0].elements.Int[i];
    }
  }
  if ((in1->type == OPIHI_INT) && (in2->type == OPIHI_INT) && (tvec->type == OPIHI_FLT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Int[i] = tvec[0].elements.Flt[i] ? in1[0].elements.Int[i] : in2[0].elements.Int[i];
    }
  }
  if ((in1->type == OPIHI_INT) && (in2->type == OPIHI_INT) && (tvec->type == OPIHI_INT)) {
    for (i = 0; i < tvec[0].Nelements; i++) {
      ovec[0].elements.Int[i] = tvec[0].elements.Int[i] ? in1[0].elements.Int[i] : in2[0].elements.Int[i];
    }
  }
  
  DeleteVector (tvec);
  free (out);
  return (TRUE);

 error:
  DeleteVector (tvec);
  DeleteVector (ovec);
  DeleteNamedVector (out);
  free (out);
  return (FALSE);
}
