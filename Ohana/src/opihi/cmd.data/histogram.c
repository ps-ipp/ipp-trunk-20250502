# include "data.h"

int histogram (int argc, char **argv) {
  
  int i, N, bin, Nbins;
  opihi_int *OUT;
  opihi_flt start, end, delta;
  Vector *xvec, *yvec, *range;

  range = NULL;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    if ((range = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }

  if ((argc != 6) && (argc != 5)) {
    gprint (GP_ERR, "USAGE: hist invec outvec start end [delta] [-range range]\n");
    return (FALSE);
  }

  delta = 1;
  start = atof (argv[3]);
  end   = atof (argv[4]);
  if (argc == 6) delta = atof (argv[5]);
 
  if ((start == end) || (delta == 0)) {
    gprint (GP_ERR, "error in value: %f to %f, %f\n", start, end, delta);
    return (FALSE);
  }
  delta = fabs (delta);
  if (end - start < 0) {
    delta = -1.0 * delta;
  }
  Nbins = (end - start) / delta;
  /* number here should match number in create.c */

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  if (range) {
    ResetVector (range, OPIHI_FLT, Nbins);
    for (i = 0; i < range[0].Nelements; i++) {
      range[0].elements.Flt[i] = start + i*delta;
    }
  }

  ResetVector (yvec, OPIHI_INT, Nbins);
  bzero (yvec[0].elements.Int, sizeof(opihi_int)*yvec[0].Nelements);
  if (Nbins < 1) return (TRUE);
  OUT = yvec[0].elements.Int;

  if (xvec[0].type == OPIHI_FLT) {
    opihi_flt *V = xvec[0].elements.Flt;
    for (i = 0; i < xvec[0].Nelements; i++, V++) {
      if (isnan(*V)) continue;
      bin = MIN (MAX (0, (*V - start) / delta), Nbins - 1);
      OUT[bin]++;
    }      
  } else {
    opihi_int *V = xvec[0].elements.Int;
    for (i = 0; i < xvec[0].Nelements; i++, V++) {
      bin = MIN (MAX (0, (*V - start) / delta), Nbins - 1);
      OUT[bin]++;
    }      
  }

  return (TRUE);

}
