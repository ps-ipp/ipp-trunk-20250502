# include "psphotInternal.h"

bool psphotMakeFluxScale (psImage *image, psMetadata *recipe, pmPSF *psf) {

    bool status = false;

    psTimerStart ("psphot.fluxscale");

    int Nx = psMetadataLookupS32(&status, recipe, "PSF.FLUXSCALE.NX");
    int Ny = psMetadataLookupS32(&status, recipe, "PSF.FLUXSCALE.NY");

    // stats structure for use by ApTrend : XXX make parameters user setable
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);

    pmTrend2D *trend = pmTrend2DAlloc (PM_TREND_MAP, image, Nx, Ny, stats);

    psVector *xPts = psVectorAlloc (Nx*Ny, PS_TYPE_F32);
    psVector *yPts = psVectorAlloc (Nx*Ny, PS_TYPE_F32);
    psVector *fPts = psVectorAlloc (Nx*Ny, PS_TYPE_F32);

    int Npts = 0;
    bool success = true;                // Function succeeded?

    // generate a set of test normalized PSF fluxes filling the grid
    for (int ix = 0; ix < Nx; ix++) {
        for (int iy = 0; iy < Ny; iy++) {

            float x = psImageBinningGetFineX (trend->map->binning, ix + 0.5);
            float y = psImageBinningGetFineY (trend->map->binning, iy + 0.5);

	    float fitSum = 0;

            // create normalized model object at xc,yc
            pmModel *model = pmModelFromPSFforXY (psf, x, y, 1.0);
            if (!model) {
                psTrace ("psphot", 3, "Unable to generate model for grid point %d,%d", ix, iy);
		fitSum = NAN;
            } else {
		// measure the fitMag for this model
		fitSum = model->class->modelFlux (model->params);
	    }
	    if (fitSum < 1.e-6) continue;
	    if (!isfinite(fitSum)) continue;

            xPts->data.F32[Npts] = x;
            yPts->data.F32[Npts] = y;
            fPts->data.F32[Npts] = fitSum;
            Npts ++;
            assert (Npts <= xPts->nalloc);
            psFree (model);
        }
    }
    xPts->n = Npts;
    yPts->n = Npts;
    fPts->n = Npts;

    // XXX we should allow the spatial sampling to decrease if the fit fails
    bool goodFit = false;
    if (!pmTrend2DFit (&goodFit, trend, NULL, 0xff, xPts, yPts, fPts, NULL)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to fit trend");
        success = false;
        goto DONE;
    }
    if (!goodFit) {
        psError(PS_ERR_UNKNOWN, false, "poor fit to flux-scale trend");
        success = false;
        goto DONE;
    }
    if (trend->mode == PM_TREND_MAP) {
	// p_psImagePrint (2, trend->map->map, "FluxScale Before"); // XXX TEST:
	psImageMapRepair (trend->map->map);
	// p_psImagePrint (2, trend->map->map, "FluxScale After"); // XXX TEST:
    }

    // XXX do something useful to measure residual statistics

    psf->FluxScale = psMemIncrRefCounter(trend);

 DONE:
    psFree(xPts);
    psFree(yPts);
    psFree(fPts);
    psFree(stats);
    psFree(trend);

    psLogMsg ("psphot", PS_LOG_MINUTIA, "built flux scale: %f sec\n", psTimerMark ("psphot.fluxscale"));

    return success;
}
