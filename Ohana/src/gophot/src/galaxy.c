# include "gophot.h"

/* this is kind of stupid:  this function takes two 
   lists of the parameters for the object because 
   star1 has x, y, relative to 0, */

bool galaxy (float *star1, float *err, float *star2) {

  float dummy[NPMAX], tot[4], temp, nsigma;
  int i;
  bool value;

  value = FALSE;

  for (i = 0; i < 4; i++) chi[i] = 0;
	
  parinterp (star2[2], star2[3], dummy);

  /* if errors on sigma x or y are so large, call it a galaxy */
  if (dummy[4] < 3*sqrt(err[4])) return (TRUE);
  if (dummy[6] < 3*sqrt(err[6])) return (TRUE);

  /* too weak a measurement to be called galaxy */
  if (star1[4] < 2*sqrt(err[4])) return (FALSE);
  if (star1[6] < 2*sqrt(err[6])) return (FALSE);
  if (star1[1] < 2*sqrt(err[1])) return (FALSE); 

  /*************************************************************
   I'm rather concerned about 'parsm' where did it come from? 
  parms[] is set in varipar_plane.c  */							
  temp = SQ (sig[1]*dummy[4]);
  tot[1] = MAX (parms[4], temp);
  temp = SQ (sig[2])/(dummy[4]*dummy[6]);
  tot[2] = MAX (parms[5], temp);
  temp = SQ (sig[3]*dummy[6]);
  tot[3] = MAX (parms[6], temp);
  chi[1] = SQ (star1[4] - dummy[4]) / (tot[1] + err[4]);
  chi[2] = SQ (star1[5] - dummy[5]) / (tot[2] + err[5]);
  chi[3] = SQ (star1[6] - dummy[6]) / (tot[3] + err[6]);

  if (star1[4] < dummy[4]) chi[1] = 0;
  if (star1[6] < dummy[6]) chi[3] = 0;
  chi[4] = chi[1] + chi[2] + chi[3];

  mprint (2, "galaxy test, object at %f %f  %f\n", star2[2], star2[3], star2[1]);
  mprint (2, "chisqs: %f %f %f %f\n", chi[1], chi[2], chi[3], chi[4]);

  nsigma = MIN (sqrt(chi[4]), 1.0e8);
  mprint (2, "nsigma = %f\n", nsigma);

  value = chi[4] >= chicrit;

  return (value);

}

/* this function uses C 0,N-1 for a[], fa[] */

