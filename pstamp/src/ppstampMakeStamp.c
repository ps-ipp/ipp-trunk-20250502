#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <strings.h>

#include "ppstamp.h"
#include "pmAstrometryUtils.h"

#define WCS_NONLIN_TOL 0.001            // Non-linear tolerance for header WCS

typedef enum {
    PPSTAMP_OFF,
    PPSTAMP_PARTIALLY_ON,
    PPSTAMP_ON,
    PSTAMP_ERROR
} ppstampOverlap;

static void skyToChip(pmAstromObj *pt, pmFPA *fpa, pmChip *chip);
static void chipToSky(pmAstromObj *pt, pmFPA *fpa, pmChip *chip);
static bool setMaskedToNAN(pmConfig *config, psImage *image, psImage *mask, psImage *variance);
static bool copySources(pmReadout *outputReadout, pmReadout *inputReadout, pmReadout *astromReadout, psRegion extractRegion);
static bool imageHasValidPixels(psImage *image);

// convert the input chip's transforms to the output
static bool convertWCS(pmHDU *outHDU, pmFPA *inFPA, pmChip *inChip, pmFPA *outFPA, pmChip *outChip, psRegion *roi)
{
    PS_ASSERT_PTR_NON_NULL(outHDU, 1);
    PS_ASSERT_PTR_NON_NULL(outHDU->header, 1);

    // Center of stamp in chip coordinates
    double center_x = (roi->x0 + roi->x1) / 2;
    double center_y = (roi->y0 + roi->y1) / 2;
    pmAstromObj *check = pmAstromObjAlloc();
    check->chip->x = center_x;
    check->chip->y = center_y;
    chipToSky(check, inFPA, inChip);
    double r_before = check->sky->r;
    double d_before = check->sky->d;
    psLogMsg("ppstampMakeStamp", 2, "Before fit  Center Pixel RA: %f   DEC: %f (degrees)\n",
             r_before * PS_DEG_RAD, d_before * PS_DEG_RAD);

    if (outFPA->toSky->type != PS_PROJ_TAN) {
        // we need to make astrometry terms for the output which are centered on the output chip center,
        // but we keep the original plate scale
        psFree(outFPA->toSky);
        outFPA->toSky = psProjectionAlloc (r_before, d_before, inFPA->toSky->Xs, inFPA->toSky->Ys, PS_PROJ_TAN);

        if (!pmAstromLinearizeToSky(inFPA, inChip, outFPA, outChip, roi)) {
            psFree(check);
            psError(PS_ERR_UNKNOWN, false, "Failed to linearize astrometry\n");
            return false;
        }
        check->chip->x = (roi->x1 - roi->x0) / 2;
        check->chip->y = (roi->y1 - roi->y0) / 2;
        chipToSky(check, outFPA, outChip);
        double r_after = check->sky->r;
        double d_after = check->sky->d;

        psLogMsg("ppstampMakeStamp", 2, "After fit:  Center Pixel RA: %f   DEC: %f (degrees)\n",
                 r_after * PS_DEG_RAD, d_after * PS_DEG_RAD);
        psLogMsg("ppstampMakeStamp", 2, "Error in fit to astrometry %.2f    %.2f (arcseconds)\n",
                 (r_after - r_before) *  PS_DEG_RAD * 3600, (d_after - d_before) * PS_DEG_RAD * 3600);

        // XXX: should we fail if the fit is bad ??

    } else {
        outChip->toFPA     = psPlaneTransformSetCenter(NULL, inChip->toFPA, (int) roi->x0, (int) roi->y0);
        outChip->fromFPA   = psPlaneTransformInvert(NULL, outChip->toFPA, *roi, 50, 4);
        if (!outChip->fromFPA) {
            psError(PS_ERR_UNKNOWN, false, "inversion of toFPA transform failed");
            return false;
        }
    }
    psFree(check);

    if (!pmAstromWriteWCS(outHDU->header, outFPA, outChip, WCS_NONLIN_TOL)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to write WCS to output\n");
        return false;
    }

    return true;
}

static bool copyMetadata(pmFPAfile *output, pmFPAfile *input, pmChip *inChip, ppstampOptions *options, pmAstromObj *center, pmFPAfile *astrom, pmFPAview *inView)
{
    pmChip    *outChip;
    pmFPAview *outView = pmFPAviewAlloc(0);

    // our output file has a single chip
    outView->chip = 0;
    outChip = pmFPAviewThisChip(outView, output->fpa);
    psFree(outView);
    pmHDU *outHDU = pmHDUGetHighest(output->fpa, outChip, NULL);

    // copy data from the input's chip header to the output.
    // since some of the keywords might be duplicated we may not want to copy both

    pmHDU *inHDU  = NULL;
    if (strcmp(options->stage, "raw") && input != astrom) {
        // Copy the header from the astrometry file since it contains the psphot and psastro results
        pmChip *astromChip = pmFPAviewThisChip(inView, astrom->fpa);
        inHDU  = pmHDUFromChip(astromChip);
    } else {
        inHDU  = pmHDUFromChip(inChip);
    }

    if (inHDU->header) {
        outHDU->header = psMetadataCopy(outHDU->header, inHDU->header);
    } else if (!outHDU->header) {
        outHDU->header = psMetadataAlloc();
    }

    // copy the fpa and chip concepts
    pmConceptsCopyFPA(output->fpa, input->fpa, false, false);
    // Needed to preserve FPA.ZP
    pmConceptsCopyChip(outChip, inChip, false);

    // copy the cell concepts (needed to get CELL.EXPOSURE which gets used to set EXPTIME)
    pmCell *outCell = outChip->cells->data[0]; // The only output cell

    if (inChip->cells->n == 1) {
        pmConceptsCopyCell(outCell, inChip->cells->data[0]);
        // Need to fix up the trimsec and biassec to correspond to the output
        psMetadataItem *trimsec = psMetadataLookup(outCell->concepts, "CELL.TRIMSEC");
        psFree(trimsec->data.V);
        trimsec->data.V = NULL;
        psMetadataItem *biassec = psMetadataLookup(outCell->concepts, "CELL.BIASSEC");
        psFree(biassec->data.V);
        biassec->data.V = psListAlloc(NULL);
    } else {
        psList *inCells = psArrayToList(inChip->cells); // Input cells
        pmConceptsAverageCells(outCell, inCells, NULL, NULL, false);
        psFree(inCells);
    }

    // If input had WCS convert it for the new image coordinate system of our stamp
    if (input->fpa->toSky) {
        // copy the input fpa's transforms
        // XXX: if we change to making multiple stamps per process we'll need to copy
        // the transformations
        output->fpa->toTPA   = psMemIncrRefCounter(input->fpa->toTPA);
        output->fpa->fromTPA = psMemIncrRefCounter(input->fpa->fromTPA);
        output->fpa->toSky   = psMemIncrRefCounter(input->fpa->toSky);

        if (!convertWCS(outHDU, input->fpa, inChip, output->fpa, outChip, &options->roi)) {
            return false;
        }
    } else {
        psLogMsg("ppstampMakeStamp", PS_LOG_WARN, "No WCS present in input.");
    }

    // Save our specific metadata
    psMetadataAddF64(outHDU->header, PS_LIST_TAIL, "RA_DEG", PS_META_REPLACE, "Right Ascension of stamp center", RAD_TO_DEG(center->sky->r));
    psMetadataAddF64(outHDU->header, PS_LIST_TAIL, "DEC_DEG", PS_META_REPLACE, "Declination of stamp center", RAD_TO_DEG(center->sky->d));

    // XXX: TODO: save the region of interest boundary and the offset to be applied. 
    // XXX: Gene thinks that there may be a way to handle this in the WCS

    // Now a few sanity checks
    // Make sure zero point in the fpa concepts matches the observed value in the input header
    bool status;
    float zpt_obs = psMetadataLookupF32(&status, outHDU->header, "ZPT_OBS");
    if (status) {
        psMetadataAddF32(output->fpa->concepts, PS_LIST_TAIL, "FPA.ZP", PS_META_REPLACE, "Magnitude zero point", zpt_obs);
    }

    ppstampVersionMetadata(outHDU->header, options);

    // copy any user supplied keywords
    if (options->headerAdditions) {
        if (!psMetadataOverlay(outHDU->header, options->headerAdditions)) {
            psError(PS_ERR_UNKNOWN, false, "Failed to copy header additions to output\n");
            return false;
        }
    }


    return true;
}

// Extract pixels from an image. If part of the bounds of the region are
// partially outside of the input those pixels are set to zero.
static psImage *extractStamp(psImage *image, psRegion region, double value)
{
    int width  = region.x1 - region.x0 + 0.5;
    int height = region.y1 - region.y0 + 0.5;

    if (width < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "negative width\n");
        return NULL;
    }
    if (height < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "negative height\n");
        return NULL;
    }

    int srcX  = region.x0;
    int srcY  = region.y0;
    int lastX = region.x1;
    int lastY = region.y1;
    if (lastX > (image->col0 + image->numCols)) {
        lastX = image->col0 + image->numCols;
    }
    if (lastY > (image->row0 + image->numRows)) {
        lastY = image->row0 + image->numRows;
    }

    int leftBlank = 0;
    int dstX  = 0;
    if (srcX < image->col0) {
        leftBlank = image->col0 - srcX;
        dstX += leftBlank;
        srcX = 0;
    }

    int copyWidth  = lastX - srcX;
    if (copyWidth <= 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "copyWidth is invalid: %d\n", copyWidth);
        return NULL;
    }
    if (copyWidth > width) {
        copyWidth = width;
    }

    psImage *output = psImageAlloc(width, height, image->type.type);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "psImageAlloc failed\n");
        return NULL;
    }

    if (!psImageInit(output, value)) {
        psError(PS_ERR_UNKNOWN, false, "psInitImage failed\n");
        return NULL;
    }

    psElemType  type = image->type.type;
    psS32       elementSize =  PSELEMTYPE_SIZEOF(type);
    int         copySize = copyWidth * elementSize;

    int     dstY = 0;
    psS32   skip = image->row0 - srcY;
    if (skip > 0) {
        dstY = skip;
        srcY = image->row0;
    }
    for ( ; dstY < height; srcY++, dstY++) {
        if (srcY >= lastY) {
            break;
        }
        // copy copyWidth pixels from srcX,srcY to dstX, dstY
        psU8 *pdst = output->data.U8[dstY] + (dstX * elementSize);

        psU8 *psrc = image->data.U8[srcY]  + (srcX * elementSize);

        memcpy(pdst, psrc, copySize);
    }

    return output;
}

// Build the postage stamp output file

static int makeStamp(pmConfig *config, ppstampOptions *options, pmFPAfile *input,
                pmChip *inChip, pmFPAview *view, pmAstromObj *center, pmFPAfile *astrom)
{
    int status = false;

    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, options->outputFileRule);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Can't find output data\n");
        return PS_EXIT_DATA_ERROR;
    }
    pmFPAview *outview = pmFPAviewAlloc(0);
    pmFPAAddSourceFromView(output->fpa, outview, output->format);

    outview->chip = 0;
    outview->cell = 0;
    pmCell *target =  pmFPAviewThisCell(outview, output->fpa); // Target cell
    psFree(outview);
    outview = NULL;

    //   psMetadataPrint(stderr, inChip->concepts, 0);

    // These default to zero. would that be ok?
    psMetadataAddS32(target->concepts, PS_LIST_TAIL, "CELL.XBIN", PS_META_REPLACE, "Binning in x", 1);
    psMetadataAddS32(target->concepts, PS_LIST_TAIL, "CELL.YBIN", PS_META_REPLACE, "Binning in y", 1);

    pmFPAfile *srcFile;
    pmFPAview *srcView = pmFPAviewAlloc(0);
    if (psArrayLength(inChip->cells) > 1) {
        // we extract our postage stamp from a mosaic
        pmFPAfile *mosaic = ppstampBuildMosaic(config, input, view);
        if (mosaic == NULL) {
            return PS_EXIT_UNKNOWN_ERROR;
        }
        srcFile = mosaic;
        srcView->chip = 0;
    } else {
        srcFile = input;
        *srcView = *view;
    }

    if (astrom->camera != srcFile->camera) {
        psError(PS_ERR_UNKNOWN, true, "Input camera and astrometry camera do not match");
        return PS_EXIT_CONFIG_ERROR;
    }

    // At this point we know we have one cell
    srcView->cell = 0;

    pmReadout *readout;
    while ((readout = pmFPAviewNextReadout (srcView, srcFile->fpa, 1)) != NULL) {
        if (!readout->data_exists) {
            psError(PS_ERR_UNKNOWN, false, "no data in input readout!\n");
            continue;
        }
        pmReadout *outReadout = pmReadoutAlloc(target);
        if (!outReadout) {
            psError(PS_ERR_UNKNOWN, false, "failed to allocate output readout\n");
            status = false;
            break;
        }

        psRegion extractRegion = options->roi;

        // Close your eyes while I hack around bug 986
        if (ppstampMegacamWorkaround) {
            // the coordinates of the mosaic are shifted 32 pixels from the chip
            // TODO does this always apply? For example I doubt that applies to
            // skycells.
            extractRegion.x0 -= 32;
            extractRegion.x1 -= 32;
        }

        outReadout->image = extractStamp(readout->image, extractRegion,  NAN);
        if (!outReadout->image) {
            psError(PS_ERR_UNKNOWN, false, "failed to create postage stamp image\n");
            status = false;
            break;
        }
        if (!imageHasValidPixels(outReadout->image)) {
            return PSTAMP_NO_VALID_PIXELS;
        }
        if (readout->variance) {
            outReadout->variance = extractStamp(readout->variance, extractRegion,  NAN);
            if (!outReadout->variance) {
                psError(PS_ERR_UNKNOWN, false, "failed to create postage stamp weight image\n");
                status = false;
                break;
            }
        }
        if (readout->mask) {
            psImageMaskType maskInitValue = pmConfigMaskGet("BLANK", config);
            outReadout->mask = extractStamp(readout->mask, extractRegion,  maskInitValue);
            if (!outReadout->mask) {
                psError(PS_ERR_UNKNOWN, false, "failed to create postage stamp mask image\n");
                status = false;
                break;
            }

            if (options->censorMasked && !setMaskedToNAN(config, outReadout->image, outReadout->mask, outReadout->variance)) {
                psError(PS_ERR_UNKNOWN, false, "failed to create postage stamp mask image\n");
                status = false;
                break;
            }
        }

        outReadout->data_exists = true;
        outReadout->parent->data_exists = true;
        outReadout->parent->parent->data_exists = true;
        status = true;

        if (options->writeCMF) {
            pmReadout *sourcesReadout = NULL;
            if (astrom->fpa != input->fpa) {
                sourcesReadout = pmFPAviewThisReadout(srcView, astrom->fpa);
            } else {
                sourcesReadout = readout;
            }
            if (!copySources(outReadout, sourcesReadout, sourcesReadout, extractRegion)) {
                psError(PS_ERR_UNKNOWN, false, "failed to extract sources from region of interest.\n");
                status = false;
                break;
            }
        }

        psFree(outReadout); // drop reference
    }

    psFree(srcView);

    if (status) {
        // For raw stage we need to do more work to build a proper header
        status = copyMetadata(output, input, inChip, options, center, astrom, view);
    }
    return status ? PS_EXIT_SUCCESS : PS_EXIT_UNKNOWN_ERROR;
}



static bool regionContainsPoint(psRegion *r, psPlane *pt)
{
    if (pt->x < r->x0)
        return false;
    if (pt->x >= r->x1)
        return false;
    if (pt->y < r->y0)
        return false;
    if (pt->y >= r->y1)
        return false;

    return true;
}

// true if the inner region is equal to or completely contained in
// the outer region
static bool regionContainsRegion(psRegion *outer, psRegion *inner)
{
    if ((outer->x0 <= inner->x0) &&
        (outer->y0 <= inner->y0) &&
        (outer->x1 >= inner->x1) &&
        (outer->y1 >= inner->y1)) {
        return true;
    } else {
        return false;
    }
}


static void TPToChip(pmAstromObj *pt, pmFPA *fpa, pmChip *chip)
{
    psPlaneTransformApply(pt->FP, fpa->fromTPA, pt->TP);
    // convert from FP to chip
    psPlaneTransformApply(pt->chip, chip->fromFPA, pt->FP);
}
static void skyToChip(pmAstromObj *pt, pmFPA *fpa, pmChip *chip)
{
    // convert from sky to TP to FP
    psProject(pt->TP, pt->sky, fpa->toSky);
    TPToChip(pt, fpa, chip);
}

static void chipToSky(pmAstromObj *pt, pmFPA *fpa, pmChip *chip)
{
    // chip to FP
    psPlaneTransformApply(pt->FP, chip->toFPA, pt->chip);
    // FP to TP to sky
    psPlaneTransformApply(pt->TP, fpa->toTPA, pt->FP);
    psDeproject(pt->sky, pt->TP, fpa->toSky);
}

static void compareToBox(psRegion *region, psPlane *pt)
{
    if (pt->x < region->x0)
        region->x0 = pt->x;
    if (pt->x > region->x1)
        region->x1 = pt->x;
    if (pt->y < region->y0)
        region->y0 = pt->y;
    if (pt->y > region->y1)
        region->y1 = pt->y;
}

// For roi width and height given in sky coordinates find a bounding box in chip coordinates
// that encloses the requested range
static void findBoundingBox(ppstampOptions *options, pmFPA *fpa, pmChip *chip, pmAstromObj *center)
{
    pmAstromObj *pt = pmAstromObjAlloc();

    // calculate the four corners of the bounding box in sky coordinates, translate them to
    // chip coordinates and build the ROI by comparison.

    options->roi.x0 = INFINITY;
    options->roi.x1 = -INFINITY;
    options->roi.y0 = INFINITY;
    options->roi.y1 = -INFINITY;

    double dx = 0.5 * options->roip.dRA  / fpa->toSky->Xs;
    double dy = 0.5 * options->roip.dDEC / fpa->toSky->Ys;

    // XXX: why do we limit this?
    if (dx > 8000) {
        psWarning( "requested width %f too large reducing to 8000\n", dx);
        dx = 8000;
    }
    if (dy > 8000) {
        psWarning( "requested height %f too large reducing to 8000\n", dy);
        dy = 8000;
    }

    pt->TP->xErr = 0;
    pt->TP->yErr = 0;

    pt->TP->x = center->TP->x - dx;
    pt->TP->y = center->TP->y - dy;
    TPToChip(pt, fpa, chip);
    compareToBox(&options->roi, pt->chip);

    pt->TP->x = center->TP->x + dx;
    pt->TP->y = center->TP->y - dy;
    TPToChip(pt, fpa, chip);
    compareToBox(&options->roi, pt->chip);

    pt->TP->x = center->TP->x + dx;
    pt->TP->y = center->TP->y + dy;
    TPToChip(pt, fpa, chip);
    compareToBox(&options->roi, pt->chip);

    pt->TP->x = center->TP->x - dx;
    pt->TP->y = center->TP->y + dy;
    TPToChip(pt, fpa, chip);
    compareToBox(&options->roi, pt->chip);

    psFree(pt);

    // save the width and height in case we need them later
    options->roip.dX = options->roi.x1 - options->roi.x0;
    options->roip.dY = options->roi.y1 - options->roi.y0;
}

// findROI
// calculate the region of interest in chip coordinates and determine whether that region
// is completely contained on a single chip

static ppstampOverlap findROI(ppstampOptions *options, pmFPAview *view,
                    pmFPAfile *input, pmFPAfile *astrom, bool bilevelAstrometry,
                    pmChip *chip, pmAstromObj *center)
{
    psString chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");
    psRegion    *chipBounds = ppstampChipRegion(chip);
    bool        onChip = false;
    ppstampOverlap   returnval = PPSTAMP_OFF;

    // set up the astrometry
    pmHDU *hdu = pmFPAviewThisHDU(view, astrom->fpa);
    PS_ASSERT_PTR_NON_NULL(hdu, 1)
    PS_ASSERT_PTR_NON_NULL(hdu->header, 1)

    // we can live without astrometry if the ROI is completely specified in pixel coordinates
    bool mustHaveAstrometry = false;
    if (options->roip.celestialCenter || options->roip.celestialRange) {
        mustHaveAstrometry = true;
    }
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelChip (chip, hdu->header) && mustHaveAstrometry) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read bilevel chip astrometry for input FPA.");
            return false;
        }
    } else {
        if (!pmAstromReadWCS (input->fpa, chip, hdu->header, 1.0) && mustHaveAstrometry) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read WCS astrometry for input FPA.");
            return false;
        }
    }

    if (options->roip.celestialCenter) {

        center->sky->r = options->roip.centerRA;
        center->sky->d = options->roip.centerDEC;
        center->sky->rErr = 0;
        center->sky->dErr = 0;

        skyToChip(center, input->fpa, chip);

        if (regionContainsPoint(chipBounds, center->chip)) {
            psLogMsg("ppstampMakeStamp", 2, "Found center (%f %f) on chip: %s\n",
                center->chip->x, center->chip->y, chipName);
            onChip = true;
        }
    } else {
        // center specified in pixels.
        // If the user specified a name of a chip name wait until we get to that one.
        // If no chip name was specified, select this one (the first one that had data)
        if ((options->chipName == NULL) || !strcasecmp(chipName, options->chipName)) {
            if (options->chipName) {
                psLogMsg("ppstampMakeStamp", 2, "Center on chip: %s\n", chipName);
            }
            if (options->wholeFile) {
                center->chip->x = (chipBounds->x1 + chipBounds->x0 + 1) / 2;
                center->chip->y = (chipBounds->y1 + chipBounds->y0 + 1) / 2;
            } else {
                center->chip->x = options->roip.centerX;
                center->chip->y = options->roip.centerY;
            }
            center->chip->xErr = 0;
            center->chip->yErr = 0;
            onChip = true;
            if (input->fpa->toSky) {
                chipToSky(center, input->fpa, chip);
            }
        }
    }

    //MEH add hack for ROI not to include center on chip
    if (onChip || options->centeroffchip ) {
        if (options->roip.celestialRange) {
            findBoundingBox(options, input->fpa, chip, center);
        } else {
            if (options->wholeFile) { 
                options->roi = *chipBounds;
            } else {
                int width  = options->roip.dX;
                int height = options->roip.dY;
                if (width > 8000) {
                    psWarning( "requested width %d too large reducing to 8000\n", width);
                    width = 8000;
                }
                if (height > 8000) {
                    psWarning( "requested height %d too large reducing to 8000\n", height);
                    height = 8000;
                }

                // calculate the ROI in chip coordinates
                options->roi.x0 = center->chip->x - width / 2;
                options->roi.x1 = options->roi.x0 + width;
                options->roi.y0 = center->chip->y - height / 2;
                options->roi.y1 = options->roi.y0 + height;
            }
        }


        if (options->wholeFile || regionContainsRegion(chipBounds, &options->roi)) {
            psLogMsg("ppstampMakeStamp", 2, "ROI contained on: %s\n", chipName);
            returnval = PPSTAMP_ON;
        } else {
            psLogMsg("ppstampMakeStamp", 2, "Partial Overlap chip %s\n", chipName);
            psLogMsg("ppstampMakeStamp", 2, "ROI:         %s\n", psRegionToString(options->roi));
            psLogMsg("ppstampMakeStamp", 2, "Chip Extent: %s\n", psRegionToString(*chipBounds));

            returnval = PPSTAMP_PARTIALLY_ON;
        }
    }
    psFree(chipBounds);

    return returnval;
}

int ppstampMakeStamp (pmConfig *config, ppstampOptions *options)
{
    bool        status = false;
    int        returnval = PS_EXIT_SUCCESS;;
    bool        foundOverlap = false;

    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPSTAMP.INPUT");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "Can't find input file!\n");
        return PS_EXIT_DATA_ERROR;
    }

    pmFPAfile *astrom = psMetadataLookupPtr(NULL, config->files, "PSWARP.ASTROM");
    if (!astrom) {
        astrom = input;
    }

    pmFPAview *view = pmFPAviewAlloc(0);// View for level of interest

    // files associated with the science image
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to load input.");
        psFree (view);
        return PS_EXIT_DATA_ERROR;
    }
    bool bilevelAstrometry  = false;
    pmHDU *phu = pmFPAviewThisPHU(view, astrom->fpa);
    if (phu) {
        char *ctype = psMetadataLookupStr(NULL, phu->header, "CTYPE1");
        if (ctype) {
            bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
        }
    }
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelMosaic(input->fpa, phu->header)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read bilevel mosaic astrometry for input FPA.");
            psFree(view);
            return PS_EXIT_DATA_ERROR;
        }
    }

    // Loop over the chips and find the one that contains the center
    pmAstromObj *center = pmAstromObjAlloc();
    pmChip *chip;
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1)) != NULL) {
        bool allDone = false;

        if (!chip->process || !chip->file_exists) {
            continue;
        }

        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            psError(PS_ERR_UNKNOWN, false, "failed to load chip");
            status = false;
            break;
        }

        ppstampOverlap overlap = findROI(options, view, input, astrom, bilevelAstrometry, chip, center);

        switch (overlap) {
        case PPSTAMP_OFF:
            // keep looking
            break;
        case PPSTAMP_ON:
        case PPSTAMP_PARTIALLY_ON:
            returnval = makeStamp(config, options, input, chip, view, center, astrom);
            allDone = true;
            foundOverlap = true;
            break;
        case PSTAMP_ERROR:
            returnval = PS_EXIT_UNKNOWN_ERROR;
            allDone = true;
            break;
        default:
            psError(PS_ERR_PROGRAMMING, false, "findROI returned unexpected value %d\n", overlap);
            break;
        }

        pmFPAfileIOChecks(config, view, PM_FPA_AFTER);

        if (allDone) {
            view->chip = -1;
            break;
        }
    }
    pmFPAfileIOChecks(config, view, PM_FPA_AFTER);

    psFree(center);
    psFree(view);

    if (!foundOverlap && (returnval == PS_EXIT_SUCCESS)) {
        psWarning( "ROI not found in input\n");
        returnval = PSTAMP_NO_OVERLAP;
    }

    return returnval;
}


static bool setMaskedToNAN(pmConfig *config, psImage *image, psImage *mask, psImage *variance)
{
#ifdef notdef
    bool status;
    psMetadata *masks = psMetadataLookupMetadata(&status, config->recipes, "MASKS");
    if (!status) {
        psError(PM_ERR_CONFIG, false, "failed to lookup MASKS in recipes\n");
        return false;
    }
    // we set anything masked to NAN except if CONV.POOR is the only bit set
    // First check the old value
    psU32 convPoor = psMetadataLookupU32(&status, masks, "POOR.WARP");
    if (!status) {
        convPoor = psMetadataLookupU32(&status, masks, "CONV.POOR");
        if (!status) {
            psError(PM_ERR_CONFIG, false, "failed to lookup mask value for CONV.POOR in recipes\n");
            return false;
        }
    }
    psU32 suspect = psMetadataLookupU32(&status, masks, "SUSPECT");
    if (!status) {
        psError(PM_ERR_CONFIG, false, "failed to lookup mask value for SUSPECT in recipes\n");
        return false;
    }
    psU32 maskMask = ~(convPoor | suspect);

    double exciseValue;
    if (image->type.type == PS_TYPE_U16) {
        exciseValue = 0xffff;
    } else if (image->type.type == PS_TYPE_F32) {
        exciseValue = NAN;
    } else {
         psError(PS_ERR_PROGRAMMING, true, "unexpected image type: %d\n", image->type.type);
        return false;
    }
    long numExcised = 0;
    for (int y=0; y<image->numRows; y++) {
        for (int x=0; x<image->numCols; x++) {
            psU16 maskVal = psImageGet(mask, x, y);
            if (maskVal & maskMask) {
                numExcised++;
                psImageSet(image, x, y, exciseValue);
                if (variance) {
                    psImageSet(variance, x, y, exciseValue);
                }
            }
        }
    }
#endif

    long numCensored;
    if (!pmCensorMasked(config, image, mask, variance, &numCensored)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to censor masked pixels.\n");
        return false;
    }

    psLogMsg("ppstamp", PS_LOG_DETAIL, "Censored %ld masked pixels\n", numCensored);

    return true;
}

static bool copySources(pmReadout *outReadout, pmReadout *inReadout, pmReadout *astromReadout, psRegion extractRegion) {
    bool status;

    // first look for detections in the input readout. Those would have come from the -sources file
    if (!inReadout->analysis) {
        psLogMsg("ppstampMakeStamp", PS_LOG_WARN, "no analysis metadata found on input\n");
        return false;
    }
    pmDetections *inDetections = psMetadataLookupPtr (&status, inReadout->analysis, "PSPHOT.DETECTIONS");
    if (!inDetections) {
        // No detections on the input readout. Try the astrometry readout if supplied.
        if (astromReadout) {
            inDetections = psMetadataLookupPtr (&status, astromReadout->analysis, "PSPHOT.DETECTIONS");
            if (!inDetections) {
                psLogMsg("ppstampMakeStamp", PS_LOG_WARN, "no detections found on input or astrometry readout\n");
                return false;
            } else {
                psLogMsg("ppstampMakeStamp", PS_LOG_INFO, "using detections from astrom file\n");
            }
        } else {
            psLogMsg("ppstampMakeStamp", PS_LOG_WARN, "no detections found on input readout\n");
            return false;
        }
    }
    if (!inDetections->allSources) {
        psLogMsg("ppstampMakeStamp", PS_LOG_WARN, "no sources array found on detections structure\n");
        return false;
    }

    // Ok we have sources, allocate a detections structure and copy them to the output readout
    psArray *inSources = inDetections->allSources;

    // copy analysis metadata in order to pick up any data stored there from the input file.
    // This includes detection efficiency analysis and extended source parameters.
    psMetadataCopy(outReadout->analysis, inReadout->analysis);

    // now replace the sources with a new container
    pmDetections *outDetections = pmDetectionsAlloc();
    psMetadataAddPtr (outReadout->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN,
        "psphot detections", outDetections);

    psArray *outSources = outDetections->allSources = psArrayAllocEmpty( 100 );

    // include sources that are off the stamp by just a little bit 
    // XXX: should I be doing this?
    #define EXPAND_SIZE 16
    psRegion sourceRegion = extractRegion;
    sourceRegion.x0 -= EXPAND_SIZE;
    sourceRegion.y0 -= EXPAND_SIZE;
    sourceRegion.x1 += EXPAND_SIZE;
    sourceRegion.y1 += EXPAND_SIZE;

    bool adjustCoords = extractRegion.x0 != 0 || extractRegion.y0 != 0;

    for (int i = 0; i < inSources->n; i++ ) {
        pmSource *src = inSources->data[i];

        float x = src->peak->xf;
        float y = src->peak->yf;
        if (x < sourceRegion.x0 || x > sourceRegion.x1 ||
            y < sourceRegion.y0 || y > sourceRegion.y1) {
            continue;
        }
        if (adjustCoords) {
            // adjust the source position for the stamp coordinate system
            src->peak->x  -= extractRegion.x0;
            src->peak->xf -= extractRegion.x0;

            src->peak->y  -= extractRegion.y0;
            src->peak->yf -= extractRegion.y0;
            if (src->moments) {
                src->moments->Mx -= extractRegion.x0;
                src->moments->My -= extractRegion.y0;
            }
            if (src->modelPSF) {
                src->modelPSF->params->data.F32[PM_PAR_XPOS] -= extractRegion.x0;
                src->modelPSF->params->data.F32[PM_PAR_YPOS] -= extractRegion.y0;
            }

            if (src->modelFits) {
                for (int j = 0; j < src->modelFits->n; j++) {
                    pmModel *model = src->modelFits->data[j];
                    assert (model != NULL);
                    model->params->data.F32[PM_PAR_XPOS] -= extractRegion.x0;
                    model->params->data.F32[PM_PAR_YPOS] -= extractRegion.y0;
                }
            }
        }

        psArrayAdd(outSources, 100, src);
    }

    return true;
}

static bool
imageHasValidPixels(psImage *image) {
    // check F32 image and return true if any pixel is finite
    if (image->type.type != PS_TYPE_F32) {
        return true;
    }
    for (int y=0; y<image->numRows; y++) {
        for (int x=0; x<image->numCols; x++) {
            psF32 pixel = image->data.F32[y][x];
            if (isfinite(pixel)) {
                return true;
            }
        }
    }
    return false;
}
