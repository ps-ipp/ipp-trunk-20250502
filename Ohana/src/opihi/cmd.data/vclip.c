# include "data.h"

int vclip (int argc, char **argv) {

  int i, Npix, DO_NAN, DO_INF, DO_CLIP, N;
  double min, Vmin, max, Vmax, nan_val, inf_val;
  Vector *vec;

  inf_val = nan_val = min = Vmin = max = Vmax = 0;

  DO_NAN = FALSE;
  if ((N = get_argument (argc, argv, "-nan"))) {
    remove_argument (N, &argc, argv);
    nan_val  = atof(argv[N]);
    remove_argument (N, &argc, argv);
    DO_NAN = TRUE;
  }

  DO_INF = FALSE;
  if ((N = get_argument (argc, argv, "-inf"))) {
    remove_argument (N, &argc, argv);
    inf_val  = atof(argv[N]);
    remove_argument (N, &argc, argv);
    DO_INF = TRUE;
  }

  if ((argc != 6) && (!(DO_INF || DO_NAN))) {
    gprint (GP_ERR, "USAGE: vclip (vector) [min Vmin max Vmax] [-inf val] [-nan val]\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  DO_CLIP = FALSE;
  if (argc == 6) {
    min = atof (argv[2]);
    Vmin = atof (argv[3]);
    max = atof (argv[4]);
    Vmax = atof (argv[5]);
    DO_CLIP = TRUE;
  }

  if (DO_NAN && vec[0].type == OPIHI_INT) {
    gprint (GP_ERR, "  warning : clipping on NAN is invalid for INT vectors\n");
    return (FALSE);
  }
    
  if (DO_INF && vec[0].type == OPIHI_INT) {
    gprint (GP_ERR, "  warning : clipping on INF is invalid for INT vectors\n");
    return (FALSE);
  }
    
  Npix = vec[0].Nelements;

  if (vec[0].type == OPIHI_FLT) {
    opihi_flt *in =  vec[0].elements.Flt;
    for (i = 0; i < Npix; i++, in++) {
      if (DO_CLIP && (*in < min)) {
	*in = Vmin;
      }
      if (DO_CLIP && (*in > max)) {
	*in = Vmax;
      }
      if (!finite (*in) && DO_INF) {
	*in = inf_val;
      }
      if (isnan (*in) && DO_NAN) {
	*in = nan_val;
      }
    }
  } else {
    opihi_int *in =  vec[0].elements.Int;
    for (i = 0; i < Npix; i++, in++) {
      if (DO_CLIP && (*in < min)) {
	*in = Vmin;
      }
      if (DO_CLIP && (*in > max)) {
	*in = Vmax;
      }
    }
  }
  return (TRUE);
}
