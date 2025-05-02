# include "data.h"

int vbin (int argc, char **argv) {
  
  int i, j, n, N, Nin, Nout;
  int Normalize, Ignore;
  opihi_flt *Vout, IgnoreValue;
  double scale;
  Vector *in, *out;

  Normalize = FALSE;
  if ((N = get_argument (argc, argv, "-norm"))) {
    remove_argument (N, &argc, argv);
    Normalize = TRUE;
  }

  Ignore = FALSE;
  IgnoreValue = 0.0;
  if ((N = get_argument (argc, argv, "-ignore"))) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: vbin <input> <output> scale \n");
    gprint (GP_ERR, "  (use interpolate to expand)\n");
    return (FALSE);
  }

  if ((in  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((out = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  scale  = atof (argv[3]);
  if ((int)(scale) != scale) {
    gprint (GP_ERR, "integer binning only, please\n");
    return (FALSE);
  }

  Nin  = in[0].Nelements;
  Nout = Nin / scale;

  // re-binning creates a float vector
  ResetVector (out, OPIHI_FLT, Nout);

  Vout = out[0].elements.Flt;

  if (in[0].type == OPIHI_FLT) {
    opihi_flt *Vin  = in[0].elements.Flt;
    for (n = j = 0; j < Nout; j++, Vout++) {
      *Vout = 0;
      for (N = i = 0; (i < scale) && (n < Nin); n++, i++, Vin++) {
	if (!finite (*Vin)) continue;
	if (Ignore && (*Vin == IgnoreValue)) continue;
	*Vout += *Vin;
	N ++;
      } 
      if (Normalize) {
	if (N > 0) { 
	  *Vout /= (opihi_flt) N; 
	} else {
	  *Vout = 0;
	}
      }
    }
  } else {
    opihi_int *Vin  = in[0].elements.Int;
    for (n = j = 0; j < Nout; j++, Vout++) {
      *Vout = 0;
      for (N = i = 0; (i < scale) && (n < Nin); n++, i++, Vin++) {
	if (!finite (*Vin)) continue;
	if (Ignore && (*Vin == IgnoreValue)) continue;
	*Vout += *Vin;
	N ++;
      } 
      if (Normalize) {
	if (N > 0) { 
	  *Vout /= (opihi_flt) N; 
	} else {
	  *Vout = 0;
	}
      }
    }
  }
  return (TRUE);
}
