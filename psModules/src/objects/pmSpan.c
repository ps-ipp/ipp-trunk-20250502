/* @file  pmSpan.c
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-01 00:00:17 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>
#include "pmSpan.h"

/*** functions to manipulate image spans ***/

static void spanFree(pmSpan *tmp) {
    return;
}

/*
 * pmSpanAlloc()
 */
pmSpan *pmSpanAlloc(int y, int x0, int x1)
{
    pmSpan *span = (pmSpan *)psAlloc(sizeof(pmSpan));
    psMemSetDeallocator(span, (psFreeFunc) spanFree);

    span->y = y;
    span->x0 = x0;
    span->x1 = x1;

    return(span);
}

bool pmSpanTest(const psPtr ptr)
{
    return (psMemGetDeallocator(ptr) == (psFreeFunc)spanFree);
}

//
// Sort pmSpans by y, then x0, then x1
//
int pmSpanSortByYX(const void **a, const void **b) {
    const pmSpan *sa = *(const pmSpan **)a;
    const pmSpan *sb = *(const pmSpan **)b;

    if (sa->y < sb->y) {
	return -1;
    } else if (sa->y == sb->y) {
	if (sa->x0 < sb->x0) {
	    return -1;
	} else if (sa->x0 == sb->x0) {
	    if (sa->x1 < sb->x1) {
		return -1;
	    } else if (sa->x1 == sb->x1) {
		return 0;
	    } else {
		return 1;
	    }
	} else {
	    return 1;
	}
    } else {
	return 1;
    }
}
