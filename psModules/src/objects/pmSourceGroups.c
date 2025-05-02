#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmSourceGroups.h"

// the strategy here is to divide the image into 2x2 blocks of cells and cycle through
// the four discontiguous sets of cells, threading all within a set and blocking between
// sets

// we divide the image region into 2*2 blocks of size Nx*Ny, the image will have
// Cx*Cy blocks so that (2Nx)Cx = numCols, (2Ny)Cy = numRows.  We want to choose Cx and
// Cy so that (2Nx)Cx * (2Ny)Cy = 4 * NFILL * nThreads -- each of the four sets of cells
// has enough cells to allow NFILL cells for each thread (to better distribute heavy and
// light load cells

// the array runs from readout->image->col0 to readout->image->col0 + readout->image->numCols


static void sourceGroupsFree(pmSourceGroups *groups)
{
    psFree(groups->groups);
    return;
}


bool pmSourceGroupsCoordToCell(int *group, // Group number, returned
                               int *cell,  // Cell number, returned
                               float x, float y, // Coordinates
                               const pmSourceGroups *groups // Groups
                               )
{
    // XXX need to handle edges
    int ix = (x - groups->Xo) / (2 * groups->Nx);
    ix = PS_MAX(0, PS_MIN(ix, groups->Cx - 1));

    int iy = (y - groups->Yo) / (2 * groups->Ny);
    iy = PS_MAX(0, PS_MIN(iy, groups->Cy - 1));

    int jx = (((int)(x - groups->Xo)) % (2 * groups->Nx)) / groups->Nx;
    jx = PS_MAX(0, PS_MIN(jx, groups->Nx - 1));

    int jy = (((int)(y - groups->Yo)) % (2 * groups->Ny)) / groups->Ny;
    jy = PS_MAX(0, PS_MIN(jy, groups->Ny - 1));

    *group = jx + 2 * jy;
    *cell  = ix + groups->Cx * iy;

    return true;
}


pmSourceGroups *pmSourceGroupsAlloc(const pmReadout *readout, int nThreads)
{
    pmSourceGroups *groups = psAlloc(sizeof(pmSourceGroups)); // Groups, to return
    psMemSetDeallocator(groups, (psFreeFunc)sourceGroupsFree);

    groups->Xo = readout->image->col0;
    groups->Yo = readout->image->row0;

    if (nThreads == 0 || nThreads == 1) {
        // Trivial case
        groups->Cx = groups->Cy = 1;
        groups->Nx = readout->image->numCols;
        groups->Ny = readout->image->numRows;
        groups->groups = psArrayAlloc(1);
        return groups;
    }

    int nCells = nThreads * 2*2;        // number of cells in a single set
    int C = sqrt(nCells) + 0.5;
    int Cx = 1, Cy = 1;                 // Number of cells in x and y

    // we need to assign Cx and Cy based on the dimensionality of the image
    // crude way to find most evenly balanced factors of nCells:
    for (int i = C; i >= 1; i--) {
        int C1 = nCells / C;
        int C2 = nCells / C1;
        if (C1*C2 != nCells) continue;

        if (readout->image->numRows > readout->image->numCols) {
            Cx = PS_MAX(C1, C2);
            Cy = PS_MIN(C1, C2);
        } else {
            Cx = PS_MAX(C1, C2);
            Cy = PS_MIN(C1, C2);
        }
    }

    groups->Cx = Cx;
    groups->Cy = Cy;
    groups->Nx = readout->image->numCols / (Cx*2);
    groups->Ny = readout->image->numRows / (Cy*2);
    groups->groups = psArrayAlloc(4);

    return groups;
}



pmSourceGroups *pmSourceGroupsFromSources(const pmReadout *readout, const psArray *sources, int nThreads)
{
    PM_ASSERT_READOUT_NON_NULL(readout, NULL);
    PM_ASSERT_READOUT_IMAGE(readout, NULL);
    PS_ASSERT_ARRAY_NON_NULL(sources, NULL);
    PS_ASSERT_INT_NONNEGATIVE(nThreads, NULL);

    pmSourceGroups *groups = pmSourceGroupsAlloc(readout, nThreads);

    int numSources = sources->n;        // Number of sources
    psVector *x = psVectorAlloc(numSources, PS_TYPE_F32), *y = psVectorAlloc(numSources, PS_TYPE_F32);
    for (int i = 0; i < numSources; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        x->data.F32[i] = source->peak->xf;
        y->data.F32[i] = source->peak->yf;
    }

    if (!pmSourceGroupsPopulate(groups, x, y)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to populate source groups");
        psFree(x);
        psFree(y);
        psFree(groups);
        return NULL;
    }

    psFree(x);
    psFree(y);
    return groups;
}

pmSourceGroups *pmSourceGroupsFromVectors(const pmReadout *readout, const psVector *x,
                                          const psVector *y, int nThreads)
{
    PM_ASSERT_READOUT_NON_NULL(readout, NULL);
    PM_ASSERT_READOUT_IMAGE(readout, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, NULL);
    PS_ASSERT_INT_NONNEGATIVE(nThreads, NULL);

    pmSourceGroups *groups = pmSourceGroupsAlloc(readout, nThreads);
    if (!pmSourceGroupsPopulate(groups, x, y)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to populate source groups");
        psFree(groups);
        return NULL;
    }

    return groups;
}


bool pmSourceGroupsPopulate(pmSourceGroups *groups, const psVector *x, const psVector *y)
{
    PS_ASSERT_PTR_NON_NULL(groups, false);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, NULL);

    int numSources = x->n;                // Number of sources

    // Populate the groups
    long total = groups->Cx * groups->Cy; // Total size
    for (int i = 0; i < groups->groups->n; i++) {
        psArray *cells = psArrayAlloc(total); // Cells within a group
        groups->groups->data[i] = cells;
        for (int j = 0; j < cells->n; j++) {
            cells->data[j] = psVectorAllocEmpty(numSources / total, PS_TYPE_S32);
        }
    }

    // Populate the cells
    if (total == 1) {
        // Trivial case: Cx == Cy == 1
        psArray *cells = groups->groups->data[0]; // Cell
        psVector *cellSources = cells->data[0];   // Indices of sources for cell
        for (int i = 0; i < numSources; i++) {
            cellSources->data.S32[i] = i;
        }
	cellSources->n = numSources;
    } else {
        for (int i = 0; i < numSources; i++) {
            int group = 0, cell = 0;        // Group and cell index for source
            pmSourceGroupsCoordToCell(&group, &cell, x->data.F32[i], y->data.F32[i], groups);

            psArray *cells = groups->groups->data[group]; // Cells for group
            psVector *cellSources = cells->data[cell];    // Indices of sources for cell within group
            psVectorAppend(cellSources, i);
        }
    }

    return groups;
}
