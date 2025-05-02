#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPSF.h"


bool ppVizPSFLoop(ppVizPSFData *data // Run-time data
    )
{
    pmConfig *config = data->config;                                        // Configuration data
    pmFPAfile *psfFile = pmFPAfileSelectSingle(config->files, "PSPHOT.PSF.LOAD", 0); // File with PSF

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }
    fprintf(stderr,"Woo!\n");
    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, psfFile->fpa, 1))) {
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            psError(PS_ERR_UNKNOWN, false, "Error loading data from files.");
            return false;
        }

        if (chip->cells->n != 1) {
            psWarning("More than one cell present for chip %d", view->chip);
        }

        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, psfFile->fpa, 1))) {
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                psError(PS_ERR_UNKNOWN, false, "Error loading data from files.");
                return false;
            }

            if (cell->readouts->n == 0) {
                pmReadout *ro = pmReadoutAlloc(cell);
                ro->data_exists = true;
                cell->data_exists = true;
                chip->data_exists = true;
                psFree(ro);             // Drop reference
            }

            if (cell->readouts->n > 1) {
                psWarning("More than one readout present for chip %d, cell %d", view->chip, view->cell);
            }
	    fprintf(stderr,"Woo!\n");
            pmReadout *readout;         // Readout from cell
            while ((readout = pmFPAviewNextReadout(view, psfFile->fpa, 1))) {
	      fprintf(stderr,"Woo?\n");
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    psError(PS_ERR_UNKNOWN, false, "Error loading data from files.");
                    return false;
                }
		fprintf(stderr,"Woo2?\n");
                pmPSF *psf = psMetadataLookupPtr(NULL, chip->analysis, "PSPHOT.PSF");              // PSF
                assert(psf);

                bool mdok;              // Status of MD lookup
                psArray *sources = psMetadataLookupPtr(&mdok, readout->analysis, "PSPHOT.SOURCES"); // Sources

		if (sources && !readout->data_exists) {  // This fails if -sources not specified
                    continue;
                }
		readout->data_exists = true;
		fprintf(stderr,"Woo! %ld\n", sources ? sources->n : -1);

		fprintf(stderr,"%d\n",mdok);
                int numCols = 0, numRows = 0;              // Size of image
                psVector *xOffset = NULL, *yOffset = NULL; // Offset from source to true position

                if (sources || data->input || (data->fakeNum > 0 && isfinite(data->fakeMag))) {
                    numCols = psf->fieldNx;
                    numRows = psf->fieldNy;
                    psLogMsg("ppVizPSF", PS_LOG_INFO, "Generating %dx%d image", numCols, numRows);
                }
                if (sources && !sources->n) {
                    psMemIncrRefCounter(sources);
                    psLogMsg("ppVizPSF", PS_LOG_INFO, "Using %ld input sources from CMF file", sources->n);
                } else if (data->input) {
                    psVector *x = data->input->data[0]; // x coordinates
                    psLogMsg("ppVizPSF", PS_LOG_INFO, "Using %ld input sources from text file", x->n);
                }

                if (data->fakeNum > 0 && isfinite(data->fakeMag)) {
		  fprintf(stderr,"Here! fakes\n");
                    long numOld = 0; // Old number of sources
                    long numNew = -1; // New number of sources
                    psLogMsg("ppVizPSF", PS_LOG_INFO, "Adding %d fake sources", data->fakeNum);
                    if (sources) {
                        numOld = sources->n;
                        numNew = numOld + data->fakeNum;
                        sources = psArrayRealloc(sources, numNew);
                        sources->n = numNew;
                    } else {
                        numNew = data->fakeNum;
                        sources = psArrayAlloc(numNew);
                    }

                    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

                    for (int i = numOld; i < numNew; i++) {
                        pmSource *source = pmSourceAlloc(); // Fake source
                        sources->data[i] = source;
                        float xSrc = psRandomUniform(rng) * numCols, ySrc = psRandomUniform(rng) * numRows; // Position of source

                        pmModel *model = pmModelFromPSFforXY(psf, xSrc, ySrc, 1.0); // Model for normalisation
                        float fluxNorm = model->class->modelFlux(model->params);               // Flux for peak=1
                        float fluxPeak = powf(10.0, -0.4 * data->fakeMag) / fluxNorm; // Peak flux
                        source->peak = pmPeakAlloc(xSrc, ySrc, fluxPeak, PM_PEAK_LONE);
                        source->psfMag = data->fakeMag;
                        psFree(model);
                    }
                }
                if (!sources && !data->input) {
		  fprintf(stderr,"Here! default\n");
                    // Generate fake image with only a single realisation of the PSF
                    sources = psArrayAlloc(1);
                    pmSource *source = pmSourceAlloc(); // Fake source
                    sources->data[0] = source;
                    float xRel = isfinite(data->x) ? data->x : 0.5; // Relative position in x
                    float yRel = isfinite(data->y) ? data->y : 0.5; // Relative position in y
                    float xSrc = xRel * psf->fieldNx, ySrc = yRel * psf->fieldNy; // Position of source
                    source->peak = pmPeakAlloc(xSrc, ySrc, 1.0, PM_PEAK_LONE);
                    pmModel *model = pmModelFromPSFforXY(psf, xSrc, ySrc, 1.0); // Model for normalisation
                    float flux = model->class->modelFlux(model->params);               // Flux for peak=1
                    psFree(model);
                    source->psfMag = -2.5 * log10(flux);
                    xOffset = psVectorAlloc(1, PS_TYPE_S32);
                    yOffset = psVectorAlloc(1, PS_TYPE_S32);
                    xOffset->data.S32[0] = data->size - xSrc;
                    yOffset->data.S32[0] = data->size - ySrc;
                    numCols = 2 * data->size + 1;
                    numRows = 2 * data->size + 1;
                    psLogMsg("ppVizPSF", PS_LOG_INFO, "Generating %dx%d image with single PSF",
                             numCols, numRows);

                    psRegion *trimsec = psMetadataLookupPtr(NULL, cell->concepts, "CELL.TRIMSEC");
                    *trimsec = psRegionSet(0, numCols, 0, numRows);
                }

                if (numCols <= 0 || numRows <= 0) {
                    psError(PS_ERR_UNKNOWN, true, "Image size isn't set: %dx%d.", numCols, numRows);
                    return false;
                }

                // We have trouble with PSF residuals, so don't use them
		// XXX EAM 2022.05.02 : re-instating the residuals; what kind of trouble??
		// optionally use residuals (if -use-residuals is supplied)
		if (!data->useResiduals) {
		    fprintf (stderr, "skipping residuals\n");
		    psFree(psf->residuals);
		    psf->residuals = NULL;
		}
		fprintf(stderr,"still here");
                if (sources) {
		  fprintf(stderr,"Here! sources?\n");
                    if (!pmReadoutFakeFromSources(readout, numCols, numRows, sources, 0, xOffset, yOffset,
                                                  psf, data->minFlux, 0, false, true)) {
                        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake readout.");
                        return false;
                    }
                } else if (data->input) {

                    psVector *x = data->input->data[0]; // x coordinates
                    psVector *y = data->input->data[1]; // y coordinates
                    psVector *mag = data->input->data[2]; // Magnitudes
		    fprintf(stderr,"Here! input? %d %d %g %d %ld\n",numCols,numRows,data->minFlux,1,x->n);
                    if (!pmReadoutFakeFromVectors(readout, numCols, numRows, x, y, mag, xOffset, yOffset,
                                                  psf, data->minFlux, 0, false, true)) {
                        psError(PS_ERR_UNKNOWN, false, "Unable to generate fake readout.");
                        return false;
                    }
                }
		
                psFree(sources);
                psFree(xOffset);
                psFree(yOffset);

                pmHDU *hdu = pmHDUGetLowest(psfFile->fpa, chip, cell); // HDU for readout
                if (!hdu) {
                    psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find HDU for data.");
                    return false;
                }
                if (!hdu->header) {
                    hdu->header = psMetadataAlloc();
                }
                ppVizPSFVersionHeader(hdu->header);


                // Readout
                if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                    psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
                    return false;
                }
            }
            // Cell
            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
                return false;
            }
        }
        // Chip
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
            return false;
        }
    }
    // FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(PS_ERR_UNKNOWN, false, "Error saving data to files.");
        return false;
    }

    return true;
}
