# include <dvo.h>
# define MAX_ORDER 3

/*** This file contains deprecated functions which were used to implement the 2D zero point variations per image.
     This code has not been used for a long time, and is not up-to-date wrt the change from
     millimag shorts to mag floats for values in the db tables.  As of 2011.02.03, these
     structures are no longer used.  There is now an (as yet unused) index in the image table
     to a zero-point map table.  Someday, we may need similarly named functions to interact
     with the ZP map.  for now, this file is not used in the build.
***/

/* convert double to low-precision short int */
short int putMi (double value) {
  
  double T;
  unsigned int I;

  if (value > 0) {
    T = 1000 * log10 (value);
    I = MAX (MIN (T, 16383), -16382) + 0x3fff;
    return (I);
  }
  if (value < 0) {
    T = -value;
    T = 1000 * log10 (T);
    I = MAX (MIN (T, 16383), -16382) + 0xbfff;
    return (I);
  }
  if (value == 0) {
    I = 0;
    return (0);
  }
  return (NAN_S_SHORT);
}

/* convert low-precision short int to double */
double getMi (short int value) {
  
  double T, V, sign;
  short int I;

  if (value == 0) {
    return (0.0);
  }
  if (value & 0x8000) {
    sign = -1;
    I = value & 0x7fff;
  } else {
    sign = +1;
    I = value;
  }
    
  T = (I - 0x3fff) / 1000.0;
  V = sign * pow (10.0, T);
  return (V);
}

/* convert image parameters to c[i] coeffs */
void returnMcal (Image *image, double *c) {

  switch (image[0].order) {
  case 0:
    c[0] = image[0].Mcal;
    return;
  case 1:
    c[0] = image[0].Mcal;
    c[1] = getMi (image[0].Mx);
    c[2] = getMi (image[0].My);
    return;
  case 2:
    c[0] = image[0].Mcal;
    c[1] = getMi (image[0].Mx);
    c[2] = getMi (image[0].Mxx);
    c[3] = getMi (image[0].My);
    c[4] = getMi (image[0].Mxy);
    c[5] = getMi (image[0].Myy);
    return;
  case 3:
    c[0] = image[0].Mcal;
    c[1] = getMi (image[0].Mx);
    c[2] = getMi (image[0].Mxx);
    c[3] = getMi (image[0].Mxxx);
    c[4] = getMi (image[0].My);
    c[5] = getMi (image[0].Mxy);
    c[6] = getMi (image[0].Mxxy);
    c[7] = getMi (image[0].Myy);
    c[8] = getMi (image[0].Mxyy);
    c[9] = getMi (image[0].Myyy);
    return;
  case 4:
    c[0] = image[0].Mcal;
    c[1] = getMi (image[0].Mx);
    c[2] = getMi (image[0].Mxx);
    c[3] = getMi (image[0].Mxxx);
    c[4] = getMi (image[0].Mxxxx);
    c[5] = getMi (image[0].My);
    c[6] = getMi (image[0].Mxy);
    c[7] = getMi (image[0].Mxxy);
    c[8] = getMi (image[0].Mxxxy);
    c[9] = getMi (image[0].Myy);
    c[10] = getMi (image[0].Mxyy);
    c[11] = getMi (image[0].Mxxyy);
    c[12] = getMi (image[0].Myyy);
    c[13] = getMi (image[0].Mxyyy);
    c[14] = getMi (image[0].Myyyy);
    return;
  default:
    c[0] = 0;
    return;
  }
}

void assignMcal (Image *image, double *c, int order) {

  image[0].order = order;

  switch (order) {
  case 0:
    image[0].Mcal = c[0];
    return;
  case 1:
    image[0].Mcal = c[0];
    image[0].Mx    = putMi(c[1]);
    image[0].My    = putMi(c[2]);
    return;
  case 2:
    image[0].Mcal = c[0];
    image[0].Mx    = putMi(c[1]);
    image[0].Mxx   = putMi(c[2]);
    image[0].My    = putMi(c[3]);
    image[0].Mxy   = putMi(c[4]);
    image[0].Myy   = putMi(c[5]);
    return;
  case 3:
    image[0].Mcal = c[0];
    image[0].Mx    = putMi(c[1]);
    image[0].Mxx   = putMi(c[2]);
    image[0].Mxxx  = putMi(c[3]);
    image[0].My    = putMi(c[4]);
    image[0].Mxy   = putMi(c[5]);
    image[0].Mxxy  = putMi(c[6]);
    image[0].Myy   = putMi(c[7]);
    image[0].Mxyy  = putMi(c[8]);
    image[0].Myyy  = putMi(c[9]);
    return;
  case 4:
    image[0].Mcal = c[0];
    image[0].Mx    = putMi(c[1]);
    image[0].Mxx   = putMi(c[2]);
    image[0].Mxxx  = putMi(c[3]);
    image[0].Mxxxx = putMi(c[4]);
    image[0].My    = putMi(c[5]);
    image[0].Mxy   = putMi(c[6]);
    image[0].Mxxy  = putMi(c[7]);
    image[0].Mxxxy = putMi(c[8]);
    image[0].Myy   = putMi(c[9]);
    image[0].Mxyy  = putMi(c[10]);
    image[0].Mxxyy = putMi(c[11]);
    image[0].Myyy  = putMi(c[12]);
    image[0].Mxyyy = putMi(c[13]);
    image[0].Myyyy = putMi(c[14]);
    return;
  default:
    image[0].Mcal = 0.0;
    image[0].order = 0;
    return;
  }
}

/* return value of image fit at x,y */
double applyMcal (Image *image, double x, double y) {

  double Mcal, c[15];

  returnMcal (image, c);  /* convert image parameters to c[i] coeffs */
  Mcal = 0.0;
  switch (image[0].order) {
  case 0:
    Mcal = c[0];
    break;
  case 1:
    Mcal = c[0] + x*c[1] + y*c[2];
    break;
  case 2:
    Mcal = c[0] + x*(c[1] + x*c[2]) + y*(c[3] + x*c[4] + y*c[5]);
    break;
  case 3:
    Mcal = c[0] + x*(c[1] + x*(c[2] + x*c[3])) + y*(c[4] + x*(c[5] + x*c[6]) + y*(c[7] + x*c[8] + y*c[9]));
    break;
  case 4:
    Mcal = c[0] + x*(c[1] + x*(c[2] + x*(c[3] + x*c[4]))) + y*(c[5] + x*(c[6] + x*(c[7] + x*c[8])) + y*(c[9] + x*(c[10] + x*c[11]) + y*(c[12] + x*c[13] + y*c[14])));
    break;
  }
  return (Mcal);
}

double findscatter (double *X, double *Y, double *M, double *dM, int N, double *c, int order) {
  
  int i;
  double *x, *y, *m, *dm;
  double S, S2, s, dS;

  S2 = S = 0;
  if (N < 2) {
    return (0.0);
  }
  
  x = X; y = Y; m = M; dm = dM;
  switch (order) {
  case 0:
    for (i = 0; i < N; i++, m++, dm++) {
      dS  = *m - c[0];
      *dm = dS;
      S  += dS;
      S2 += dS*dS;
    }
    break;
  case 1:
    for (i = 0; i < N; i++, m++, x++, y++, dm++) {
      s   = c[0] + *x*c[1] + *y*c[2];
      dS  = *m - s;
      *dm = dS;
      S  += dS;
      S2 += dS*dS;
    }
    break;
  case 2:
    for (i = 0; i < N; i++, m++, x++, y++, dm++) {
      s   = c[0] + *x*(c[1] + *x*c[2]) + *y*(c[3] + *x*c[4] + *y*c[5]);
      dS  = *m - s;
      *dm = dS;
      S  += dS;
      S2 += dS*dS;
    }
    break;
  case 3:
    for (i = 0; i < N; i++, m++, x++, y++, dm++) {
      s   = c[0] + *x*(c[1] + *x*(c[2] + *x*c[3])) + *y*(c[4] + *x*(c[5] + *x*c[6]) + *y*(c[7] + *x*c[8] + *y*c[9]));
      dS  = *m - s;
      *dm = dS;
      S  += dS;
      S2 += dS*dS;
    }
    break;
  case 4:
    for (i = 0; i < N; i++, m++, x++, y++, dm++) {
      s   = c[0] + *x*(c[1] + *x*(c[2] + *x*(c[3] + *x*c[4]))) + *y*(c[5] + *x*(c[6] + *x*(c[7] + *x*c[8])) + *y*(c[9] + *x*(c[10] + *x*c[11]) + *y*(c[12] + *x*c[13] + *y*c[14])));
      dS  = *m - s;
      *dm = dS;
      S  += dS;
      S2 += dS*dS;
    }
    break;
  }
  S = S / N;
  S2 = sqrt (S2 / N - S*S);
  return (S2);
}
