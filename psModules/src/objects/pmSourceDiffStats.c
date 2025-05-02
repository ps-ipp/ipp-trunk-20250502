/** @file  pmSourceDiffStats.c
 *
 *  Functions defining the pmSourceDiffStats structure and associated measurements

 *  @author EAM, IfA
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-03 20:59:16 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include "pmSourceDiffStats.h"

/** initialization function for a pmSourceDiffStats object
 **/
void pmSourceDiffStatsInit(pmSourceDiffStats *diffStats)
{
    diffStats->fRatio = NAN;
    diffStats->nRatioBad = NAN;
    diffStats->nRatioMask = NAN;
    diffStats->nRatioAll = NAN;
    diffStats->nGood = 0;

    diffStats->SNp = NAN;
    diffStats->SNm = NAN;
    diffStats->Rp = NAN;
    diffStats->Rm = NAN;
}

/******************************************************************************
pmSourceDiffStatsAlloc(): Allocate the pmSourceDiffStats structure and initialize the members
to zero.
*****************************************************************************/
pmSourceDiffStats *pmSourceDiffStatsAlloc(void)
{
    pmSourceDiffStats *tmp = (pmSourceDiffStats *) psAlloc(sizeof(pmSourceDiffStats));
    pmSourceDiffStatsInit(tmp);
    return(tmp);
}
