#include "ppSim.h"

# define ESCAPE(CODE,MSG) { \
  psError(CODE, false, MSG); \
  psFree (rng); \
  return false; }

bool ppSimLoop(pmConfig *config)
{
    bool status;

    // XXX if we are supplying a PSF, then we should use that to specify the seeing.
    // we will need to force the psf to be loaded here (deactivate everyone, activate psf, load
    // it, then calculate seeing as appropriate).

    PS_ASSERT_PTR_NON_NULL(config, PS_EXIT_PROG_ERROR);

    // in this program, we are looping over the output image, rather than the input as in ppImage
    pmFPAfile *file = psMetadataLookupPtr(NULL, config->files, "PPSIM.OUTPUT"); // Output file
    assert(file);

    pmFPA *fpa = file->fpa;             // FPA for file
    assert(fpa);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    char *typeStr = psMetadataLookupStr(NULL, recipe, "IMAGE.TYPE"); // Type of image to simulate
    ppSimType type = ppSimTypeFromString (typeStr); // Type of image to simulate
    int binning = psMetadataLookupS32(NULL, recipe, "BINNING"); // Binning in x and y

    bool psfConvolve = psMetadataLookupBool(NULL, recipe, "PSF.CONVOLVE"); // smooth the image with the PSF?

    ppSimUpdateConceptsFPA (fpa, config);
    if (fpa->hdu) { // XXX only do this if there is no INPUT image
        if (!ppSimInitHeader(config, fpa, NULL, NULL)) ESCAPE (PS_ERR_UNKNOWN, "problem setting output header");
    }

    psArray *stars = psArrayAllocEmpty (1);
    psArray *galaxies = psArrayAllocEmpty (1);
    if (type == PPSIM_TYPE_OBJECT) {
        // Load forced-photometry positions (these are placed on fpa->analysis for use in ppSimPhotomReadout)
        // if (!ppSimLoadSpots (fpa, config)) ESCAPE (PS_ERR_UNKNOWN, "failed to load forced-photometry spots");

        // Load catalogue stars
        if (!ppSimLoadStars (stars, fpa, config)) ESCAPE (PS_ERR_UNKNOWN, "failed to load catalog stars");

        // Add random stars
        if (!ppSimMakeStars (stars, fpa, config, rng)) ESCAPE (PS_ERR_UNKNOWN, "failed to make random stars");

        // Add random stars
        if (!ppSimMakeStarGrid (stars, fpa, config, rng)) ESCAPE (PS_ERR_UNKNOWN, "failed to make star grid");

        // Add random stars
        if (!ppSimMakeStarCluster (stars, fpa, config, rng)) ESCAPE (PS_ERR_UNKNOWN, "failed to make cluster stars");

        // Add random galaxies
        if (!ppSimMakeGalaxies (galaxies, fpa, config, rng)) ESCAPE (PS_ERR_UNKNOWN, "failed to make random galaxies");
    }

    pmFPAview *view = pmFPAviewAlloc(0);// View for iterating over FPA

    // load any needed files (eg, input image, PSF)
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
        psError(PS_ERR_UNKNOWN, false, "failed IO for fpa in ppSim\n");
        psFree(view);
        return false;
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, fpa, 1))) {

        // load any needed files (eg, input image, PSF)
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
            psError(PS_ERR_UNKNOWN, false, "failed IO for chip %d in ppSim\n", view->chip);
            psFree (view);
            return false;
        }

        if (type == PPSIM_TYPE_OBJECT) {
            if (!ppSimSetPSF (chip, config)) {
                psError(PS_ERR_UNKNOWN, false, "failed IO for chip %d in ppSim\n", view->chip);
                psFree (view);
                return false;
            }
        }

        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, fpa, 1))) {

            // check that we are able to work with this cell (readdir must be 1)
            int readdir = psMetadataLookupS32(NULL, cell->concepts, "CELL.READDIR"); // Read direction
            if (readdir != 1) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "CELL.READDIR = 1 is the only supported mode.");
                psFree(rng);
                return false;
            }

            // if no readout exists, generate a single readout (upgrade to allow multiple
            // readouts?)
            if (cell->readouts->n == 0) {
                // Size, position and orientation of cell
                int numCols = psMetadataLookupS32(NULL, cell->concepts, "CELL.XSIZE") / binning;
                int numRows = psMetadataLookupS32(NULL, cell->concepts, "CELL.YSIZE") / binning;

                // generate a new readout for this cell
                pmReadout *readout = pmReadoutAlloc(cell); // Readout within cell

                // TO DO: Decide if cell is to be windowed, reduce numCols, numRows appropriately
                readout->image = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Signal in pixels
                readout->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Noise in pixels

                psImageInit (readout->image, 0.0);
                psImageInit (readout->variance, 0.0);

                psFree(readout);        // Drop reference
            }

            psVector *biasCols = ppSimMakeBiassec (cell, config);

            pmReadout *readout;
            while ((readout = pmFPAviewNextReadout (view, fpa, 1))) {

                // if we have not read in a variance or generated a fake image above, we need to
                // build one here
                if (!readout->variance) {
                    if (!pmReadoutGenerateVariance(readout, NULL, true)) {
                        psError (PS_ERR_UNKNOWN, false, "trouble creating variance");
                        return false;
                    }
                }

                psImage *expCorr = NULL; // Exposure correction per pixel, for adding objects
                if (type == PPSIM_TYPE_OBJECT) {
                    expCorr = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
                }

                psVector *biasRows = ppSimMakeBias (&status, readout, config, rng);
                if (!status) ESCAPE (PS_ERR_UNKNOWN, "problem generating dark structure");
                if (type == PPSIM_TYPE_BIAS) goto done;

                if (!ppSimMakeDark (readout, config)) ESCAPE (PS_ERR_UNKNOWN, "problem generating dark structure");
                if (type == PPSIM_TYPE_DARK) goto done;

                if (!ppSimMakeSky (readout, expCorr, type, config)) ESCAPE (PS_ERR_UNKNOWN, "problem generating sky background");
                if (type == PPSIM_TYPE_FLAT) goto done;

                if (type == PPSIM_TYPE_OBJECT) {
                    if (!ppSimInsertStars (readout, expCorr, stars, config)) ESCAPE (PS_ERR_UNKNOWN, "problem inserting stars");
                }

                if (type == PPSIM_TYPE_OBJECT) {
                    if (!ppSimInsertGalaxies (readout, expCorr, galaxies, config)) ESCAPE (PS_ERR_UNKNOWN, "problem inserting galaxies");
                }

                psFree(expCorr);

		// we have two options for generating images which have a PSF:
		// 1) lay down stars with the PSF model applied : in this case, galaxies do NOT have the PSF
		// 2) lay down delta functions for stars and smooth the image with a PSF : in this case, the stars land at integer pixel locations
		if (psfConvolve) {
		    if (!ppSimSmoothReadout(readout, recipe)) ESCAPE (PS_ERR_UNKNOWN, "problem smoothing image");
		}

            done:
                if (!ppSimAddNoise(readout->image, readout->variance, cell, config, rng)) ESCAPE (PS_ERR_UNKNOWN, "problem adding noise");

                if (!ppSimBadCTE(readout->image, config)) ESCAPE (PS_ERR_UNKNOWN, "problem inducing bad cte");

                if (!ppSimSaturate(readout, config)) ESCAPE (PS_ERR_UNKNOWN, "problem setting saturation levels");

                if (!ppSimBadPixels(readout, config, rng)) ESCAPE (PS_ERR_UNKNOWN, "problem adding bad pixels");

                if (!ppSimAddOverscan (readout, config, biasCols, biasRows, rng)) ESCAPE (PS_ERR_UNKNOWN, "problem adding overscan region");
                psFree(biasRows);

                readout->data_exists = true;
                readout->parent->data_exists = true;
                readout->parent->parent->data_exists = true;

                // if there is an input image, merge it with the simulated image
                if (!ppSimMergeReadouts (config, view)) ESCAPE (PS_ERR_UNKNOWN, "problem merging input image with simulated image");
            }
            psFree(biasCols);

            if (!ppSimUpdateConceptsCell (cell, config)) ESCAPE (PS_ERR_UNKNOWN, "problem updating cell concepts");

            if (cell->hdu) {
                // XXX only do this if there is no INPUT image?
                if (!ppSimInitHeader(config, NULL, NULL, cell)) ESCAPE (PS_ERR_UNKNOWN, "problem setting output header");
            }

            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                psError(PS_ERR_IO, false, "Unable to write file.");
                psFree(rng);
                psFree(view);
                // return PS_EXIT_SYS_ERROR;
                return false;
            }
        }

        // XXX why no UpdateConceptsChip??

        if (chip->hdu) {
            // XXX only do this if there is no INPUT image
            if (!ppSimInitHeader(config, NULL, chip, NULL)) ESCAPE (PS_ERR_UNKNOWN, "problem setting output header");
        }

        // we perform photometry on the readouts of this chip in the output
        // if (!ppSimPhotom (config, view)) ESCAPE (PS_ERR_UNKNOWN, "problem performing photometry");

        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(PS_ERR_IO, false, "Unable to write file.");
            psFree(rng);
            psFree(view);
            return false;
        }
    }

    psFree(stars);
    psFree(galaxies);

    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PS_ERR_IO, false, "Unable to write file.");
        psFree(rng);
        psFree(view);
        return false;
    }

    psFree(rng);
    psFree(view);

    return true;
}
