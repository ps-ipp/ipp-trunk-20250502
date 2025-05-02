# include "data.h"

int medacc (int argc, char **argv) {
  
  int i, j, Nbins, Nvalues, N, N0, N1;
  double start, end, delta, k0, k1, fn;
  opihi_flt *V, *K, *V1, *K1, *O, *tmpvec, *tmpkey;
  Vector *val, *key, *out;

  Vector *stdev = NULL;
  if ((N = get_argument (argc, argv, "-stdev"))) {
    remove_argument (N, &argc, argv);
    stdev = SelectVector (argv[N], ANYVECTOR, TRUE);
    remove_argument (N, &argc, argv);
    if (!stdev) {
      gprint (GP_ERR, "output vector for standard deviation not found\n");
      return (FALSE);
    }
  }

  if ((argc != 6) && (argc != 7)) {
    gprint (GP_ERR, "USAGE: medacc <value> <vector> <key> start end [delta]\n");
    gprint (GP_ERR, "  value and key are vectors of the same length\n");
    gprint (GP_ERR, "  the output vector bins correspond to values ranging from start to end in steps of delta (default 1)\n");
    gprint (GP_ERR, "  for a given output bin, all input values for which the key matches the output bin are used to calculate the statistic\n");

    gprint (GP_ERR, "  the output is the average of the inner 25%% of data values\n");
    gprint (GP_ERR, "  -stdev (outvec) : optionally return the standard deviation, calculated based on the 68%%-ile range\n");

    return (FALSE);
  }

  if ((val = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((key = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (val[0].Nelements != key[0].Nelements) {
    gprint (GP_ERR, "key and value don't match\n");
    return (FALSE);
  }
  if ((out = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (val, FALSE); 
  REQUIRE_VECTOR_FLT (key, FALSE); 

  start = atof (argv[4]);
  end   = atof (argv[5]);
  if (argc == 7) 
    delta = atof (argv[6]);
  else 
    delta = 1;

  delta = fabs (delta);
  if (end - start < 0) {
    delta = -1.0 * delta;
  }
  Nbins = 1 + (int)((end - start) / delta);

  ResetVector (out, OPIHI_FLT, Nbins);
  bzero (out[0].elements.Flt, sizeof(opihi_flt)*out[0].Nelements);

  if (stdev) {
    ResetVector (stdev, OPIHI_FLT, Nbins);
  }

  /* copy vec and key to temp vectors */
  ALLOCATE (tmpvec, opihi_flt, val[0].Nelements);
  ALLOCATE (tmpkey, opihi_flt, val[0].Nelements);

  V = val[0].elements.Flt;
  K = key[0].elements.Flt;
  V1 = tmpvec;
  K1 = tmpkey;
  Nvalues = val[0].Nelements;
  for (i = 0; i < Nvalues; i++, V++, K++, V1++, K1++) {
    *V1 = *V;
    *K1 = *K;
  }      

  /* sort vec and key by key */
  dsortpair (tmpkey, tmpvec, Nvalues);

  O = out[0].elements.Flt;
  /* find the start and end key for each range */
  N0 = 0;
  N1 = 0;
  for (i = 0; i < Nbins; i++) {
    k0 = i*delta + start;
    k1 = (i+1)*delta + start;
    for (j = N1; (j < Nvalues) && (tmpkey[j] < k0); j++);
    N0 = j;
    for (j = N0; (j < Nvalues) && (tmpkey[j] < k1); j++);
    N1 = j;
    N = N1 - N0;
    dsort (&tmpvec[N0], N);
    fn = O[i] = 0;
    for (j = N0 + 0.25*N; j < N0 + 0.75*N; j++) {
      O[i] += tmpvec[j];
      fn += 1.0;
    }
    if (fn > 0) O[i] /= fn;

    if (stdev) {
      int Nmin = 0.16*N;
      int Nmax = 0.84*N;
      float Smin = tmpvec[N0 + Nmin];
      float Smax = tmpvec[N0 + Nmax];
      stdev[0].elements.Flt[i] = 0.5*(Smax - Smin);
    }
  }      
  
  free (tmpvec);
  free (tmpkey);

  return (TRUE);
}

