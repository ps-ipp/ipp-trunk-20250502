/** @file  psImageBinning.c
 *
 *  @brief Functions to define the binning strategy and to perform image binning / unbinning
 *  (resampling).
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-13 00:54:26 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include "psMemory.h"
#include "psError.h"
#include "psAbort.h"
#include "psAssert.h"
#include "psRegion.h"
#include "psImage.h"
#include "psImageBinning.h"

static void psImageBinningFree(psImageBinning *binning) {
    return;
}

psImageBinning *psImageBinningAlloc(void) {
    psImageBinning *binning = (psImageBinning*)psAlloc(sizeof(psImageBinning));
    psMemSetDeallocator(binning, (psFreeFunc)psImageBinningFree);

    binning->nXfine = 0;
    binning->nYfine = 0;
    binning->nXruff = 0;
    binning->nYruff = 0;
    binning->nXbin = 0;
    binning->nYbin = 0;
    binning->nXoff = 0;
    binning->nYoff = 0;
    binning->nXskip = 0;
    binning->nYskip = 0;
    return binning;
}

bool psMemCheckBinning(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) psImageBinningFree);
}

void psImageBinningSetRuffSize(psImageBinning *binning, psImageBinningAlign align) {

    assert (binning->nXfine > 0);
    assert (binning->nYfine > 0);
    assert (binning->nXbin > 0);
    assert (binning->nYbin > 0);

    // force nXruff*nXbin > nXfine
    binning->nXruff = binning->nXfine / binning->nXbin;
    if (binning->nXfine % binning->nXbin) binning->nXruff ++;

    // force nYruff*nYbin > nYfine
    binning->nYruff = binning->nYfine / binning->nYbin;
    if (binning->nYfine % binning->nYbin) binning->nYruff ++;

    switch (align) {
      case PS_IMAGE_BINNING_LEFT:
        binning->nXoff = 0;
        binning->nYoff = 0;
        break;
      case PS_IMAGE_BINNING_CENTER:
        binning->nXoff = (binning->nXruff * binning->nXbin - binning->nXfine) / 2;
        binning->nYoff = (binning->nYruff * binning->nYbin - binning->nYfine) / 2;
        break;
      case PS_IMAGE_BINNING_RIGHT:
        binning->nXoff = (binning->nXruff * binning->nXbin - binning->nXfine);
        binning->nYoff = (binning->nYruff * binning->nYbin - binning->nYfine);
        break;
      default:
        psAbort ("programming error in %s: impossible case\n", __func__);
    }
    return;
}

void psImageBinningSetFineSize(psImageBinning *binning, psImageBinningAlign align) {

    binning->nXfine = binning->nXruff * binning->nXbin;
    binning->nYfine = binning->nYruff * binning->nYbin;
    return;
}

void psImageBinningSetSkip(psImageBinning *binning, const psImage *image)
{
    int col0, row0;                     // Offset for image
    if (image) {
        col0 = image->col0;
        row0 = image->row0;
    } else {
        col0 = row0 = 0;
    }
    psImageBinningSetSkipByOffset(binning, col0, row0);
    return;
}

void psImageBinningSetSkipByOffset(psImageBinning *binning, int col0, int row0) {

    binning->nXskip = col0 - binning->nXoff;
    binning->nYskip = row0 - binning->nYoff;
    return;
}

void psImageBinningSetScale(psImageBinning *binning, psImageBinningAlign align) {

    assert (binning->nXfine > 0);
    assert (binning->nYfine > 0);
    assert (binning->nXruff > 0);
    assert (binning->nYruff > 0);

    // force nXruff*nXbin > nXfine
    binning->nXbin = binning->nXfine / binning->nXruff;
    if (binning->nXfine % binning->nXruff) binning->nXbin ++;

    // force nYruff*nYbin > nYfine
    binning->nYbin = binning->nYfine / binning->nYruff;
    if (binning->nYfine % binning->nYruff) binning->nYbin ++;

    switch (align) {
      case PS_IMAGE_BINNING_LEFT:
        binning->nXoff = 0;
        binning->nYoff = 0;
        break;
      case PS_IMAGE_BINNING_CENTER:
        binning->nXoff = (binning->nXruff * binning->nXbin - binning->nXfine) / 2;
        binning->nYoff = (binning->nYruff * binning->nYbin - binning->nYfine) / 2;
        break;
      case PS_IMAGE_BINNING_RIGHT:
        binning->nXoff = (binning->nXruff * binning->nXbin - binning->nXfine);
        binning->nYoff = (binning->nYruff * binning->nYbin - binning->nYfine);
        break;
      default:
        psAbort ("programming error in %s: impossible case\n", __func__);
    }
    return;
}

psRegion psImageBinningSetFineRegion (psImageBinning *binning, psRegion ruffRegion) {

    psRegion fineRegion;

    fineRegion.x0 = ruffRegion.x0 * binning->nXbin + binning->nXskip;
    fineRegion.x1 = ruffRegion.x1 * binning->nXbin + binning->nXskip;
    fineRegion.y0 = ruffRegion.y0 * binning->nYbin + binning->nYskip;
    fineRegion.y1 = ruffRegion.y1 * binning->nYbin + binning->nYskip;
    return fineRegion;
}

psRegion psImageBinningSetRuffRegion (psImageBinning *binning, psRegion fineRegion) {

    psRegion ruffRegion;

    ruffRegion.x0 = (fineRegion.x0 - binning->nXskip) / binning->nXbin;
    ruffRegion.x1 = (fineRegion.x1 - binning->nXskip) / binning->nXbin;
    ruffRegion.y0 = (fineRegion.y0 - binning->nYskip) / binning->nYbin;
    ruffRegion.y1 = (fineRegion.y1 - binning->nYskip) / binning->nYbin;
    return ruffRegion;
}


// convert the fine coordinate to the ruff coordinate
double psImageBinningGetRuffX (const psImageBinning *binning, const double xFine) {

    PS_ASSERT_INT_POSITIVE(binning->nXbin, NAN);
    PS_ASSERT_INT_POSITIVE(binning->nYbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nXskip, binning->nXbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nYskip, binning->nYbin, NAN);

    double xRuff = (xFine - binning->nXskip)/binning->nXbin;
    return xRuff;
}
double psImageBinningGetRuffY (const psImageBinning *binning, const double yFine) {

    PS_ASSERT_INT_POSITIVE(binning->nXbin, NAN);
    PS_ASSERT_INT_POSITIVE(binning->nYbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nXskip, binning->nXbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nYskip, binning->nYbin, NAN);

    double yRuff = (yFine - binning->nYskip)/binning->nYbin;
    return yRuff;
}

// convert the ruff coordinate to the fine coordinate
double psImageBinningGetFineX (const psImageBinning *binning, const double xRuff) {

    PS_ASSERT_INT_POSITIVE(binning->nXbin, NAN);
    PS_ASSERT_INT_POSITIVE(binning->nYbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nXskip, binning->nXbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nYskip, binning->nYbin, NAN);

    double xFine = xRuff * binning->nXbin + binning->nXskip;
    return xFine;
}
double psImageBinningGetFineY (const psImageBinning *binning, const double yRuff) {

    PS_ASSERT_INT_POSITIVE(binning->nXbin, NAN);
    PS_ASSERT_INT_POSITIVE(binning->nYbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nXskip, binning->nXbin, NAN);
    // PS_ASSERT_INT_LESS_THAN_OR_EQUAL(binning->nYskip, binning->nYbin, NAN);

    double yFine = yRuff * binning->nYbin + binning->nYskip;
    return yFine;
}
