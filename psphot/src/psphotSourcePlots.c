# include "psphotInternal.h"

bool psphotSourcePlots (pmReadout *readout, psArray *sources, psMetadata *recipe) {

    bool status = false;
    psTimerStart ("psphot");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // the source images are written to an image 10x the size of a PSF object
    // float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    // PS_ASSERT (status, false);

    int DX = 21;
    int DY = 21;

    // examine PSF sources in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // counters to track the size of the image and area used in a row
    int dX = 0;                         // starting corner of next box
    int dY = 0;                         // height of row so far
    int NX = 20*DX;                     // full width of output image
    int NY = DY;                        // total height of output image so far

    // first, examine the PSF and SAT stars to set output image size:
    // - add stamp widths until we exceed output image width,
    // - then start a new row offset by max height
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];

        bool keep = false;
        keep |= (source->mode & PM_SOURCE_MODE_PSFSTAR);
        keep |= (source->mode & PM_SOURCE_MODE_SATSTAR);
        if (!keep) {
	    psTrace ("psphot", 4, "not plotting star %d: %x", i, source->mode);
	    continue;
	}

        // how does this subimage get placed into the output image?
        // DX = source->pixels->numCols
        // DY = source->pixels->numRows

        if (dX + DX > NX) {
            // too wide for the rest of this row
            if (dX == 0) {
                // alone on this row
                NY += DY;
                dX = 0;
                dY = 0;
            } else {
                // start the next row
                NY += dY;
                dX = DX;
                dY = DY;
            }
        } else {
            // extend this row
            dX += DX;
            dY = PS_MAX (dY, DY);
        }
    }

    if (NY == 0) {
	psWarning ("no PSF or SAT stars to plot? skipping.\n");
	return false;
    }

    // allocate output image
    psImage *outpos = psImageAlloc (NX, NY, PS_TYPE_F32);
    psImage *outsub = psImageAlloc (NX, NY, PS_TYPE_F32);

    psImageInit (outpos, 0.0);
    psImageInit (outsub, 0.0);

    int Xo = 0;                         // starting corner of next box
    int Yo = 0;                         // starting corner of next box
    dY = 0;                             // height of row so far

    int nPSF = 0;
    int nSAT = 0;
    int kapa = 0;                       // file descriptor for plotting routine

    // first, examine the PSF and SAT stars:
    // - generate radial plots (PS plots)
    // - create output image array
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];

        bool keep = false;
        if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
            nPSF ++;
            keep = true;
        }
        if (source->mode & PM_SOURCE_MODE_SATSTAR) {
            nSAT ++;
            keep = true;
        }
        if (!keep) continue;

        // how does this subimage get placed into the output image?
        // DX = source->pixels->numCols
        // DY = source->pixels->numRows

        if (Xo + DX > NX) {
            // too wide for the rest of this row
            if (Xo == 0) {
                // place source alone on this row
		bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
		if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
                psphotRadialPlot (&kapa, "radial.plots.ps", source);
                psphotMosaicSubimage (outpos, source, Xo, Yo, DX, DY, true);

		pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
                psphotMosaicSubimage (outsub, source, Xo, Yo, DX, DY, true);

		if (!subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

                Yo += DY;
                Xo = 0;
                dY = 0;
            } else {
                // start the next row
                Yo += dY;
                Xo = 0;

		bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
		if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
                psphotRadialPlot (&kapa, "radial.plots.ps", source);
                psphotMosaicSubimage (outpos, source, Xo, Yo, DX, DY, true);

		pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
                psphotMosaicSubimage (outsub, source, Xo, Yo, DX, DY, true);
		if (!subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

                Xo = DX;
                dY = DY;
            }
        } else {
            // extend this row
	    bool subtracted = (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED);
	    if (subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
            psphotRadialPlot (&kapa, "radial.plots.ps", source);
            psphotMosaicSubimage (outpos, source, Xo, Yo, DX, DY, true);

	    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
            psphotMosaicSubimage (outsub, source, Xo, Yo, DX, DY, true);
	    if (!subtracted) pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

            Xo += DX;
            dY = PS_MAX (dY, DY);
        }
    }

    psphotSaveImage (NULL, outpos, "outpos.fits");
    psphotSaveImage (NULL, outsub, "outsub.fits");
    psLogMsg ("psphot", PS_LOG_INFO, "plotted %d sources (%d psf, %d sat): %f sec\n", nPSF + nSAT, nPSF, nSAT, psTimerMark ("psphot"));

    psFree (outpos);
    psFree (outsub);
    return true;
}

