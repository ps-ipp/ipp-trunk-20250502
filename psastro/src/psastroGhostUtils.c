/** @file psastroGhostUtils.c
 *
 *  @brief Ghost support functions
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.19 $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

static void psastroGhostFree (psastroGhost *ghost) {

    if (ghost == NULL) return;

    psFree (ghost->srcFP);
    psFree (ghost->FP);
    psFree (ghost->chip);

    return;
}

psastroGhost *psastroGhostAlloc (void) {

    psastroGhost *ghost = (psastroGhost *) psAlloc(sizeof(psastroGhost));
    psMemSetDeallocator(ghost, (psFreeFunc) psastroGhostFree);

    ghost->srcFP = psPlaneAlloc();
    ghost->FP    = psPlaneAlloc();
    ghost->chip  = psPlaneAlloc();
    
    ghost->Mag   = 0.0;

    ghost->inner.major = 0.0;
    ghost->inner.minor = 0.0;
    ghost->inner.theta = 0.0;

    ghost->outer.major = 0.0;
    ghost->outer.minor = 0.0;
    ghost->outer.theta = 0.0;

    return ghost;
}

