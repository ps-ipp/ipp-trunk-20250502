# include "data.h"
// NOTE: if there are only a few uniq values, the old algorithm is not bad.  
// for 10000 uniq values, 30M points take ~20sec in the new algorithm, 
// 3M points takess 45 sec in the old method.

int uniq (int argc, char **argv) {
  
  int Nnew, i, N;
  Vector *ivec, *ovec;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  Vector *cvec = NULL;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    if ((cvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, "invalid vector %s\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: uniq (in) (out) -c count\n");
    return (FALSE);
  }

  if ((ivec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  /* allocate the maximum possible needed */
  ResetVector (ovec, ivec->type, ivec->Nelements);
  if (cvec) {
    ResetVector (cvec, OPIHI_INT, ivec->Nelements);
  }

  Nnew = 0;

  if (ivec->type == OPIHI_FLT) {
    // copy the input data to a temporary array to avoid damaging it with sort
    opihi_flt *indata = NULL;
    ALLOCATE (indata, opihi_flt, ivec[0].Nelements);

    // manually copy and exclude any nan values
    int Nindata = 0;
    for (i = 0; i < ivec->Nelements; i++) {
      if (!isfinite(ivec[0].elements.Flt[i])) continue;
      indata[Nindata] = ivec[0].elements.Flt[i];
      Nindata ++;
    }
    // XXX do not do this: memcpy (indata, ivec->elements.Flt, ivec[0].Nelements*sizeof(opihi_flt));

    dsort (indata, Nindata);

    Nnew = 0;
    opihi_flt *vtgt = ovec[0].elements.Flt;

    opihi_flt *vsrc = indata;

    int onePercent = Nindata / 100;

    struct sigaction *old_sigaction = SetInterrupt();
    for (i = 0; (i < Nindata) && !interrupt; Nnew++) {
      vtgt[Nnew] = *vsrc;
      int Ndup = 0;
      opihi_flt lastValue = *vsrc;
      while ((i < ivec->Nelements) && (*vsrc == lastValue)) {
	i++;
	vsrc ++;
	Ndup ++;
	if (VERBOSE && (i % onePercent == 0)) gprint (GP_ERR, ".");
      }
      if (cvec) {
	cvec->elements.Int[Nnew] = Ndup;
      }
    }
    ClearInterrupt (old_sigaction);
    if (VERBOSE) gprint (GP_ERR, "\n");
    free (indata);
  } else {
    // copy the input data to a temporary array to avoid damaging it with sort
    opihi_int *indata = NULL;
    ALLOCATE (indata, opihi_int, ivec[0].Nelements);
    memcpy (indata, ivec->elements.Int, ivec[0].Nelements*sizeof(opihi_int));

    llsort (indata, ivec->Nelements);

    Nnew = 0;
    opihi_int *vtgt = ovec[0].elements.Int;

    opihi_int *vsrc = indata;

    int onePercent = ivec->Nelements / 100;

    struct sigaction *old_sigaction = SetInterrupt();
    for (i = 0; (i < ivec->Nelements) && !interrupt; Nnew++) {
      vtgt[Nnew] = *vsrc;
      int Ndup = 0;
      opihi_int lastValue = *vsrc;
      while ((i < ivec->Nelements) && (*vsrc == lastValue)) {
	i++;
	vsrc ++;
	Ndup ++;
	if (VERBOSE && (i % onePercent == 0)) gprint (GP_ERR, ".");
      }
      if (cvec) {
	cvec->elements.Int[Nnew] = Ndup;
      }
    }
    ClearInterrupt (old_sigaction);
    if (VERBOSE) gprint (GP_ERR, "\n");
    free (indata);
  }

  // free up extra memory
  ResetVector (ovec, ivec->type, Nnew);
  if (cvec) ResetVector (cvec, OPIHI_INT, Nnew);

  return (TRUE);
}

