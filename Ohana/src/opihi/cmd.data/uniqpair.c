# include "data.h"

/* given two 32bit IDs, a 64bit joint ID and sort it, along with an index (sequence) vector */ 
int uniqpair (int argc, char **argv) {

  int N;
  Vector *ID1vec = NULL, *ID2vec = NULL, *OUTvec = NULL;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  Vector *SEQdup = NULL;
  if ((N = get_argument (argc, argv, "-d"))) {
    remove_argument (N, &argc, argv);
    if ((SEQdup = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, "invalid vector %s\n", "SEQdups");
      return FALSE;
    }
    remove_argument (N, &argc, argv);
  }

  Vector *CNTvec = NULL;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    if ((CNTvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, "invalid vector %s\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: uniqpair (ID1) (ID2) (IDout) [-c count] [-d dupindex]\n");
    gprint (GP_ERR, "  merge ID1 and ID2 (32 bit int values) into a single index and find unique entries\n");
    gprint (GP_ERR, "  -c count: save the number of each unique entry in the count vector\n");
    gprint (GP_ERR, "  -d dupindex: save the sequence number of duplicate entries in the dupindex vector\n");
    return (FALSE);
  }

  if ((ID1vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ID2vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((OUTvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if ((ID1vec->type == OPIHI_FLT) || (ID2vec->type == OPIHI_FLT)) {
    gprint (GP_ERR, "ERROR: ID vectors much be int\n");
    return (FALSE);
  }
  if (ID1vec->Nelements != ID2vec->Nelements) {
    gprint (GP_ERR, "ERROR: ID vectors not all the same length\n");
    return (FALSE);
  }

  // storage for the output and count vectors
  int D_NOUT = 0.02*ID1vec->Nelements;

  int Nout = 0;
  int NOUT = D_NOUT;
  ResetVector (OUTvec, OPIHI_INT, NOUT);
  if (CNTvec) {
    ResetVector (CNTvec, OPIHI_INT, NOUT);
  }

  // storage for the duplicate sequences
  int NseqDup = 0;
  int NSEQDUP = 10000;
  if (SEQdup) {
    ResetVector (SEQdup, OPIHI_INT, NSEQDUP);
  }
  
  // incrementing pointers to the input IDs
  opihi_int *ID1 = ID1vec->elements.Int;
  opihi_int *ID2 = ID2vec->elements.Int;

  // generate the joined and sequence vectors
  opihi_int *IDfull = NULL;
  opihi_int *SEQdata = NULL;
  ALLOCATE (IDfull, opihi_int, ID1vec->Nelements);
  ALLOCATE (SEQdata, opihi_int, ID1vec->Nelements);
  for (int i = 0; i < ID1vec->Nelements; i++) {
    IDfull[i] = ID1[i] + (ID2[i] << 32);
    SEQdata[i] = i;
  }

  // XXX watch out: this all needs to get rationalized...
  llsortpair ((off_t *)IDfull, (off_t *)SEQdata, ID1vec->Nelements);

  opihi_int *vtgt = OUTvec->elements.Int;
  opihi_int *vsrc = IDfull;

  int onePercent = ID1vec->Nelements / 100;

  struct sigaction *old_sigaction = SetInterrupt();
  for (int i = 0; (i < ID1vec->Nelements) && !interrupt; Nout++) {
    if (Nout >= NOUT) {
      NOUT += D_NOUT;
      REALLOCATE (vtgt, opihi_int, NOUT);
      if (CNTvec) {
	REALLOCATE (CNTvec->elements.Int, opihi_int, NOUT);
      }
    }
    vtgt[Nout] = *vsrc;
    int Ndup = 0;
    opihi_int lastValue = *vsrc;
    while ((i < ID1vec->Nelements) && (*vsrc == lastValue)) {
      i++;
      vsrc ++;
      Ndup ++;
      if (VERBOSE && (i % onePercent == 0)) gprint (GP_ERR, ".");
    }
    if (CNTvec) {
      CNTvec->elements.Int[Nout] = Ndup;
    }
    if (SEQdup && (Ndup > 1)) {
      // we want to save SEQdata values for the duplicates
      // SEQdata[i-Ndup] .. SEQdata[i-N
      for (int j = i - Ndup; j < i; j++) {
	SEQdup->elements.Int[NseqDup] = SEQdata[j];
	NseqDup ++;
	if (NseqDup >= NSEQDUP) {
	  NSEQDUP += 10000;
	  REALLOCATE (SEQdup->elements.Int, opihi_int, NSEQDUP);
	}
      }
    }
  }
  ClearInterrupt (old_sigaction);
  if (VERBOSE) gprint (GP_ERR, "\n");

  free (SEQdata);
  free (IDfull);

  if (SEQdup) {
    SEQdup->Nelements = NseqDup;
    REALLOCATE (SEQdup->elements.Int, opihi_int, NseqDup);
  }

  // fix references and free up extra memory:
  OUTvec->elements.Int = vtgt;
  ResetVector (OUTvec, OPIHI_INT, Nout);
  if (CNTvec) ResetVector (CNTvec, OPIHI_INT, Nout);

  return (TRUE);
}
