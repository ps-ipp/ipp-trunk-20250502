/** @file  pmAstrometryDistortion.c
*
*  @brief This file defines the basic types for measuring the focal-plane distortion.
*
*  @ingroup AstroImage
*
*  @author EAM, IfA
*
*  @version $Revision: 1.23 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-01-27 06:39:38 $
*
*  Copyright 2006 Institute for Astronomy, University of Hawaii
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/

#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAExtent.h"
#include "pmAstrometryObjects.h"
#include "pmAstrometryRegions.h"
#include "pmAstrometryDistortion.h"
#include "pmAstrometryUtils.h"
#include "pmKapaPlots.h"

static void pmAstromGradientFree (pmAstromGradient *grad)
{

    if (grad == NULL)
        return;

    return;
}

pmAstromGradient *pmAstromGradientAlloc (void)
{

    pmAstromGradient *gradient = psAlloc (sizeof(pmAstromGradient));
    psMemSetDeallocator(gradient, (psFreeFunc) pmAstromGradientFree);

    return (gradient);
}

psArray *pmAstromMeasureGradients(psArray *gradients, psArray *rawstars, psArray *refstars, psArray *matches, psRegion *region, int Nx, int Ny)
{

    if (gradients == NULL) {
        gradients = psArrayAllocEmpty (100);
    }

    // NOTE: region specifies the FP region in pixels covered by the chip (NOT in FP units)
    // determine range
    int DX = (region->x1 - region->x0) / Nx;
    int DY = (region->y1 - region->y0) / Ny;

    psPolynomial2D *local = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, 1, 1);
    local->coeffMask[1][1] = PS_POLY_MASK_SET;

    // measure gradient for fractional chip regions
    for (int nx = 0; nx < Nx; nx++) {
        for (int ny = 0; ny < Ny; ny++) {
            int Xmin = nx*DX;
            int Xmax = Xmin + DX;
            int Ymin = ny*DY;
            int Ymax = Ymin + DY;

            psStats *stats = NULL;
            psVector *mask = NULL;
            pmAstromGradient *grad = NULL;

            psVector *L  = psVectorAllocEmpty (100, PS_TYPE_F32);
            psVector *M  = psVectorAllocEmpty (100, PS_TYPE_F32);
            psVector *dP = psVectorAllocEmpty (100, PS_TYPE_F32);
            psVector *dQ = psVectorAllocEmpty (100, PS_TYPE_F32);

            // XXX this is a bit inefficient: first sorting by X or Y could speed this up.
            // XXX or assigning to a segment in a single pass first
            // select the stars within this chip region
            int Npts = 0;
            for (int i = 0; i < matches->n; i++) {

                pmAstromMatch *match = matches->data[i];

                pmAstromObj *raw = rawstars->data[match->raw];

                if (raw->chip->x < Xmin) continue;
                if (raw->chip->x > Xmax) continue;
                if (raw->chip->y < Ymin) continue;
                if (raw->chip->y > Ymax) continue;

                pmAstromObj *ref = refstars->data[match->ref];

                L->data.F32[Npts] = raw->FP->x;
                M->data.F32[Npts] = raw->FP->y;

                // P,Q = L,M + terms of order epsilon.
                // measuring the gradient constrains thos terms
                dP->data.F32[Npts] = ref->TP->x - raw->FP->x;
                dQ->data.F32[Npts] = ref->TP->y - raw->FP->y;

                psVectorExtend (L, 100, 1);
                psVectorExtend (M, 100, 1);
                psVectorExtend (dP, 100, 1);
                psVectorExtend (dQ, 100, 1);
                Npts++;
            }

            psTrace ("psModules.astrom", 4, "Npts: %d (%d,%d) : (%d - %d),(%d - %d)\n", Npts, nx, ny, Xmin, Xmax, Ymin, Ymax);

            if (Npts < 5)
                goto skip;

            // stats structure and mask for use in measuring the clipping statistic
            // this analysis has too few data points to use the robust median method
            stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
            mask = psVectorAlloc (Npts, PS_TYPE_VECTOR_MASK);
            psVectorInit (mask, 0);

            grad = pmAstromGradientAlloc ();

            // XXX psTraceSetLevel("psLib.math.psVectorClipFitPolynomial2D", 7);

            // fit the collection of positions and offsets with a local 1st order gradient
            // apply 3hi/3lo sigma clipping to the fitted data values
            // the mask is used to mark the points which pass / fail the fit
            if (!psVectorClipFitPolynomial2D (local, stats, mask, 0xff, dP, NULL, L, M)) {
                goto skip;
            }

            grad->dTPdL.x = local->coeff[1][0];
            grad->dTPdM.x = local->coeff[0][1];

            // XXX psTraceSetLevel("psLib.math.psVectorClipFitPolynomial2D", 0);

            // fit the collection of positions and offsets with a local 1st order gradient
            // apply 3hi/3lo sigma clipping to the fitted data values
            // the mask is used to mark the points which pass / fail the fit
            if (!psVectorClipFitPolynomial2D (local, stats, mask, 0xff, dQ, NULL, L, M)) {
                goto skip;
            }

            grad->dTPdL.y = local->coeff[1][0];
            grad->dTPdM.y = local->coeff[0][1];

            // also measure the L and M median positions as a representative coordinate
            if (!psVectorStats (stats, L, NULL, NULL, 0)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		goto skip;
	    }
            grad->FP.x = stats->sampleMedian;

            if (!psVectorStats (stats, M, NULL, NULL, 0)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		goto skip;
	    }
            grad->FP.y = stats->sampleMedian;

            psArrayAdd (gradients, 100, grad);

skip:
            psFree (grad);
            psFree (stats);
            psFree (mask);
            psFree (L);
            psFree (M);
            psFree (dP);
            psFree (dQ);
        }
    }
    psFree (local);
    return gradients;
}

bool pmAstromFitDistortion(pmFPA *fpa, psArray *gradients, double pixelScale)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_ARRAY_NON_NULL(gradients, false);

    int ExtraOrders = pmAstrometryGetExtraOrders();

    psPolynomial2D *localX = NULL;
    psPolynomial2D *localY = NULL;

    // assign the gradient elements to psVectors for fitting
    psVector *dPdL = psVectorAlloc (gradients->n, PS_TYPE_F32);
    psVector *dQdL = psVectorAlloc (gradients->n, PS_TYPE_F32);
    psVector *dPdM = psVectorAlloc (gradients->n, PS_TYPE_F32);
    psVector *dQdM = psVectorAlloc (gradients->n, PS_TYPE_F32);
    psVector *L = psVectorAlloc (gradients->n, PS_TYPE_F32);
    psVector *M = psVectorAlloc (gradients->n, PS_TYPE_F32);

    for (int i = 0; i < gradients->n; i++) {

        pmAstromGradient *grad = gradients->data[i];

        dPdL->data.F32[i] = grad->dTPdL.x;
        dQdL->data.F32[i] = grad->dTPdL.y;

        dPdM->data.F32[i] = grad->dTPdM.x;
        dQdM->data.F32[i] = grad->dTPdM.y;

        L->data.F32[i] = grad->FP.x;
        M->data.F32[i] = grad->FP.y;
    }

    // mask and stats structure for measuring the clipping statistic
    // this analysis has too few data points to use the robust median method
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    psVector *mask = psVectorAlloc (gradients->n, PS_TYPE_VECTOR_MASK);
    psVectorInit (mask, 0);

    // the order of the gradient fits need to be 1 less than the distortion term
    // determine the gradient order(s) from the fpa->toTP structure
    localX = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, fpa->toTPA->x->nX-1, fpa->toTPA->x->nY);
    localY = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, fpa->toTPA->x->nX,   fpa->toTPA->x->nY-1);

    // set masks based on fpa->toTPA
    for (int i = 0; i <= fpa->toTPA->x->nX; i++) {
        for (int j = 0; j <= fpa->toTPA->x->nY; j++) {
            if ((i > 0) && (i <= fpa->toTPA->x->nX)) {
                localX->coeffMask[i-1][j] = fpa->toTPA->x->coeffMask[i][j];
            }
            if ((j > 0) && (j <= fpa->toTPA->x->nY)) {
                localY->coeffMask[i][j-1] = fpa->toTPA->x->coeffMask[i][j];
            }
        }
    }

    // fit the local gradients in each direction
    if (!psVectorClipFitPolynomial2D (localX, stats, mask, 0xff, dPdL, NULL, L, M)) {
	// this failure has raised an error.  the return will also raise
	// errors, but these are finally cleared (and bad data quality is set) in
	// psastroAnalysis.c:143
        psLogMsg ("psastro", 3, "failed to fit x-dir gradient\n");
        psFree (localX);
        psFree (localY);
        goto escape;
    }

    if (!psVectorClipFitPolynomial2D (localY, stats, mask, 0xff, dPdM, NULL, L, M)) {
	// this failure has raised an error.  the return will also raise
	// errors, but these are finally cleared (and bad data quality is set) in
	// psastroAnalysis.c:143
        psLogMsg ("psastro", 3, "failed to fit y-dir gradient\n");
        psFree (localX);
        psFree (localY);
        goto escape;
    }

    // update fpa->toTP distortion terms
    fpa->toTPA->x->coeff[0][0] = 0;
    for (int i = 1; i <= fpa->toTPA->x->nX; i++) {
        if (fpa->toTPA->x->coeffMask[i][0] & PS_POLY_MASK_SET) {
            continue;
        }
        fpa->toTPA->x->coeff[i][0] = localX->coeff[i-1][0] / i;
    }
    for (int j = 1; j <= fpa->toTPA->x->nY; j++) {
        if (fpa->toTPA->x->coeffMask[0][j] & PS_POLY_MASK_SET) {
            continue;
        }
        fpa->toTPA->x->coeff[0][j] = localY->coeff[0][j-1] / j;
    }
    for (int i = 1; i <= fpa->toTPA->x->nX; i++) {
        for (int j = 1; j <= fpa->toTPA->x->nY; j++) {
            if (fpa->toTPA->x->coeffMask[i][j] & PS_POLY_MASK_SET) {
                continue;
            }
            fpa->toTPA->x->coeff[i][j] = 0.5*(localX->coeff[i-1][j] / i + localY->coeff[i][j-1] / j);
        }
    }
    fpa->toTPA->x->coeff[1][0] += 1.0;
    psFree (localX);
    psFree (localY);

    // the order of the gradient fits need to be 1 less than the distortion term
    // determine the gradient order(s) from the fpa->toTP structure
    localX = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, fpa->toTPA->y->nX-1, fpa->toTPA->y->nY);
    localY = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, fpa->toTPA->y->nX,   fpa->toTPA->y->nY-1);

    // set masks based on fpa->toTP
    for (int i = 0; i < fpa->toTPA->y->nX; i++) {
        for (int j = 0; j < fpa->toTPA->y->nY; j++) {
            if ((i > 0) && (i <= fpa->toTPA->y->nX)) {
                localX->coeffMask[i-1][j] = fpa->toTPA->y->coeffMask[i][j];
            }
            if ((j > 0) && (j <= fpa->toTPA->y->nY)) {
                localY->coeffMask[i][j-1] = fpa->toTPA->y->coeffMask[i][j];
            }
        }
    }

    // fit the local gradients in each direction
    psVectorClipFitPolynomial2D (localX, stats, mask, 0xff, dQdL, NULL, L, M);
    psVectorClipFitPolynomial2D (localY, stats, mask, 0xff, dQdM, NULL, L, M);

    // update fpa->toTP distortion terms
    fpa->toTPA->y->coeff[0][0] = 0;
    for (int i = 1; i <= fpa->toTPA->y->nX; i++) {
        if (fpa->toTPA->y->coeffMask[i][0] & PS_POLY_MASK_SET) {
            continue;
        }
        fpa->toTPA->y->coeff[i][0] = localX->coeff[i-1][0] / i;
    }
    for (int j = 1; j <= fpa->toTPA->y->nY; j++) {
        if (fpa->toTPA->y->coeffMask[0][j] & PS_POLY_MASK_SET) {
            continue;
        }
        fpa->toTPA->y->coeff[0][j] = localY->coeff[0][j-1] / j;
    }
    for (int i = 1; i <= fpa->toTPA->y->nX; i++) {
        for (int j = 1; j <= fpa->toTPA->y->nY; j++) {
            if (fpa->toTPA->y->coeffMask[i][j] & PS_POLY_MASK_SET) {
                continue;
            }
            fpa->toTPA->y->coeff[i][j] = 0.5*(localX->coeff[i-1][j] / i + localY->coeff[i][j-1] / j);
        }
    }
    fpa->toTPA->y->coeff[0][1] += 1.0;
    psFree (localX);
    psFree (localY);

    // free unneeded structures
    psFree (dPdL);
    psFree (dPdM);
    psFree (dQdL);
    psFree (dQdM);
    psFree (L);
    psFree (M);
    psFree (stats);
    psFree (mask);

    // reset the fromTPA terms here. choose an appropriate region based on the dimensions of
    // the complete FPA
    psRegion *region = pmAstromFPAExtent (fpa);

    // as of r40806, psPlaneTransformInvert supplies the extra order (if non-linear)
    psFree (fpa->fromTPA);
    fpa->fromTPA = psPlaneTransformInvert(NULL, fpa->toTPA, *region, 50, ExtraOrders);
    psFree (region);

    if (fpa->fromTPA == NULL) {
        psError (PS_ERR_UNKNOWN, false, "failed to invert fpa->toTPA\n");
        return false;
    }

    return true;

escape:
    // free unneeded structures
    psFree (dPdL);
    psFree (dPdM);
    psFree (dQdL);
    psFree (dQdM);
    psFree (L);
    psFree (M);
    psFree (stats);
    psFree (mask);
    return false;
}
