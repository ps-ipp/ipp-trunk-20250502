// Heap sort of the array
// Based on descriptions in Sedgewick "Algorithms in C"
//
// Copyright (C) 1999  Thomas Walter
// Copyright (C) 2007  Paul Price, Institute for Astronomy, University of Hawaii
//
// 18 February 2000: Modified for GSL by Brian Gough
// 29 November 2007: Modified for psLib by Paul Price
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

#ifndef PS_SORT_H
#define PS_SORT_H

// XXX since the key size is fixed (same number of bits) a Radix sort could be
// a big win here -- someone should benchmark this -JH

// The following sort code is based on gsl_heapsort from GSL-1.8 (www.gnu.org/software/gsl), file
// gsl-1.8/sort/sort.c, which is distributed under the GNU General Public License, version 2.


// Use of the PSSORT() and PSSELECT() macros require additional macros defined:
// COMPAREEXPR(A,B): expression to compare element A with element B; true if element A is smaller than B.
// SWAPFUNC(TYPE,A,B): swap element A with element B; the type is provided so that a temporary variable can
// be defined.

// Sort the heap
#define PSSORT_DOWNHEAP(COMPAREFUNC, SWAPFUNC, SWAPTYPE, K) { \
    unsigned long k = K; /* Local version of k --- so the higher-level version isn't modified */ \
    while (k <= N / 2) { \
        unsigned long j = 2 * k; \
        if (j < N && COMPAREFUNC(j, j + 1)) { \
            j++; \
        } \
        if (COMPAREFUNC(k, j)) { \
            SWAPFUNC(SWAPTYPE, j, k); \
        } else { \
            break; \
        } \
        k = j; \
    } \
}

// Driver for heap sort
// SIZE: Size of the array
// COMPAREEXPR: Macro with expression for comparison
// SWAPFUNC: Macro to swap elements
// SWAPTYPE: Type for swapping, passed to SWAPFUNC
#define PSSORT(SIZE, COMPAREEXPR, SWAPFUNC, SWAPTYPE) { \
    unsigned long N = (SIZE) - 1; /* Index of last element */ \
    unsigned long i = N / 2 + 1; /* Adding one to compensate for i-- below */ \
    do { \
        i--; \
        PSSORT_DOWNHEAP(COMPAREEXPR, SWAPFUNC, SWAPTYPE, i); \
    } while (i > 0); \
    while (N > 0) { \
        SWAPFUNC(SWAPTYPE, 0, N); /* Swap elements */ \
        /* Process the heap */ \
        N--; \
        PSSORT_DOWNHEAP(COMPAREEXPR, SWAPFUNC, SWAPTYPE, 0); \
    } \
}
// END of heap sort code from GSL



// The following algorithm for selection was provided by http://en.wikipedia.org/wiki/Selection_algorithm
// (version as of 21 May 2008, at 17:38), and implemented below as PSSELECT():
//
// function partition(list, left, right, pivotIndex)
//     pivotValue := list[pivotIndex]
//     swap list[pivotIndex] and list[right]  // Move pivot to end
//     storeIndex := left
//     for i from left to right-1
//         if list[i] < pivotValue
//             swap list[storeIndex] and list[i]
//             storeIndex := storeIndex + 1
//     swap list[right] and list[storeIndex]  // Move pivot to its final place
//     return storeIndex
//
// function select(list, k, left, right)
//     loop
//         select a pivot value list[pivotIndex]
//         pivotNewIndex := partition(list, left, right, pivotIndex)
//         if k = pivotNewIndex
//             return list[k]
//         else if k < pivotNewIndex
//             right := pivotNewIndex-1
//         else
//             left := pivotNewIndex+1


// Select the RANK-th element
// This macro reorders the array so that the RANK-th element is in the correct position
// SIZE: Size of the array
// RANK: RANK to move into the correct position
// COMPAREEXPR: Macro with expression for comparison
// SWAPFUNC: Macro to swap elements
// SWAPTYPE: Type for swapping, passed to SWAPFUNC
#define PSSELECT(SIZE, RANK, COMPAREEXPR, SWAPFUNC, SWAPTYPE) { \
    bool selectContinue = true;         /* Continue swapping? */ \
    long selectMax = SIZE - 1;           /* Maximum index */ \
    long selectMin = 0;                  /* Minimum index */ \
    while (selectContinue) { \
        long selectPivot = (selectMin + selectMax) >> 1; /* Pivot index */ \
        SWAPFUNC(SWAPTYPE, selectPivot, selectMax); /* Move pivot to end */ \
        long selectStore = selectMin;    /* Index of interest */ \
        for (long i = selectMin; i < selectMax; i++) { \
            if (COMPAREEXPR(i, selectMax)) { /* Note: comparing with the original pivot */ \
                SWAPFUNC(SWAPTYPE, selectStore, i); \
                selectStore++; \
            } \
        } \
        SWAPFUNC(SWAPTYPE, selectMax, selectStore); /* Move pivot to its final place */ \
        if (selectStore == RANK) { \
            selectContinue = false;     /* Done */ \
        } else if (selectStore > RANK) { \
            selectMax = selectStore - 1; \
        } else { \
            selectMin = selectStore + 1; \
        } \
    } \
}



#endif
