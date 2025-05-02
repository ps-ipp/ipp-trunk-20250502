#include "ppSim.h"

// XXX this is essentially identical to ppImageMosaicChip
bool ppSimMosaicChip(pmConfig *config, const psImageMaskType blankMask, const pmFPAview *view,
                       const char *outFile, const char *inFile)
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

    pmChip *outChip = pmFPAviewThisChip(view, out->fpa);
    pmChip *inChip = pmFPAviewThisChip(view, in->fpa);
    if (!outChip->hdu && !outChip->parent->hdu) {
        pmFPAAddSourceFromView(out->fpa, view, out->format);
    }

    psTrace("pmChipMosaic", 5, "mosaic chip %s to %s (xbin,ybin: %d,%d to %d,%d)\n",
            in->name, out->name, in->xBin, in->yBin, out->xBin, out->yBin);

    // XXX mosaic the chip, making a deep copy.  this has the side effect of making the
    // output image products pure trimmed images, but also increases the memory footprint.
    status = pmChipMosaic(outChip, inChip, true, blankMask);
    return status;
}

