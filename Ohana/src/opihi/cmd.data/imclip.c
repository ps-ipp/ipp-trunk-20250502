# include "data.h"

int imclip (int argc, char **argv) {

  int i, Npix, DO_NAN, DO_INF, N;
  double min, Vmin, max, Vmax, nan_val, inf_val;
  float *in;
  Buffer *buf;

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
    gprint (GP_ERR, "USAGE: clip (buffer) [min Vmin max Vmax] [-inf val] [-nan val]\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if (argc == 6) {
    min = atof (argv[2]);
    Vmin = atof (argv[3]);
    max = atof (argv[4]);
    Vmax = atof (argv[5]);
  }

  Npix = buf[0].matrix.Naxis[0]*buf[0].matrix.Naxis[1];
  in = (float *) buf[0].matrix.buffer;

  if (argc == 6) {
    for (i = 0; i < Npix; i++, in++) {
      if (*in < min) 
	*in = Vmin;
      if (*in > max)
	*in = Vmax;
    }
  }
  in = (float *) buf[0].matrix.buffer;
  if (DO_NAN) {
    for (i = 0; i < Npix; i++, in++) {
      if (isnan (*in)) {
	*in = nan_val;
      }
    }
  }
  in = (float *) buf[0].matrix.buffer;
  if (DO_INF) {
    for (i = 0; i < Npix; i++, in++) {
      if (!finite (*in)) {
	*in = inf_val;
      }
    }
  }
  return (TRUE);

}


