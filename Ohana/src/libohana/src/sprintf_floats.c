# include <ohana.h>

# define USE_LOGS 1
# define NSIGFIG_FLT  7
# define NEXPFIG_FLT  2
# define M_LOG2_10 (M_LN10 / M_LN2)
# define M_LOG10_2 (M_LN2 / M_LN10)

// returns the number of bytes written EXCLUDING the ending NULL char
unsigned int sprintf_float (char *output, float value) {

  // I want a fast way to print a float in %e format.  Simplifying assumptions: print a
  // single float to an existing char buffer using a fixed format: +F.ffffffe+NN 
  // NSIGFIG_FLT = number of digits after decimal point

  // returns: number of chars actually written

  if (value == 0.0) {
    strcpy (output, "+0.0000000+e00");  // NOTE: hard-wired
    return 14;
  }						

  int isPositiveValue = (value > 0.0);

  double pvalue = fabs(value);

# if (USE_LOGS)
  // get the exponent
  double Exponent = log10(pvalue);

  // get the integer component of the exponent
  int iExponent = floor(Exponent);

  // re-raise to the integer power to grab the Mantissa
  double power = pow (10.0, (double) iExponent);

  double Mantissa = pvalue / power;

# else

  // p = frexpf (x,n) returns (p,n) where x = p*2^n
  // we want f = q*10^m
  // s = n * log10(2)
  // m = floor(s)
  // r = s - m
  // t = r * log2(10)
  // q = p * 2^t
  
  int n;
  double p = frexp (pvalue, &n);
  double s = n * M_LOG10_2;
  int m = floor(s);
  double r = s - m;
  double t = r * M_LOG2_10;
  double q = p * pow(2.0, t);

  if (q < 1.0) {
    q *= 10.0;
    m --;
  }

  int iExponent = m;
  double Mantissa = q;
# endif

  // multiplier here must be 10^(NSIGFIG_FLT + 1)
  long long int iMantissa = Mantissa * 100000000;  // NOTE: hard-wired
  
  // example, NSIGFIG_FLT = 6
  // +1.234567e+01 -> 1234567
  // 0123456789012 == 6543210
  // 876543210

  // write the leading sign
  output[0] = isPositiveValue ? '+' : '-';

  // we need to know the NSIGFIG_FLT + 1 digit to know if we should round up or down
  int roundUp = (iMantissa % 10 > 4);
  // fprintf (stderr, "%f : %d : %d\n", value, iMantissa, roundUp);

  // now drop iMantissa to the correct number of digits
  iMantissa = iMantissa / 10;
  if (roundUp) iMantissa ++;
  if (iMantissa > 99999999) {
    iMantissa = iMantissa / 10;
    iExponent ++;
  }
  int isPositiveExp = (iExponent >= 0);

  // now write out the Mantissa backwards as an integer, but include a decimal point
  int i;
  for (i = 0; i < NSIGFIG_FLT + 2; i++) {
    if (i == NSIGFIG_FLT) {
      output[NSIGFIG_FLT + 2 - i] = '.';
      continue;
    }
    output[NSIGFIG_FLT + 2 - i] = '0' + iMantissa % 10;
    iMantissa = iMantissa / 10;
  }
    
  output[NSIGFIG_FLT + 3] = 'e';
  output[NSIGFIG_FLT + 4] = isPositiveExp ? '+' : '-';

  iExponent = abs(iExponent);

  for (i = 0; i < NEXPFIG_FLT; i++) {
    output[NSIGFIG_FLT + 4 + NEXPFIG_FLT - i] = '0' + iExponent % 10;
    iExponent = iExponent / 10;
  }
  output[NSIGFIG_FLT + NEXPFIG_FLT + 5] = 0;
  
  return NSIGFIG_FLT + NEXPFIG_FLT + 5;
}

# define NSIGFIG_DBL 14
# define NEXPFIG_DBL  2

// returns the number of bytes written EXCLUDING the ending NULL char
unsigned int sprintf_double (char *output, double value) {

  // I want a fast way to print a float in %e format.  Simplifying assumptions: print a
  // single float to an existing char buffer using a fixed format: +F.ffffffe+NN 
  // NSIGFIG_DBL = number of digits after decimal point

  // returns: number of chars actually written

  if (value == 0.0) {
    strcpy (output, "+0.00000000000000+e000");  // NOTE: hard-wired
    return 22;
  }						

  int isPositiveValue = (value > 0.0);

  double pvalue = fabs(value);

# if (USE_LOGS)
  // get the exponent
  double Exponent = log10(pvalue);

  // get the integer component of the exponent
  int iExponent = floor(Exponent);

  // re-raise to the integer power to grab the Mantissa
  double power = pow (10.0, (double) iExponent);

  double Mantissa = pvalue / power;

# else

  // p = frexpf (x,n) returns (p,n) where x = p*2^n
  // we want f = q*10^m
  // s = n * log10(2)
  // m = floor(s)
  // r = s - m
  // t = r * log2(10)
  // q = p * 2^t
  
  int n;
  double p = frexp (pvalue, &n);
  double s = n * M_LOG10_2;
  int m = floor(s);
  double r = s - m;
  double t = r * M_LOG2_10;
  double q = p * pow(2.0, t);

  if (q < 1.0) {
    q *= 10.0;
    m --;
  }

  int iExponent = m;
  double Mantissa = q;
# endif

  // multiplier here must be 10^(NSIGFIG_DBL + 1)
  long long int iMantissa = Mantissa * 1000000000000000;  // NOTE: hard-wired
  
  // example, NSIGFIG_DBL = 6
  // +1.234567e+01 -> 1234567
  // 0123456789012 == 6543210
  // 876543210

  // write the leading sign
  output[0] = isPositiveValue ? '+' : '-';

  // we need to know the NSIGFIG_DBL + 1 digit to know if we should round up or down
  int roundUp = (iMantissa % 10 > 4);
  // fprintf (stderr, "%f : %d : %d\n", value, iMantissa, roundUp);

  // now drop iMantissa to the correct number of digits
  iMantissa = iMantissa / 10;
  if (roundUp) iMantissa ++;

  // 1000000000000000
  //  999999999999999

  if (iMantissa > 999999999999999) {
    iMantissa = iMantissa / 10;
    iExponent ++;
  }

  int isPositiveExp = (iExponent >= 0);

  // now write out the Mantissa backwards as an integer, but include a decimal point
  int i;
  for (i = 0; i < NSIGFIG_DBL + 2; i++) {
    if (i == NSIGFIG_DBL) {
      output[NSIGFIG_DBL + 2 - i] = '.';
      continue;
    }
    output[NSIGFIG_DBL + 2 - i] = '0' + iMantissa % 10;
    iMantissa = iMantissa / 10;
  }
    
  output[NSIGFIG_DBL + 3] = 'e';
  output[NSIGFIG_DBL + 4] = isPositiveExp ? '+' : '-';

  iExponent = abs(iExponent);

  for (i = 0; i < NEXPFIG_DBL; i++) {
    output[NSIGFIG_DBL + 4 + NEXPFIG_DBL - i] = '0' + iExponent % 10;
    iExponent = iExponent / 10;
  }
  output[NSIGFIG_DBL + NEXPFIG_DBL + 5] = 0;
  
  return NSIGFIG_DBL + NEXPFIG_DBL + 5;
}
