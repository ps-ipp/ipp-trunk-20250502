# include "gophot.h"

bool transmask (int ix, int iy, float sky) {

  bool value;
  int i, j, ii, jj;
  float tmp, tmp0, tmp1, tmp2, ratio, hump2;
  float tmpB, tmpN, df;
  float *Bval, *Nval, cval, tmp3;

  value = FALSE;

  if (test7) 
    hump2 = crit7;
  else
    hump2 = SQ (bumpcrit);

  tmp0 = tmp1 = tmp2 = 0;
  tmpB = tmpN = tmp3 = 0;

  cval = big[ix + iy*nfast];

  jj = iy - iyby2;
  for (j = 0; j < 2*iyby2 + 1; j++, jj++) {
    if (jj < 0) continue;
    if (jj > nslow - 1) continue;
    
    ii = ix - ixby2;
    Bval = &big[ii + jj*nfast];
    Nval = &noise[ii + jj*nfast];
    for (i = 0; i < 2*ixby2 + 1; i++, ii++, Bval++, Nval++) {
      if (ii < 0) continue;
      if (ii > nfast - 1) continue;
      if (!finite (*Nval)) continue;

# if (1)
      tmp0 ++;
      tmp = starmask[i][j] / (*Nval + *Bval);
      tmp1 += tmp*starmask[i][j];
      tmp2 += tmp*(*Bval-sky);
      
# else

      tmp0 ++;
      tmp1 += (*Nval + *Bval);
      tmp2 += (*Bval - sky);

      df = (cval - *Bval);
      tmp3 +=  df * fabs(df) / (*Nval + *Bval);

# endif

      tmpB += (*Bval - sky);
      tmpN += (*Nval - rnoise);
    }
  }

  if (tmp0 == 0) return (FALSE);

  if (tmpB < tmpN) return (FALSE);  

  if (tmp2 > 0) {
    ratio = tmp2*tmp2/tmp1;

# if (1)
    if (ratio > hump2) {
      value = TRUE;
      mprint (3, " trigger 1 on new object: %d, %d,  %f %f   %f %f %f\n", ix, iy, ratio, tmp3, sky, tmp2, tmp1);
      if (ratio < 1.1*hump2) mprint (3, "marginal: (s/n)**2 through mask = %f\n", ratio);
    }
# else
    if ((ratio > hump2) && (tmp3 > 0.25*ratio)) {
      value = TRUE;
      mprint (3, " trigger 1 on new object: %d, %d,  %f %f   %f %f %f\n", ix, iy, ratio, tmp3, sky, tmp2, tmp1);
      if (ratio < 1.1*hump2) mprint (3, "marginal: (s/n)**2 through mask = %f\n", ratio);
    }
# endif

  }

  if (value) mprint (3, " trigger 2 on new object: %d, %d\n", ix, iy);

  return (value);

}


  /* t1 = 1/N
     t2 = d/N
     rat = N d^2 / N^2 = d^2 / N
  */

/* does not check if bump is significant in region, 
   only if mask region is significant over sky */
