#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

static void detrendRecord(
    bool doThis,                        // Are we supposed to do this?
    psMetadata *target,                 // Target metadata
    const pmConfig *config,             // Configuration
    const pmFPAview *view,              // View to cell
    const char *filename,               // Name of file
    const char *name,                   // Name in header
    const char *desc                    // Description
    )
{
    psAssert(config, "Need configuration");
    psAssert(view, "Need view to cell");
    psAssert(name, "Need file name");

    if (!doThis) {
        return;
    }

    pmFPAfile *file = psMetadataLookupPtr(NULL, config->files, filename); // File of interest
    psAssert(file, "Should be there");

    if (target) {
        psMetadataAddStr(target, PS_LIST_TAIL, name, 0, desc, file->filename);
    }

    pmCell *input = pmFPAfileThisCell(config->files, view, "PPIMAGE.INPUT"); // File we're processing
    psAssert(input, "Should be there");
    pmHDU *hdu = pmHDUGetHighest(input->parent->parent, input->parent, input);  // HDU for cell

    // Strip off path and Nebulous bits
    char *base = file->filename;        // Base name of file
    for (char *new = base; (new = strpbrk(base, "/:")); base = new + 1); // No action

    // We don't want multiple listings in the header saying the same thing, so make sure we haven't put the
    // same entry there.  Usually (if the detrend and the image have the same file level) there'll only end up
    // being one entry.
    psString regexp = NULL;             // Regular expression
    psStringAppend(&regexp, "^%s$", name);
    psMetadataIterator *iter = psMetadataIteratorAlloc(hdu->header, PS_LIST_HEAD, regexp); // Iterator for hdr
    psFree(regexp);
    psMetadataItem *item;               // Item from iteration
    bool found = false;
    while (!found && (item = psMetadataGetAndIncrement(iter))) {
        if (item->type == PS_DATA_STRING && strcmp(item->data.str, base) == 0) {
            found = true;
        }
    }
    psFree(iter);
    if (!found) {
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, name, PS_META_DUPLICATE_OK, desc, base);
    }

    return;
}


bool ppImageDetrendRecord(pmCell *cell, const pmConfig *config, const ppImageOptions *options,
                          const pmFPAview *view)
{
    psMetadata *detrend = psMetadataAlloc(); // Detrend information
    psMetadataAddMetadata(cell->analysis, PS_LIST_TAIL, "DETREND", 0, "Detrend information", detrend);

    detrendRecord(options->doMask,     detrend, config, view, "PPIMAGE.MASK", 	  "DETREND.MASK",     "Mask filename");
    detrendRecord(options->doNoiseMap, detrend, config, view, "PPIMAGE.NOISEMAP", "DETREND.NOISEMAP", "Noise Map filename");
    detrendRecord(options->doBias,     detrend, config, view, "PPIMAGE.BIAS", 	  "DETREND.BIAS",     "Bias filename");
    detrendRecord(options->doDark,     detrend, config, view, "PPIMAGE.DARK", 	  "DETREND.DARK",     "Dark filename");
    detrendRecord(options->doShutter,  detrend, config, view, "PPIMAGE.SHUTTER",  "DETREND.SHUTTER",  "Shutter correction filename");
    detrendRecord(options->doFlat,     detrend, config, view, "PPIMAGE.FLAT",     "DETREND.FLAT",     "Flat filename");
    detrendRecord(options->doFringe,   detrend, config, view, "PPIMAGE.FRINGE",   "DETREND.FRINGE",   "Fringe filename");

    detrendRecord(options->doNonLin,    detrend, config, view, "PPIMAGE.LINEARITY","DETREND.NONLIN",   "Non-linearity table filename");
    detrendRecord(options->doNewNonLin, detrend, config, view, "PPIMAGE.NEWNONLIN","DETREND.NEWNONLIN","Non-linearity table filename (v2023)");

    detrendRecord(options->doDark & options->useVideoDark, detrend, config, view, "PPIMAGE.VIDEODARK", "DETREND.VIDEODARK", "VideoDark filename");
    detrendRecord(options->doMask & options->useVideoMask, detrend, config, view, "PPIMAGE.VIDEOMASK", "DETREND.VIDEOMASK", "VideoMASK filename");
    psFree (detrend);
    return true;
}
