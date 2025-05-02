#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

bool ppImageMosaicChip(pmConfig *config, const ppImageOptions *options, const pmFPAview *view,
                       const char *outFile, const char *inFile)
{
    bool status;                        // Status of MD lookup

    // psTimerStart("mosaic.chip");

    pmFPAfile *in = psMetadataLookupPtr(&status, config->files, inFile); // Input file
    if (!status) {
        psErrorStackPrint(stderr, "Can't find required I/O file!\n");
        exit(EXIT_FAILURE);
    }

    pmFPAfile *out = psMetadataLookupPtr(&status, config->files, outFile); // Output file
    if (!status) {
        psErrorStackPrint(stderr, "Can't find required I/O file!\n");
        exit(EXIT_FAILURE);
    }

    pmChip *outChip = pmFPAviewThisChip(view, out->fpa);
    pmChip *inChip = pmFPAviewThisChip(view, in->fpa);
    if (!outChip->hdu && !outChip->parent->hdu) {
        pmFPAAddSourceFromView(out->fpa, view, out->format);
    }

    // If the input is a dark (normalising and mosaicking) then it looks like a video cell and will be ignored
    // by pmChipMosaic.  We therefore hack the structure so it doesn't look like a video cell.
    psVector *darkNumbers = NULL;       // Number of dark readouts for each cell
    if (psMetadataLookupBool(&status, config->arguments, "INPUT_IS_DARK")) {
        darkNumbers = psVectorAlloc(inChip->cells->n, PS_TYPE_S32);
	psVectorInit(darkNumbers, 0);
        for (int i = 0; i < inChip->cells->n; i++) {
            pmCell *cell = inChip->cells->data[i];
            if (!cell) {
                continue;
            }
            darkNumbers->data.S32[i] = cell->readouts->n;
            cell->readouts->n = 1;
        }
    }

    psTrace("pmChipMosaic", 5, "mosaic chip %s to %s (xbin,ybin: %d,%d to %d,%d)\n",
            in->name, out->name, in->xBin, in->yBin, out->xBin, out->yBin);

    // Mosaic the chip, making a deep copy.  This has the side effect of making the output
    // image products pure trimmed images, but also increases the memory footprint.
    status = pmChipMosaic(outChip, inChip, true, options->blankMask);

    // Restore dark structure
    if (darkNumbers) {
        for (int i = 0; i < inChip->cells->n; i++) {
            pmCell *cell = inChip->cells->data[i];
            if (!cell) {
                continue;
            }
            cell->readouts->n = darkNumbers->data.S32[i];
        }
        psFree(darkNumbers);
    }

    // psLogMsg ("ppImage", 5, "mosaic chip: %f sec\n", psTimerMark ("mosaic.chip"));

    return status;
}

bool ppImageMosaicFPA (pmConfig *config, const ppImageOptions *options, const char *outFile,
                       const char *inFile)
{
    bool status;                        // Status of MD lookup

    pmFPAfile *in = psMetadataLookupPtr(&status, config->files, inFile); // Input file
    if (!status) {
        psErrorStackPrint(stderr, "Can't find required I/O file!\n");
        exit(EXIT_FAILURE);
    }

    pmFPAfile *out = psMetadataLookupPtr(&status, config->files, outFile); // Output file
    if (!status) {
        psErrorStackPrint(stderr, "Can't find required I/O file!\n");
        exit(EXIT_FAILURE);
    }

    // XXX test printing of all concepts
    #if 0
    for (int i = 0; i < in->fpa->chips->n; i++) {
        pmChip *chip = in->fpa->chips->data[i];
        for (int j = 0; j < chip->cells->n; j++) {
            pmCell *cell = chip->cells->data[j];
            psMetadataPrint(stdout, cell->concepts, 2);
        }
    }
    #endif

    pmFPAview *view = pmFPAviewAlloc(0);
    pmFPAAddSourceFromView(out->fpa, view, out->format);
    psFree(view);

    psTrace ("pmFPAMosaic", 5, "mosaic fpa %s to %s (xbin,ybin: %d,%d to %d,%d)\n",
             in->name, out->name, in->xBin, in->yBin, out->xBin, out->yBin);
    return pmFPAMosaic(out->fpa, in->fpa, false, options->blankMask);
}
