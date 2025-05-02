# include "astro.h"

int profile (int argc, char **argv) {
  
  int i, j, N, Nx, Npt;
  float *V;
  double sx, sy;
  Vector *xvec, *yvec;
  Buffer *buf;

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: profile <buffer> <X vector> <Y vector> x y N\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  
  sx = atof (argv[4]);
  sy = atof (argv[5]);
  N  = atof (argv[6]);

  if (sx - N < 0) goto range_error;
  if (sy - N < 0) goto range_error;
  if (sx + N > buf[0].matrix.Naxis[0]) goto range_error;
  if (sy + N > buf[0].matrix.Naxis[1]) goto range_error;

  if ((xvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  ResetVector (xvec, OPIHI_FLT, (int)SQ(2*N+1));
  ResetVector (yvec, OPIHI_FLT, (int)SQ(2*N+1));

  bzero (yvec[0].elements.Flt, (int)SQ(2*N+1)*sizeof(opihi_flt));
  V = (float *)(buf[0].matrix.buffer); 
  Npt = 0;
  Nx = buf[0].matrix.Naxis[0];
  for (i = sx - N; i <= sx + N; i++) {
    for (j = sy - N; j <= sy + N; j++, Npt++) {
      yvec[0].elements.Flt[Npt] = V[i + j*Nx];
      xvec[0].elements.Flt[Npt] = hypot (i + 0.5 - sx, j + 0.5 - sy);
    }
  }

  dsortpair (xvec[0].elements.Flt, yvec[0].elements.Flt, xvec[0].Nelements);

  return (TRUE);

range_error:
  gprint (GP_ERR, "region out of range\n");
  return (FALSE);
}

