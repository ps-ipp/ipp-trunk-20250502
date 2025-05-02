/** @file psBinomialCoeff.c
 *
 *  @brief Calculate binomial coefficient
 *
 *  @author Chris Waters, IfA
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "psError.h"

#include "psBinomialCoeff.h"

psS64 psBinomialCoeff( psS64 N,
		       psS64 k) {
  psS64 i;
  psS64 b = 1;
  if (N == k) { return (b); }
  if (k == 0) { return (b); }

  for (i = 1; i <= k; i++) {
    b *= (N - (k - i))/i;
  }

  return(b);
}

