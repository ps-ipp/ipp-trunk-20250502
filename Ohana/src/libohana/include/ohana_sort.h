// Heap sort based on descriptions in Sedgewick "Algorithms in C"
//
// Copyright (C) 1999  Thomas Walter
// Copyright (C) 2007  Paul Price, Institute for Astronomy, University of Hawaii
// Copyright (C) 2008  Eugene Magnier, Institute for Astronomy, University of Hawaii
//
// 18 February 2000: Modified for GSL by Brian Gough
// 29 November 2007: Modified for psLib by Paul Price
// 07 January 2008: Modified for ohana by Eugene Magnier
//
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option) any
// later version.
//
// This source is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//

#ifndef OHANA_SORT_H
#define OHANA_SORT_H

// The following sort code is based on gsl_heapsort from GSL-1.8 (www.gnu.org/software/gsl), file
// gsl-1.8/sort/sort.c, which is distributed under the GNU General Public License, version 2.

// Use of the PSSORT() macro requires additional macros defined:
// COMPAREEXPR(A,B): expression to compare element A with element B; true if element A is smaller than B.
// SWAPFUNC(A,B): swap element A with element B

# define OHANA_DOWNHEAP(COMPARE, SWAPFUNC, K) { \
  unsigned long k = K; /* Local version of i in main loop */ \
  while (k <= last / 2) {  \
    unsigned long j = 2 * k;  \
    if ((j < last) && COMPARE(j, j + 1)) {  \
      j++;  \
    }  \
    if (COMPARE(k, j)) {  \
      SWAPFUNC(j, k);  \
    } else {  \
      break;  \
    }  \
    k = j;  \
  } \
}

# define OHANA_SORT(NVALUE, COMPARE, SWAPFUNC) { \
  unsigned long last = NVALUE - 1; \
  unsigned long i = last / 2 + 1; \
  if (NVALUE > 1) { \
    do { \
      i--;  \
      OHANA_DOWNHEAP (COMPARE, SWAPFUNC, i); \
    } while (i > 0); \
    while (last > 0) { \
      SWAPFUNC(0, last); /* Swap elements */ \
      /* Process the heap */ \
      last--; \
      OHANA_DOWNHEAP (COMPARE, SWAPFUNC, 0); \
    } \
  } \
}

// pre-defined function versions
void dsort (double *value, int N);
void fsort (float *value, int N);
void isort (int *value, int N);
void llsort (long long int *value, int N);

void dsortpair (double *X, double *Y, int N);
void fsortpair (float *X, float *Y, int N);
void isortpair (int *X, int *Y, int N);

void llsortpair (off_t *X, off_t *Y, off_t N);

void dsortthree (double *X, double *Y, double *Z, int N);
void fsortthree (float *X, float *Y, float *Z, int N);

void dsortfour (double *X, double *Y, double *Z, double *W, int N);
void fsortfour (float *X, float *Y, float *Z, float *W, int N);
void isortfour (int *X, int *Y, int *Z, int *W, int N);

void dsort_indexonly (double *X, off_t *S, off_t N);
void dsort_int_indexonly (double *X, int *S, int N);

#endif
