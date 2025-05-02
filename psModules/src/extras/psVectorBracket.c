/** @file  psVectorBracket.c
 *
 *  Vector Bracketing tools
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-12-10 18:27:26 $
 *
 *  Copyright 2006 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "psVectorBracket.h"

// return the last entry below or first entry above key value
int psVectorBracket(const psVector *index, psF32 key, bool above)
{

    int N;
    int Nlo = 0;
    int Nhi = index->n;

    if (above) {
        while (Nhi - Nlo > 10) {
            N = 0.5*(Nlo + Nhi);
            if (index->data.F32[N] > key) {
                Nhi = N;
            } else {
                Nlo = N - 1;
            }
        }
        // at this point, index[Nhi] > key >= index[Nlo]
        N = Nlo;
        while ((index->data.F32[N] <= key) && (N < Nhi)) {
            N++;
        }
        return (N);
    }
    while (Nhi - Nlo > 10) {
        N = 0.5*(Nlo + Nhi);
        if (index->data.F32[N] < key) {
            Nlo = N;
        } else {
            Nhi = N + 1;
        }
    }
    N = (Nhi >= index->n) ? Nhi - 1 : Nhi;
    // at this point, index[N] >= key > index[Nlo]
    while ((index->data.F32[N] >= key) && (N > Nlo)) {
        N--;
    }
    return (N);
}

// return the last entry below or first entry above key value (reverse sorted input)
int psVectorBracketDescend(const psVector *index, psF32 key, bool above)
{

    int N;
    int Nhi = 0;
    int Nlo = index->n;

    if (above) {
        while (Nlo - Nhi > 10) {
            N = 0.5*(Nhi + Nlo);
            if (index->data.F32[N] < key) {
                Nlo = N;
            } else {
                Nhi = N - 1;
            }
        }
        // at this point, index[Nhi] >= key > index[Nlo]
        N = Nhi;
        while ((index->data.F32[N] >= key) && (N < Nlo)) {
            N++;
        }
        return (N);
    }
    while (Nlo - Nhi > 10) {
        N = 0.5*(Nhi + Nlo);
        if (index->data.F32[N] > key) {
            Nhi = N;
        } else {
            Nlo = N + 1;
        }
    }
    // at this point, index[Nhi] > key >= index[Nlo]
    N = Nlo;
    while ((index->data.F32[N] <= key) && (N > Nhi)) {
        N--;
    }
    return (N);
}

// search for the bins bounding key in index, interpolate the corresponding values
psF32 psVectorInterpolate(const psVector *index, const psVector *value, psF32 key)
{

    int n0 = 0;
    int n1 = 0;

    // extrapolate at ends
    if (key < index->data.F32[0]) {
        n0 = 0;
        n1 = 1;
    }

    // extrapolate at ends
    if (key > index->data.F32[index->n-1]) {
        n0 = index->n-2;
        n1 = index->n-1;
    }

    if (n1 == 0) {
        n0 = psVectorBracket (index, key, FALSE);
        n1 = n0 + 1;
    }

    if (n0 == index->n-1) {
        n1 = n0;
        n0 = n1 - 1;
    }

    float dy = value->data.F32[n1] - value->data.F32[n0];
    float dx = index->data.F32[n1] - index->data.F32[n0];
    float dX = key - index->data.F32[n0];
    float dY = dX * (dy/dx);
    float result = value->data.F32[n0] + dY;
    return result;
}

