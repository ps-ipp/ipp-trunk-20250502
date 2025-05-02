# include "psphotInternal.h"

bool psphotMosaicChip(pmConfig *config, const pmFPAview *view, char *outFile, char *inFile)
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

    // XXXX we are failing to get the output hdu right
    pmChip *outChip = pmFPAviewThisChip(view, out->fpa);
    pmChip *inChip = pmFPAviewThisChip(view, in->fpa);
    if (!outChip->hdu && !outChip->parent->hdu) {
        pmFPAAddSourceFromView(out->fpa, view, out->format);
    }

    psImageMaskType blankMask = pmConfigMaskGet("BLANK", config);

    // mosaic the chip, forcing a deep copy (resulting images are not subimages)
    psTrace("pmChipMosaic", 5, "mosaic chip %s to %s (xbin,ybin: %d,%d to %d,%d)\n",
            in->name, out->name, in->xBin, in->yBin, out->xBin, out->yBin);
    status = pmChipMosaic(outChip, inChip, true, blankMask);
    return status;
}

// XXX does this do everything needed?
// * mask & weight
// * loaded PSF model (in readout->analysis)
// * loaded SRC sources (in readout->analysis)
