#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppstamp.h"

// Mosaic the cells of a given chip


pmFPAfile *ppstampBuildMosaic(pmConfig *config, pmFPAfile *input, pmFPAview *view)
{
    bool    status;

    pmFPAfile *mosaic =  psMetadataLookupPtr(&status, config->files, "PPSTAMP.CHIP");
    if (!status)  {
        psErrorStackPrint(stderr, "can't find mosaic i/o file\n");
        exit(EXIT_FAILURE);
    }

    pmFPAview *mosaicView = pmFPAviewAlloc(0);

    mosaicView->chip = 0;
    pmChip  *mChip  = pmFPAviewThisChip(mosaicView, mosaic->fpa);
    pmChip  *inChip = pmFPAviewThisChip(view, input->fpa);
    if (!mChip->hdu && !mChip->parent->hdu) {
        pmFPAAddSourceFromView(mosaic->fpa, mosaicView, mosaic->format);
    }
    psFree(mosaicView);

    psMaskType blankMask = pmConfigMaskGet("BLANK", config);

    status = pmChipMosaic(mChip, inChip, true, blankMask);
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "chip mosaic failed\n");
        return NULL;
    }

    return mosaic;
}

