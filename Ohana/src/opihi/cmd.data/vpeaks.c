# include "data.h"

int vpeaks (int argc, char **argv) {

  int N;
  Vector *vecx, *vecf, *vecv;

  float collevel = NAN;
  if ((N = get_argument (argc, argv, "-collevel"))) {
    remove_argument (N, &argc, argv);
    collevel = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: vpeaks (vector) <xvec> <fvec> <threshold>\n");
    gprint (GP_ERR, "  finds coordinate (x) and peak-flux (f) of peaks above threshold in vector\n");
    return (FALSE);
  }

  if ((vecv = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecx = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecf = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  float threshold = atof (argv[4]);
  int Nv = vecv->Nelements;

  // we cannot have more peaks than pixels in the input vector, just start with that:
  ALLOCATE_PTR (peakX, opihi_flt, Nv);
  opihi_flt *value = vecv->elements.Flt;

  int OnPeak = FALSE;
  int Npeaks = 0;
  for (int i = 1; i < Nv - 1; i++) {

    if (!isfinite(value[i])) continue;   // ignore NAN values
    if (OnPeak && isfinite(collevel) && (value[i] < collevel)) OnPeak = FALSE;

    if (value[i] <  threshold) continue; // only accept pixels above threshold
    if (value[i] <  value[i - 1]) continue; // peak pixel must be at least exceed preceeding pixel
    if (value[i] <= value[i + 1]) continue; // we accept the last pixel of a series of equal values
    if (OnPeak && isfinite(collevel) && (value[i] > collevel)) continue;
    
    OnPeak = TRUE;
    peakX[Npeaks] = i;
    Npeaks ++;
  }

  ResetVector (vecf, OPIHI_FLT, Npeaks);
  for (int i = 0; i < Npeaks; i++) {
    int ix = peakX[i];
    vecf->elements.Flt[i] = value[ix];
  }  

  // save the peakX values:
  vecx->Nelements = Npeaks; free (vecx->elements.Flt); vecx->elements.Flt = peakX; 

  return (TRUE);
}
