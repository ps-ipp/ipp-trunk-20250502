# include "data.h"

int accum (int argc, char **argv) {
  
  int i, Nbins, bin, N, Normalize;
  float start, end, delta;
  int *NV;
  opihi_flt *V, *K, *O;
  Vector *val, *key, *out;

  NV = NULL;
  Normalize = FALSE;
  if ((N = get_argument (argc, argv, "-norm"))) {
    remove_argument (N, &argc, argv);
    Normalize = TRUE;
  }

  if ((argc != 6) && (argc != 7)) {
    gprint (GP_ERR, "USAGE: accum <value> <vector> <key> start end [delta]\n");
    gprint (GP_ERR, "  sum <value> in bins corresponding to the value of <key>\n");
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
  if ((start == end) || (delta == 0)) {
    gprint (GP_ERR, "error in value: %f to %f, %f\n", start, end, delta);
    return (FALSE);
  }
  delta = fabs (delta);
  if (end - start < 0) {
    delta = -1.0 * delta;
  }
  Nbins = (end - start) / delta;

  ResetVector (out, OPIHI_FLT, Nbins);
  bzero (out[0].elements.Flt, sizeof(opihi_flt)*out[0].Nelements);
  if (Normalize) {
    ALLOCATE (NV, int, Nbins);
    bzero (NV, sizeof(int)*Nbins);
  }

  V = val[0].elements.Flt;
  K = key[0].elements.Flt;
  O = out[0].elements.Flt;

  for (i = 0; i < val[0].Nelements; i++, V++, K++) {
    bin = MIN (MAX (0, (*K - start) / delta), Nbins - 1);
    O[bin] += *V;
    if (Normalize) NV[bin] ++;
  }      

  if (Normalize) {
    for (i = 0; i < Nbins; i++) {
      O[i] /= (float) NV[i];
    }
    free (NV);
  }

  return (TRUE);
}

