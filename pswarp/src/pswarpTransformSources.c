/** @file pswarpTransformSources.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

#define SOURCE_ARRAY_BUFFER 100         ///< Size to grow the array of sources at a time

/**
 * NOTE: in this function, the coordinates are transformed from the OUTPUT to the INPUT
 */
bool pswarpTransformSources(pmReadout *output, pmReadout *input, pmConfig *config)
{

    // find the output pixel range
    int minX, minY, maxX, maxY;
    pswarpMatchRange (&minX, &minY, &maxX, &maxY, input, output);

    // Get warp parameters
    bool mdok;
    int nGridX = psMetadataLookupS32(NULL, config->arguments, "GRID.NX");
    int nGridY = psMetadataLookupS32(NULL, config->arguments, "GRID.NY");

    // Transform sources
    pmDetections *inDetections = psMetadataLookupPtr(&mdok, input->analysis, "PSPHOT.DETECTIONS"); // Sources in source
    if (!inDetections) return true;
    psArray *inSources = inDetections->allSources;
    if (!inSources) return true;

    pswarpMapGrid *sourceGrid = pswarpMapGridFromImage(output, input, nGridX, nGridY); // Grid for sources

    pmDetections *outDetections = psMemIncrRefCounter(psMetadataLookupPtr(&mdok, output->analysis, "PSPHOT.DETECTIONS")); // Target sources
    if (!outDetections) {
        outDetections = pmDetectionsAlloc();
	psMetadataAddPtr(output->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_DATA_ARRAY , "Warped sources", outDetections);
    }
    psArray *outSources = outDetections->allSources;
    if (!outSources) {
        outDetections->allSources = psArrayAllocEmpty(SOURCE_ARRAY_BUFFER);
        outSources = outDetections->allSources;
    }

    // XXX it is probably not necessary to use the locally linear transformations we should
    // be using the full astrometry: since there are 100 - 1000 fewer sources than pixels,
    // this does not cost us so much time.
    for (int i = 0; i < inSources->n; i++) {
        pmSource *source = inSources->data[i]; ///< Source of interest
        pmModel *model = source->modelPSF; ///< Model for this source
        float xIn, yIn;             ///< Coordinates of source
        xIn = model->params->data.F32[PM_PAR_XPOS] - input->image->col0;
        yIn = model->params->data.F32[PM_PAR_YPOS] - input->image->row0;

        int xGrid, yGrid;           ///< Grid coordinates for local map
        if (!pswarpMapGridSetGrid(sourceGrid, xIn, yIn, &xGrid, &yGrid)) {
            psError(psErrorCodeLast(), false, "Unable to get grid coordinates for source at %f,%f\n",
                    xIn, yIn);
            psFree(outDetections);
            psFree(sourceGrid);
            return false;
        }
        if (xGrid < 0 || xGrid >= sourceGrid->nXpts || yGrid < 0 || yGrid >= sourceGrid->nYpts) {
            // It's not even on the grid
            // XXX how can this happen?
            continue;
        }

        pswarpMap *map = sourceGrid->maps[xGrid][yGrid]; ///< Locally linear transformation
        double xOut, yOut;          ///< Output coordinates
        if (!pswarpMapApply(&xOut, &yOut, map, xIn, yIn)) {
            psError(psErrorCodeLast(), false, "Unable to transform coordinates for source at %f,%f\n",
                    xIn, yIn);
            psFree(outDetections);
            psFree(sourceGrid);
            return false;
        }
        xOut += output->image->col0;
        yOut += output->image->row0;
        if (xOut < minX || xOut > maxX || yOut < minY || yOut > maxY) {
            // It's not in the output image
            continue;
        }

        // Generate the new source in the output frame
        //
        // Magnitudes will be off if there's any change in scale, but for our purposes (mainly x,y and
        // relative flux) that's OK.
        pmSource *new = pmSourceAlloc(); ///< New source
        new->peak = pmPeakAlloc(xOut, yOut, source->peak->detValue, PM_PEAK_LONE);
        new->peak->rawFlux = source->peak->rawFlux;
        new->peak->smoothFlux = source->peak->smoothFlux;
        new->type = source->type;
        new->mode = source->mode;
        new->psfMag = source->psfMag;
        new->extMag = source->extMag;
        new->psfMagErr = source->psfMagErr;
        new->apMag = source->apMag;
        new->pixWeightNotBad = source->pixWeightNotBad;
        new->pixWeightNotPoor = source->pixWeightNotPoor;
        new->psfChisq = source->psfChisq;
        new->crNsigma = source->crNsigma;
        new->extNsigma = source->extNsigma;
        new->sky = source->sky;
        new->skyErr = source->skyErr;
        new->apRadius = source->apRadius;

        new->modelPSF = pmModelAlloc(source->modelPSF->type);
        new->modelPSF->params->data.F32[PM_PAR_I0]=
          (isfinite(new->psfMag) ? pow(10.0, -0.4*new->psfMag) : NAN);
        new->modelPSF->dparams->data.F32[PM_PAR_I0]=
          (isfinite(new->psfMag) ? new->psfMagErr*pow(10.0, -0.4*new->psfMag) : NAN);

#if 0
        // XXX Note that this will not set the correct axes
	psEllipseAxes axes = pmPSF_ModelToAxes(source->modelPSF->params->data.F32, 20.0, source->modelPSF->class->useReff);
        pmPSF_AxesToModel(new->modelPSF->params->data.F32, axes, new->modelPSF->class->useReff);
#endif

        // Propagate the position erorrs
        float dxIn, dyIn;           // Errors in input coordinates
        dxIn = model->dparams->data.F32[PM_PAR_XPOS];
        dyIn = model->dparams->data.F32[PM_PAR_YPOS];

        float dxOut, dyOut;         // Errors in output coordinates
        dxOut = sqrt(PS_SQR(map->Xx * dxIn) + PS_SQR(map->Xy * dyIn));
        dyOut = sqrt(PS_SQR(map->Yx * dxIn) + PS_SQR(map->Yy * dyIn));

        new->modelPSF->params->data.F32[PM_PAR_XPOS] = xOut;
        new->modelPSF->params->data.F32[PM_PAR_YPOS] = yOut;
        new->modelPSF->dparams->data.F32[PM_PAR_XPOS] = dxOut;
        new->modelPSF->dparams->data.F32[PM_PAR_YPOS] = dyOut;

        new->modelPSF->chisq = model->chisq;
        new->modelPSF->chisqNorm = model->chisqNorm;
        new->modelPSF->mag = model->mag;
        new->modelPSF->nDOF = model->nDOF;
        new->modelPSF->nIter = model->nIter;
        new->modelPSF->flags = model->flags;
        new->modelPSF->fitRadius = model->fitRadius;

        psArrayAdd(outSources, SOURCE_ARRAY_BUFFER, new);
        psFree(new);                // Drop reference
    }
    psFree(sourceGrid);
    psFree(outDetections);             // Drop reference
    return true;
}

