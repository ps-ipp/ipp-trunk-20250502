# include "data.h"

int vmedfilt (int argc, char **argv) {
  
  Vector *in, *out;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: vmedfilt (input) (output) Npts\n");
    return (FALSE);
  }
  
  if ((in  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((out = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  // smoothing window is 2Ns + 1 wide (require an odd number)
  int Ns = atoi (argv[3]);
  int Np = 2*Ns + 1;

  int Nx = in[0].Nelements;

  ResetVector (out, OPIHI_FLT, Nx);

  // storage for the current sample
  ALLOCATE_PTR (sample, opihi_flt, Np);

  int isFloat = (in[0].type == OPIHI_FLT);
  opihi_flt *vf = in[0].elements.Flt;
  opihi_int *vi = in[0].elements.Int;

  opihi_flt *tf = out[0].elements.Flt;

  for (int i = 0; i < Nx; i++) {

    int s = 0;
    for (int n = -Ns; n <= Ns; n++) {
      if (i+n < 0) continue;
      if (i+n >= Nx) continue;
      opihi_flt value = isFloat ? vf[i+n] : vi[i+n];
      if (isnan(value)) continue;

      sample[s] = value;
      s ++;
    }
    dsort (sample, s);

    if (s == 0) { tf[i] = NAN; continue; }

    int s2 = s / 2;
    if (s % 2) {
      // s == 1, s2 = 0, s == 3, s2 = 1; s == 5, s2 = 2, ..
      tf[i] = sample[s2]; 
    } else {
      // s == 2, s2 = 1, s2-1 = 0; s == 4, s2 = 2, s2-1 = 1; s == 6, s2 = 3, s2-1 = 2..
      tf[i] = 0.5*(sample[s2] + sample[s2-1]); 
    }
  }

  free (sample);
  return (TRUE);
}

// we are running the sort for every sample vector; we could probably do this more efficiently, see:
// https://nomis80.org/ctmf.pdf
// https://arxiv.org/pdf/1406.1717.pdf
