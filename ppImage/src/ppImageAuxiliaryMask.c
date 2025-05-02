#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

psImage *readAuxiliaryMask(pmConfig *config, psString fileName)
{
    psString realName = pmConfigConvertFilename(fileName, config, false, false);
    if (!realName) {
        psError (psErrorCodeLast(), false, "unable to resolve %s", fileName);
        return NULL;
    }

    psFits *fits = psFitsOpen(realName, "r");
    if (!fits) {
        psError (psErrorCodeLast(), false, "psFitsOpen failed for %s", realName);
        psFree(realName);
        return NULL;
    }
    psMetadata *header = psFitsReadHeader(NULL, fits);
    if (!header) {
        psFree(fits);
        psError (psErrorCodeLast(), false, "psFitsReadHeader failed for %s", realName);
        psFree(realName);
        return NULL;
    }
    psRegion region = {0, 0, 0, 0};
    psImage *image = psFitsReadImage(fits, region, 0);
    psFree(fits);
    psFree(header);
    if (!image) {
        psError (psErrorCodeLast(), false, "psFitsReadImage failed for %s", realName);
        psFree(realName);
        return NULL;
    }
    if (image->type.type != PS_TYPE_IMAGE_MASK) {
        psWarning("auxiliary mask image %s has unexpected type %d\n", realName, image->type.type);
        return false;
    }
    psFree(realName);

    return image;
}

bool recordFileInHeader(pmChip *chip, psString tag, psString desc, psString filename)
{
    pmHDU *hdu = pmHDUGetHighest(chip->parent, chip, NULL);

    // strip off the directories and nebulous bits
    char *base = filename;
    for (char *new = base; (new = strpbrk(base, "/:")); base = new + 1);

    psMetadataAddStr(hdu->header, PS_LIST_TAIL, tag, PS_META_DUPLICATE_OK, desc, base);
    return true;
}

// In this function, we augment the mask with the more conservative auxiliary mask
bool ppImageAuxiliaryMask(pmConfig *config, const pmFPAview *view, const ppImageOptions *options, psMetadata *stats)
{
    psAssert(config, "Need configuration");
    psAssert(view, "Need view to chip");
    psAssert(options, "Need options");

    if (!options->doAuxMask) {
        psLogMsg ("ppImage", PS_LOG_DETAIL, "Auxiliary mask not enabled.");
        return true;
    }

    // psTimerStart("aux.mask");

    bool status;
    // Our target chip
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPIMAGE.CHIP"); // File to correct
    if (!status) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "PPIMAGE.CHIP file is not defined");
        return false;
    }

    pmFPAfile *auxmask = psMetadataLookupPtr(&status, config->files, "PPIMAGE.AUXMASK");
    if (!status) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "PPIMAGE.CHIP file is not defined");
        return false;
    }

    // Go find the readout
    // code to find the readouts was adapted from ppImageSubtractBackground
    // XXX: shouldn't most of these psWarnings be psAsserts?

    // Since we are working on a chip-mosaicked image, there should only be a single cell and readout
    pmChip *chip = pmFPAviewThisChip(view, input->fpa); // Chip of interest
    if (!chip) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find chip");
        return false;
    }
    if (chip->cells->n == 0) {
        psWarning("Chip has no cells");
        return true;
    }
    if (chip->cells->n > 1) {
        psWarning("Chip has %ld cells; only the first will be processed", chip->cells->n);
    }
    pmCell *cell = chip->cells->data[0]; // Cell of interest
    if (!cell || !cell->process || !cell->file_exists) {
        // Nothing to process
        return true;
    }
    if (cell->readouts->n == 0) {
        psWarning("Cell has no readouts");
        return true;
    }
    if (cell->readouts->n > 1) {
        psWarning("Cell has %ld readouts; only the first will be processed", cell->readouts->n);
    }
    pmReadout *ro = cell->readouts->data[0]; // Readout of interest
    if (!ro || !ro->data_exists) {
        // Nothing to process
        return true;
    }

    psImage *mask = ro->mask;

    if (mask->type.type != PS_TYPE_IMAGE_MASK) {
        psWarning("mask image has unexpected type %d\n", mask->type.type);
        return false;
    }

    pmChip *auxMaskChip = pmFPAviewThisChip(view, auxmask->fpa);
    if (!auxMaskChip) {
        psWarning("failed to find pmChip for auxiliary mask");
        return false;
    }
    pmCell *auxMaskCell = auxMaskChip->cells->data[0];
    if (!auxMaskCell) {
        psWarning("failed to find pmCell for auxiliary mask");
        return false;
    }
    pmReadout *auxMaskReadout = auxMaskCell->readouts->data[0];
    if (!auxMaskReadout) {
        psWarning("failed to find pmReadout for auxiliary mask");
        return false;
    }
    psImage *auxMaskImage = auxMaskReadout->mask;
    if (!auxMaskImage) {
        psWarning("failed to find psImage for auxiliary mask");
        return false;
    }
    if (auxMaskImage->type.type != PS_TYPE_IMAGE_MASK) {
        psWarning("auxiliary mask image has unexpected type %d\n", auxMaskImage->type.type);
        return false;
    }

    // if the cell has video and the recipe value is set or in the video mask
    if (options->hasVideo && options->auxVideoMask && strcmp(options->auxVideoMask, "NULL")) {
        psImage *videoMask = readAuxiliaryMask(config, options->auxVideoMask);
        if (!videoMask) {
            psError(PS_ERR_UNKNOWN, false, "failed to read auxiliary video mask file");
            return false;
        }
        psLogMsg ("ppImage", PS_LOG_INFO, "Auxiliary video mask file: %s", options->auxVideoMask);
        pmConfigRunFilenameAddRead(config, "PPIMAGE.AUXVIDEOMASK", options->auxVideoMask);
        recordFileInHeader(chip, "DETREND.AUXVIDEOMASK", "auxiliary video mask", options->auxVideoMask);

        // compute auxMask |= videoMask
        if (!psBinaryOp(auxMaskImage, auxMaskImage, "|", videoMask)) {
            psError(PS_ERR_UNKNOWN, false, "combination of auxiliary mask and auxiliary video mask failed");
            return false;
        }
        psFree(videoMask);
    }

    if ((mask->numRows != auxMaskImage->numRows) || (mask->numCols != auxMaskImage->numCols) ||
        (mask->row0 != auxMaskImage->row0) || (mask->col0 != auxMaskImage->col0)) {
        psError(PS_ERR_IO, false, "structure of auxiliary mask does not match this chip");
        return false;
    }

    psImageMaskType maskDetector = pmConfigMaskGet("DETECTOR", config);

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "PPIMAGE");
    psAssert(recipe, "Need recipe!");
    psImageMaskType staticMaskVal = psMetadataLookupImageMask(&status, recipe, "MASKSTAT.STATIC");
    psAssert(staticMaskVal, "Need staticMaskVal!");

    // XXX: now that we are using an auxilary mask that matches the format of ps masks, we could just
    // do psBinaryOp(mask, mask, "|", auxMaskImage) but run an explicit loop so that we can measure
    // and log the number of additional pixels masked
    int numCols = mask->numCols, numRows = mask->numRows; // Size of image
    unsigned long numMasked = 0;
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (auxMaskImage->data.U16[y][x] != 0) {
                if ((mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & staticMaskVal) == 0) {
                    // pixel was not previously included in the static mask so mask it
                    ++numMasked;
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskDetector;
                }
                // I don't need to do this.
                // image->data.F32[y][x] = 0.0;
                // The background subtraction code runs after us and will do it there
	    }
        }
    }

    float maskedFrac = (float) numMasked / (numRows * numCols);
    psLogMsg ("ppImage", PS_LOG_INFO, "Auxiliary masks masked %ld aditional pixels. Masked fraction: %f", numMasked, maskedFrac);
    if (stats) {
        psMetadataAddF32(stats,PS_LIST_TAIL, "MASKFRAC_MAGIC", PS_META_REPLACE,
                   "Fraction of pixels masked by auxiliary masks", maskedFrac);
    }

    // psLogMsg ("ppImage", 5, "auxiliary mask: %f sec\n", psTimerMark ("aux.mask"));

    return true;
}
