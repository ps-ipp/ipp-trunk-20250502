# include "data.h"

/* need to check dimensions of vectors */

int subset (int argc, char **argv) {
  
  char *out;
  int  i, Npts, size, valid;
  Vector *ivec, *ovec, *tvec;

  out = NULL;
  ivec = ovec = tvec = NULL;
  Npts = 0;

  if (argc < 6) {
    gprint (GP_ERR, "SYNTAX: subset vec = vec [if/where] (logic expression)\n");
    return (FALSE);
  }

  valid = TRUE;
  valid &= !strcmp(argv[2], "=");
  valid &= !strcmp(argv[4], "if") || !strcmp (argv[4], "where");
  if (!valid) {
    gprint (GP_ERR, "SYNTAX: subset vec = vec [if/where] (logic expression)\n");
    return (FALSE);
  }

  if ((ovec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) goto error;
  if ((ivec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto error;

  out = dvomath (argc - 5, &argv[5], &size, 1);
  if (out == NULL) {
    print_error ();
    goto error;
  }
  if ((tvec = SelectVector (out, OLDVECTOR, TRUE)) == NULL) goto error;
  if (ivec->Nelements != tvec->Nelements) {
    /* check size of ivec, tvec: must match */
    gprint (GP_ERR, "logical expression has different length from input vector\n");
    goto error;
  }

  // ovec matches ivec in type
  ResetVector (ovec, ivec->type, tvec[0].Nelements);

  // we have size cases: (ivec == flt, int, str) and (tvec == flt or int)
  if ((ivec->type == OPIHI_FLT) && (tvec->type == OPIHI_FLT)) {
    opihi_flt *vi = ivec[0].elements.Flt;
    opihi_flt *vt = tvec[0].elements.Flt;
    for (Npts = i = 0; i < tvec[0].Nelements; i++, vi++, vt++) {
      if (!*vt) continue;
      ovec[0].elements.Flt[Npts] = *vi;
      Npts++;
    }
  }
  if ((ivec->type == OPIHI_FLT) && (tvec->type == OPIHI_INT)) {
    opihi_flt *vi = ivec[0].elements.Flt;
    opihi_int *vt = tvec[0].elements.Int;
    for (Npts = i = 0; i < tvec[0].Nelements; i++, vi++, vt++) {
      if (!*vt) continue;
      ovec[0].elements.Flt[Npts] = *vi;
      Npts++;
    }
  }
  if ((ivec->type == OPIHI_INT) && (tvec->type == OPIHI_FLT)) {
    opihi_int *vi = ivec[0].elements.Int;
    opihi_flt *vt = tvec[0].elements.Flt;
    for (Npts = i = 0; i < tvec[0].Nelements; i++, vi++, vt++) {
      if (!*vt) continue;
      ovec[0].elements.Int[Npts] = *vi;
      Npts++;
    }
  }
  if ((ivec->type == OPIHI_INT) && (tvec->type == OPIHI_INT)) {
    opihi_int *vi = ivec[0].elements.Int;
    opihi_int *vt = tvec[0].elements.Int;
    for (Npts = i = 0; i < tvec[0].Nelements; i++, vi++, vt++) {
      if (!*vt) continue;
      ovec[0].elements.Int[Npts] = *vi;
      Npts++;
    }
  }
  // XXX if ivec is an existing vector, this step will
  // leak (existing elements need to be freed if they
  // have been allocated).
  if ((ivec->type == OPIHI_STR) && (tvec->type == OPIHI_FLT)) {
    opihi_flt *vt = tvec[0].elements.Flt;
    for (Npts = i = 0; i < tvec[0].Nelements; i++, vt++) {
      if (!*vt) continue;
      ovec[0].elements.Str[Npts] = strcreate(ivec[0].elements.Str[i]);
      Npts++;
    }
  }
  if ((ivec->type == OPIHI_STR) && (tvec->type == OPIHI_INT)) {
    opihi_int *vt = tvec[0].elements.Int;
    for (Npts = i = 0; i < tvec[0].Nelements; i++, vt++) {
      if (!*vt) continue;
      ovec[0].elements.Str[Npts] = strcreate(ivec[0].elements.Str[i]);
      Npts++;
    }
  }

  // free up unused memory
  ResetVector (ovec, ivec->type, Npts);

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

