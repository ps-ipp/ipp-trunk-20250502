# include "data.h"

int reindex (int argc, char **argv) {
  
  int  i, Nmax, N;
  Vector *ivec, *ovec, *xvec;

  ivec = ovec = xvec = NULL;
  int Npts = 0;

  int KEEP_UNMATCH = FALSE;
  if ((N = get_argument (argc, argv, "-keep-unmatched"))) {
    remove_argument (N, &argc, argv);
    KEEP_UNMATCH = TRUE;
  }

  // BLANKVAL is the name of the value to use for unmatched elements
  char *BLANKVAL = NULL;
  if ((N = get_argument (argc, argv, "-blank")) || (N = get_argument (argc, argv, "-blank-value"))) {
    remove_argument (N, &argc, argv);
    BLANKVAL = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) goto usage;
  if (strcmp(argv[2], "=")) goto usage;
  if (strcmp(argv[4], "using")) goto usage;

  if ((ovec = SelectVector (argv[1], ANYVECTOR, TRUE)) == NULL) goto error;
  if ((ivec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto error;
  if ((xvec = SelectVector (argv[5], OLDVECTOR, TRUE)) == NULL) goto error;

  if (xvec->type != OPIHI_INT) ESCAPE("%s\n", "index is not an integer?");

  // ovec matches ivec in type and xvec in size (xvec need not have all ivec elements, and may have duplicates
  int NPTS = xvec[0].Nelements;
  ResetVector (ovec, ivec->type, xvec[0].Nelements);

  Nmax = ivec[0].Nelements - 1;

  // we have two cases: (ivec == flt or int)
  if (ivec->type == OPIHI_FLT) {
    opihi_flt *vi = ivec[0].elements.Flt;
    opihi_int *vx = xvec[0].elements.Int;
    opihi_flt myBlank = (BLANKVAL == NULL) ? NAN : atof(BLANKVAL);
    for (Npts = i = 0; i < xvec[0].Nelements; i++, vx++) {
      if (Npts >= NPTS) {
	NPTS += 2000;
	REALLOCATE (ovec[0].elements.Flt, opihi_flt, NPTS);
      }
      if (*vx < 0) {
	if (KEEP_UNMATCH) {
	  ovec[0].elements.Flt[Npts] = myBlank;
	  Npts++;
	} 
	continue;
      }
      if (*vx > Nmax) ESCAPE("unexpected value in index: "OPIHI_INT_FMT" (%d)\n", *vx, i);
      ovec[0].elements.Flt[Npts] = vi[*vx];
      Npts++;
    }
  }
  if (ivec->type == OPIHI_INT) {
    opihi_int *vi = ivec[0].elements.Int;
    opihi_int *vx = xvec[0].elements.Int;
    opihi_int myBlank = (BLANKVAL == NULL) ? -4096 : atoi(BLANKVAL);
    for (Npts = i = 0; i < xvec[0].Nelements; i++, vx++) {
      if (Npts >= NPTS) {
	NPTS += 2000;
	REALLOCATE (ovec[0].elements.Int, opihi_int, NPTS);
      }
      if (*vx < 0) {
	if (KEEP_UNMATCH) {
	  ovec[0].elements.Int[Npts] = myBlank;
	  Npts++;
	} 
	continue;
      }
      if (*vx > Nmax) ESCAPE("unexpected value in index: "OPIHI_INT_FMT" (%d)\n", *vx, i);
      ovec[0].elements.Int[Npts] = vi[*vx];
      Npts++;
    }
  }
  if (ivec->type == OPIHI_STR) {
    opihi_int *vx = xvec[0].elements.Int;
    char *myBlank = (BLANKVAL == NULL) ? strcreate ("") : strcreate(BLANKVAL);
    for (Npts = i = 0; i < xvec[0].Nelements; i++, vx++) {
      if (Npts >= NPTS) {
	NPTS += 2000;
	REALLOCATE (ovec[0].elements.Str, char *, NPTS);
      }
      if (*vx < 0) {
	if (KEEP_UNMATCH) {
	  ovec[0].elements.Str[Npts] = strcreate(""); // XXX use a different or a specified value?
	  Npts++;
	} 
	continue;
      }
      if (*vx > Nmax) ESCAPE("unexpected value in index: "OPIHI_INT_FMT" (%d)\n", *vx, i);
      ovec[0].elements.Str[Npts] = strcreate(ivec->elements.Str[*vx]);
      Npts++;
    }
    FREE (myBlank);
  }

  // free up unused memory
  ResetVector (ovec, ivec->type, Npts);
  FREE (BLANKVAL);
  return (TRUE);

error:
  DeleteVector (ovec);
  FREE (BLANKVAL);
  return (FALSE);

usage:
    gprint (GP_ERR, "USAGE: reindex (out) = (in) using (index)\n");
    gprint (GP_ERR, "  Creates a new vector (out) from (in) based on sequence in (index)\n");
    gprint (GP_ERR, "  output[i] = input[index[i]]\n");
    gprint (GP_ERR, "  If -keep-unmatched is provided, elements of index with negative values will be set to NaN / -4096 (or specific value)\n");
    gprint (GP_ERR, "    otherwise they will be skipped in the output.\n");
    gprint (GP_ERR, "  If -blank-value is provided, this supplied value will be used for unmatched elements\n");
    gprint (GP_ERR, "  The output vector has the type of the input vector and the length of the index (if -keep-unmatched).\n");
    gprint (GP_ERR, "  The index vector may have duplicates\n");
    FREE (BLANKVAL);
    return (FALSE);
}

