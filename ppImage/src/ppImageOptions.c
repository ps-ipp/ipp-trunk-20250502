#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

static void imageOptionsFree(ppImageOptions *options)
{
    psFree(options->overscan);
    // psFree(options->nonLinearData);
    // psFree(options->nonLinearSource);
    psFree(options->auxVideoMask);
}

ppImageOptions *ppImageOptionsAlloc(void)
{
    ppImageOptions *options = psAlloc(sizeof(ppImageOptions));
    psMemSetDeallocator(options, (psFreeFunc)imageOptionsFree);

    // actions which ppImage should perform
    options->doMaskBuild     = false;   // Build internal mask
    options->doMaskSat       = false;   // mask saturated pixels
    options->doMaskLow       = false;   // mask low pixels
    options->doMaskBurntool  = false;   // mask potential burntool trails
    options->doApplyBurntool = false;   // apply burntool correction
    options->doVarianceBuild = false;   // Build internal variance
    options->doMask          = false;   // Mask bad pixels
    options->doAuxMask       = false;   // apply auxillary mask
    options->doNonLin        = false;   // Non-linearity correction
    options->doNewNonLin     = false;   // New Non-linearity correction (v2023)
    options->doOverscan      = false;   // Overscan subtraction
    options->doNoiseMap      = false;   // Apply Read Noise Map
    options->doBias          = false;   // Bias subtraction
    options->doDark          = false;   // Dark subtraction
    options->doRemnance      = false;   // Remnance masking
    options->doShutter       = false;   // Shutter correction
    options->doFlat          = false;   // Flat-field normalisation
    options->doPatternRow    = false;   // Row pattern correction
    options->doPatternCell   = false;   // Cell pattern correction
    options->doPatternContinuity = false; // Cell continuity correction
    options->doBackgroundContinuity = false; // Chip level background continuity correction
    options->doFringe        = false;   // Fringe subtraction
    options->doPhotom        = false;   // Source identification and photometry
    options->doAstromChip    = false;   // Astrometry (per-chip)
    options->doAstromMosaic  = false;   // Astrometry (full-mosaic)
    options->doStats         = false;   // Measure and save image statistics
    options->checkCTE        = false;   // Measure pixel-based variance
    options->checkNoise      = false;   // Measure cell-level variances.
    options->squashNANs      = false;   // Measure cell-level variances.
    options->applyParity     = false;   // Apply Cell parities
    options->doMaskStats     = false;   // Calculate mask fractions
    options->addNoise        = false;  //Degrade an MD image to a 3pi image
    options->hasVideo        = false;   // Determine if this OTA has a video cell
    options->useVideoDark    = false;   // Use video dark if we can?
    options->useVideoMask    = false;   // Use video mask if we can?
    options->doApplyPixelZero  = true;   // option for zero'ing pixels under masks

    // output files requested
    options->BaseFITS        = false;   // create output image
    options->BaseMaskFITS    = false;   // create output mask image
    options->BaseVarianceFITS  = false;   // create output variance image

    options->ChipFITS        = false;   // create output chip-mosaic image
    options->ChipMaskFITS    = false;   // create output chip-mosaic mask image
    options->ChipVarianceFITS  = false;   // create output chip-mosaic variance image

    options->FPA1FITS        = false;   // create fpa-mosaic binned image (scale 1)
    options->FPA2FITS        = false;   // create fpa-mosaic binned image (scale 2)
    options->Bin1FITS        = false;   // create binned image (scale 1)
    options->Bin2FITS        = false;   // create binned image (scale 2)
    options->Bin1JPEG        = false;   // create jpeg of binned image (scale 1)
    options->Bin2JPEG        = false;   // create jpeg of binned image (scale 2)

    // default flags for various activities
    options->maskValue       = 0x00;    // Default mask value (used to skip / ignore pixels)
    options->satMask         = 0x00;    // Saturated pixels (supplied to pmReadoutGenerateMask)
    options->lowMask         = 0x00;    // out-of-bounds (low) pixels (supplied to pmReadoutGenerateMask)
    options->flatMask        = 0x00;    // Bad flat pixels (supplied to pmFlatField)
    options->darkMask        = 0x00;    // Bad dark pixels (supplied to pmDarkApply)
    options->blankMask       = 0x00;    // Blank (no data, cell gap) pixels (supplied to pmChipMosaic, pmFPAMosaic)
    options->markValue       = 0x00;    // A safe bit for internal marking
    options->burntoolMask    = 0x00;    // Suspect pixels that fall where a burntool trail is expected.
    options->burntoolTrails  = 0x07;    // Which types of burntool areas to mask.
    // crosstalk options
    options->doCrosstalkMeasure = false;   // measure crosstalk
    options->doCrosstalkCorrect = false;   // correct crosstalk

    // Non-linearity default options
    options->nonLinearType   = 0;       // Type of non-linearity data (vector, string or metadata)
    options->nonLinearData   = NULL;    // The non-linearity data
    options->nonLinearSource = NULL;    // If the non-linearity data is a menu, this provides the key

    // Overscan defaults
    options->overscan        = NULL;    // Overscan options

    // binning parameters
    options->xBin1           = 16;      // x-binning, scale 1
    options->yBin1           = 16;      // y-binning, scale 1
    options->xBin2           = 16;      // x-binning, scale 2
    options->yBin2           = 16;      // y-binning, scale 2

    // Fringe defaults
    options->fringeRej       = NAN;     // Fringe rejection limit
    options->fringeIter      = 0;       // Fringe iterations
    options->fringeKeep      = 1.0;     // Fringe keep fraction

    // Pattern correction values

    options->patternRowOrder    = 0;       // Polynomial order
    options->patternRowIter     = 0;       // Clipping iterations
    options->patternRowRej      = NAN;     // Clipping rejection threshold
    options->patternRowThresh   = NAN;     // Threshold for ignoring pixels (pixels with counts > median + thresh * stdev are ignored)
    options->patternRowMean     = PS_STAT_NONE; // Statistic for mean
    options->patternRowStdev    = PS_STAT_NONE; // Statistic for standard deviation
    options->patternCellBG      = PS_STAT_NONE; // Statistic for background
    options->patternCellMean    = PS_STAT_NONE; // Statistic for mean

    // Remnance values
    options->remnanceSize    = 30;      // Size for remnance detection
    options->remnanceThresh  = 25.0;    // Threshold for remnance detection

    // per-class normalization source
    options->normClass       = NULL;    // per-class normalizations refer to this class

    options->auxVideoMask    = NULL;    // auxillary video mask file name

    return options;
}

ppImageOptions *ppImageOptionsParse(pmConfig *config)
{
    bool status;
    ppImageOptions *options = ppImageOptionsAlloc ();

    // select the recipe for this analysis
    bool mdStatus = false;              // Result of MD lookup
    psMetadata *recipe = psMetadataLookupMetadata(&mdStatus, config->recipes, RECIPE_NAME);
    if (! mdStatus || !recipe) {
        psLogMsg("ppImage", PS_LOG_ERROR, "Can't find recipe %s in the RECIPES.\n", RECIPE_NAME);
        exit(EXIT_FAILURE);
    }
    psMetadata *format = config->format;
    
    // Non-linearity recipe options
    if (psMetadataLookupBool(NULL, recipe, "NONLIN")) {
        psMetadataItem *dataItem = psMetadataLookup(recipe, "NONLIN.DATA");
        if (! dataItem) {
            psLogMsg("ppImage", PS_LOG_ERROR, "Non-linearity correction desired, but unable to find NONLIN.DATA in recipe %s.", RECIPE_NAME);
            exit(EXIT_FAILURE);
        }

        options->doNonLin = true;
        options->nonLinearType = dataItem->type;
        options->nonLinearData = dataItem;

        switch (dataItem->type) {
            // No immediate action required
          case PS_DATA_VECTOR:
          case PS_DATA_STRING:
            break;

            // This is a menu; we need the key
          case PS_DATA_METADATA:
	    options->nonLinearSource = psMetadataLookupStr(&status, recipe, "NONLIN.SOURCE");
	    if (! status || ! options->nonLinearSource) {
		psLogMsg("ppImage", PS_LOG_ERROR, "Non-linearity correction desired, but unable to find NONLIN.SOURCE in recipe %s.", RECIPE_NAME);
		exit(EXIT_FAILURE);
	    }
            break;
          default:
            psLogMsg("ppImage", PS_LOG_ERROR, "Non-linearity correction desired, but NONLIN.DATA is of invalid type in recipe %s.", RECIPE_NAME);
            exit(EXIT_FAILURE);
        }
    }

    // XXX PAP: The overscan stuff needs to be updated following the reworked API

    // New Non-linearity (v 2023) recipe options
    // non-linearity corrections are loaded from a file defined in the detrend system or on the command-line
    if (psMetadataLookupBool(NULL, recipe, "NEWNONLIN")) {
        options->doNewNonLin = true;
    }

    // XXX PAP: The overscan stuff needs to be updated following the reworked API

    // Overscan recipe options
    // XXX EAM : we should abort on invalid options. default options?
    if (psMetadataLookupBool(NULL, recipe, "OVERSCAN")) {
        options->doOverscan = true;

        // Do the overscan as a single value?
        bool overscanSingle = psMetadataLookupBool(NULL, recipe, "OVERSCAN.SINGLE");

        // How do we fit it?
        pmFit overscanFit = PM_FIT_NONE; // Fit type for overscan
        int overscanOrder = 0;          // Order for overscan fit
        psString fit = psMetadataLookupStr(NULL, recipe, "OVERSCAN.FIT");
        if (! strcasecmp(fit, "POLYNOMIAL")) {
            overscanFit = PM_FIT_POLY_ORD;
            overscanOrder = psMetadataLookupS32(NULL, recipe, "OVERSCAN.ORDER");
        } else if (! strcasecmp(fit, "CHEBYSHEV")) {
            overscanFit = PM_FIT_POLY_CHEBY;
            overscanOrder = psMetadataLookupS32(NULL, recipe, "OVERSCAN.ORDER");
        } else if (! strcasecmp(fit, "SPLINE")) {
            overscanFit = PM_FIT_SPLINE;
        } else if (strcasecmp(fit, "NONE")) {
            psLogMsg(__func__, PS_LOG_WARN,
                     "OVERSCAN.FIT (%s) in recipe %s is not one of NONE, POLYNOMIAL, or SPLINE",
                     fit, RECIPE_NAME);
            exit(EXIT_FAILURE);
        }

        // What method do we use to measure the overscan statistics?
        // XXX allow user to specify psStats types by name
        psStats *overscanStats = NULL;  // Statistics for overscan
        psString stat = psMetadataLookupStr(NULL, recipe, "OVERSCAN.STAT");
        if (! strcasecmp(stat, "MEAN")) {
            // overscanStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
            overscanStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        } else if (! strcasecmp(stat, "MEDIAN")) {
            overscanStats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
        } else {
            psErrorStackPrint(stderr, "OVERSCAN.STAT (%s) in recipe %s is not one of MEAN or MEDIAN",
                              stat, RECIPE_NAME);
            exit(EXIT_FAILURE);
        }

	bool mdok;
        int boxcar = psMetadataLookupS32(NULL, recipe, "OVERSCAN.BOXCAR");
        float gauss = psMetadataLookupF32(NULL, recipe, "OVERSCAN.GAUSS");
        float minValid = psMetadataLookupF32(&mdok, recipe, "OVERSCAN.MIN.VALID");
	if (!mdok) { minValid = 0.0; }

        float maxValid = psMetadataLookupF32(&mdok, recipe, "OVERSCAN.MAX.VALID");
	if (!mdok) { maxValid = (float) 0x10000; }

        // Fill in the options
        options->overscan = pmOverscanOptionsAlloc(overscanSingle, overscanFit, overscanOrder,
                                                   overscanStats, boxcar, gauss);

        options->overscan->constant = psMetadataLookupBool(NULL, recipe, "OVERSCAN.CONSTANT");
        options->overscan->value = psMetadataLookupF32(NULL, recipe, "OVERSCAN.VALUE");
        options->overscan->minValid = minValid;
        options->overscan->maxValid = maxValid;
        options->overscan->maskVal  = 0x0001;

        psFree(overscanStats);
    }

    // for these images, even if not required otherwise
    options->doMaskBuild     = psMetadataLookupBool(NULL, recipe, "MASK.BUILD");
    options->doMaskSat       = psMetadataLookupBool(NULL, recipe, "MASK.SATURATED");
    options->doMaskLow       = psMetadataLookupBool(NULL, recipe, "MASK.LOW");
    options->doMaskBurntool  = psMetadataLookupBool(NULL, recipe, "MASK.BURNTOOL");
    options->doApplyBurntool = psMetadataLookupBool(NULL, recipe, "APPLY.BURNTOOL");
    //TdB: read in switch for zero'ing pixels under masks
    options->doApplyPixelZero = psMetadataLookupBool(NULL, recipe, "APPLY.PIXELZERO");
    options->doVarianceBuild = psMetadataLookupBool(NULL, recipe, "VARIANCE.BUILD");
    options->doAuxMask       = psMetadataLookupBool(NULL, recipe, "MASK.AUXMASK");
    if (options->doAuxMask) {
        // if we are applying an auxiliary mask we can optionally apply another
        // mask to video cells only. 
        psString auxVideoMask = psMetadataLookupStr(NULL, recipe, "AUX.VIDEO.MASK");
        // save the value if defined and not the value "NULL"
        if (auxVideoMask && strcmp(auxVideoMask, "NULL")) {
            options->auxVideoMask = psStringCopy(auxVideoMask);
        }
    }


    // Mask recipe options (note that mask bit values are set in ppImageSetMaskBits.c)
    options->doMask = psMetadataLookupBool(NULL, recipe, "MASK");

    // Mask recipe options (note that mask bit values are set in ppImageSetMaskBits.c)
    options->doCrosstalkMeasure = psMetadataLookupBool(NULL, recipe, "CROSSTALK.MEASURE");
    options->doCrosstalkCorrect = psMetadataLookupBool(NULL, recipe, "CROSSTALK.CORRECT");

    options->doNoiseMap = psMetadataLookupBool(NULL, recipe, "NOISEMAP");
    options->doBias = psMetadataLookupBool(NULL, recipe, "BIAS");
    options->doDark = psMetadataLookupBool(NULL, recipe, "DARK");
    options->doRemnance = psMetadataLookupBool(NULL, recipe, "REMNANCE");
    options->doFlat = psMetadataLookupBool(NULL, recipe, "FLAT");
    options->doFringe = psMetadataLookupBool(NULL, recipe, "FRINGE");
    options->doShutter = psMetadataLookupBool(NULL, recipe, "SHUTTER");

    // PATTERN.ROW is selected by the recipe.  If it is selected, we search for the table
    // PATTERN.ROW.SUBSET.  If this is found in our format file, we use that version;
    // otherwise, we use the table provided in the recipe file "ppImage.config".  Within that
    // table, we select the entry that matches our CHIP.NAME.  This will be either a boolean or
    // a string of bits.  If it is a boolean, it specified whether or not to correct the entire
    // chip; if it is a string, the bits specify which cells to correct (sequence is order of
    // CELLS in the format:CHIPS metadata table)

    // We also check the chip header for the boolean 'PTRN_ROW' : if this is true, we have
    // already applied this correct to this data, so we simply skip the correction for this
    // chip.

    options->doPatternRow = psMetadataLookupBool(NULL, recipe, "PATTERN.ROW");
    options->doPatternCell = psMetadataLookupBool(NULL, recipe, "PATTERN.CELL");
    options->doPatternContinuity = psMetadataLookupBool(NULL, recipe, "PATTERN.CONTINUITY");
    options->doPatternDeadCells = psMetadataLookupBool(NULL, recipe, "PATTERN.DEAD.CELLS");

    options->doMaskStats = psMetadataLookupBool(NULL, recipe, "MASK.STATS");
    options->addNoise = psMetadataLookupBool(NULL, recipe, "ADDNOISE");

    options->doStats = false;
    char *statsName = psMetadataLookupStr(&status, config->arguments, "STATS"); // Filename for statistics
    if (statsName) {
        options->doStats = true;
    }

    options->applyParity = psMetadataLookupBool(NULL, recipe, "APPLY.CELL.PARITY");

    options->burntoolTrails = psMetadataLookupS32(&status, recipe, "BURNTOOL.TRAILS");
    psTrace("psModules.detrend", 7, "burntoolTrails: %d BURNTOOL.TRAILS: %d Status: %d\n",
            options->burntoolTrails,psMetadataLookupS32(&status,recipe,"BURNTOOL.TRAILS"),status);
    if (!status) {
      psWarning("BURNTOOL.TRAILS not found in recipe: setting to default value.\n");
    }

    // binned image options
    options->xBin1 = psMetadataLookupS32(&status, recipe, "BIN1.XBIN");
    if (!status) {
        psWarning("BIN1.XBIN not found in recipe: setting to default value.\n");
        options->xBin1 = 4;
    }
    options->yBin1 = psMetadataLookupS32(&status, recipe, "BIN1.YBIN");
    if (!status) {
        psWarning("BIN1.YBIN not found in recipe: setting to default value.\n");
        options->yBin1 = 4;
    }

    options->xBin2 = psMetadataLookupS32(&status, recipe, "BIN2.XBIN");
    if (!status) {
        psWarning("BIN2.XBIN not found in recipe: setting to default value.\n");
       options->xBin1 = 16;
    }
    options->yBin2 = psMetadataLookupS32(&status, recipe, "BIN2.YBIN");
    if (!status) {
        psWarning("BIN2.YBIN not found in recipe: setting to default value.\n");
        options->yBin1 = 16;
    }

    options->BaseFITS       = psMetadataLookupBool(NULL, recipe, "BASE.FITS");
    options->BaseMaskFITS   = psMetadataLookupBool(NULL, recipe, "BASE.MASK.FITS");
    options->BaseVarianceFITS = psMetadataLookupBool(NULL, recipe, "BASE.VARIANCE.FITS");

    options->ChipFITS       = psMetadataLookupBool(NULL, recipe, "CHIP.FITS");
    options->ChipMaskFITS   = psMetadataLookupBool(NULL, recipe, "CHIP.MASK.FITS");
    options->ChipVarianceFITS = psMetadataLookupBool(NULL, recipe, "CHIP.VARIANCE.FITS");

    options->FPA1FITS       = psMetadataLookupBool(NULL, recipe, "FPA1.FITS");
    options->FPA2FITS       = psMetadataLookupBool(NULL, recipe, "FPA2.FITS");

    options->Bin1FITS       = psMetadataLookupBool(NULL, recipe, "BIN1.FITS");
    options->Bin1JPEG       = psMetadataLookupBool(NULL, recipe, "BIN1.JPEG");
    options->Bin2FITS       = psMetadataLookupBool(NULL, recipe, "BIN2.FITS");
    options->Bin2JPEG       = psMetadataLookupBool(NULL, recipe, "BIN2.JPEG");

    options->doPhotom       = psMetadataLookupBool(NULL, recipe, "PHOTOM");
    options->doAstromChip   = psMetadataLookupBool(NULL, recipe, "ASTROM.CHIP");
    options->doAstromMosaic = psMetadataLookupBool(NULL, recipe, "ASTROM.MOSAIC");
    options->doBG           = psMetadataLookupBool(NULL, recipe, "BACKGROUND");

    options->checkCTE       = psMetadataLookupBool(NULL, recipe, "CHECK.CTE");
    options->checkNoise     = psMetadataLookupBool(NULL, recipe, "CHECK.NOISE");
    options->squashNANs     = psMetadataLookupBool(NULL, recipe, "SQUASH.NANS");

    /* doMaskBuild : there are some cases where we require a mask, so we force doMaskBuild to be set even if the user specified 'FALSE'
     *
     * doPhotom : a mask is required because it is used to mark the locations of stars
     *
     * doNoiseMap : no reason this needs to trigger a mask?
     * doBias : no reason this needs to trigger a mask?
     * doOverscan : no reason this needs to trigger a mask?
     * doDark : no reason this needs to trigger a mask?
     * doShutter : no reason this needs to trigger a mask?
     * doFlat : no reason this needs to trigger a mask?
     */

    // if the variance image is requested, build it (if not supplied)
    if (options->BaseVarianceFITS || options->ChipVarianceFITS) {
        options->doVarianceBuild = true;
    } 
    // photometry and noisemap both require a variance image
    if (options->doNoiseMap || options->doPhotom) {
        options->doVarianceBuild = true;
    } 

    // we need a mask if we are going to apply these things:
    if (options->doMaskSat || options->doMaskLow || options->doMaskBurntool || options->doMaskStats) {
        options->doMaskBuild = true;
    }
    // photometry, mask, and background all require a mask image
    if (options->doMask || options->doBG || options->doPhotom) {
        options->doMaskBuild = true;
    }

    if ((options->doAstromChip || options->doAstromMosaic) && !options->doPhotom) {
        psLogMsg(__func__, PS_LOG_ERROR, "Invalid PPIMAGE options: cannot do ASTROMetry without PHOTOMetry");
        exit(EXIT_FAILURE);
    }

    // Fringe options
    options->fringeRej = psMetadataLookupF32(NULL, recipe, "FRINGE.REJ");
    options->fringeIter = psMetadataLookupS32(NULL, recipe, "FRINGE.ITER");
    options->fringeKeep = psMetadataLookupF32(NULL, recipe, "FRINGE.KEEP");

    // Video cell options
    if (psMetadataLookup(recipe, "USE.VIDEO.DARK")) {
      options->useVideoDark = psMetadataLookupBool(NULL,recipe,"USE.VIDEO.DARK");
    }
    if (psMetadataLookup(recipe, "USE.VIDEO.MASK")) {
      options->useVideoMask = psMetadataLookupBool(NULL,recipe,"USE.VIDEO.MASK");
    }

    // Pattern correction
    if (psMetadataLookup(format, "PATTERN.ROW.ORDER")) {
      options->patternRowOrder = psMetadataLookupS32(NULL, format, "PATTERN.ROW.ORDER");
    }
    else {
      options->patternRowOrder = psMetadataLookupS32(NULL, recipe, "PATTERN.ROW.ORDER");
    }
    if (psMetadataLookup(format, "PATTERN.ROW.ITER")) {
      options->patternRowIter = psMetadataLookupS32(NULL, format, "PATTERN.ROW.ITER");
    }
    else {
      options->patternRowIter = psMetadataLookupS32(NULL, recipe, "PATTERN.ROW.ITER");
    }
    if (psMetadataLookup(format, "PATTERN.ROW.REJ")) {
      options->patternRowRej = psMetadataLookupF32(NULL, format, "PATTERN.ROW.REJ");
    }
    else {
      options->patternRowRej = psMetadataLookupF32(NULL, recipe, "PATTERN.ROW.REJ");
    }
    if (psMetadataLookup(format, "PATTERN.ROW.THRESH")) {
      options->patternRowThresh = psMetadataLookupF32(NULL, format, "PATTERN.ROW.THRESH");
    }
    else {
      options->patternRowThresh = psMetadataLookupF32(NULL, recipe, "PATTERN.ROW.THRESH");
    }
    if (psMetadataLookup(format, "PATTERN.ROW.MEAN")) {
      options->patternRowMean = psStatsOptionFromString(psMetadataLookupStr(NULL, format, "PATTERN.ROW.MEAN"));
    }
    else {
      options->patternRowMean = psStatsOptionFromString(psMetadataLookupStr(NULL, recipe, "PATTERN.ROW.MEAN"));
    }
    if (psMetadataLookup(format, "PATTERN.ROW.STDEV")) {
      options->patternRowStdev = psStatsOptionFromString(psMetadataLookupStr(NULL, format, "PATTERN.ROW.STDEV"));
    }
    else {
      options->patternRowStdev = psStatsOptionFromString(psMetadataLookupStr(NULL, recipe, "PATTERN.ROW.STDEV"));
    }
    if (psMetadataLookup(format, "PATTERN.CELL.BG")) {
      options->patternCellBG = psStatsOptionFromString(psMetadataLookupStr(NULL, format, "PATTERN.CELL.BG"));
    }
    else {
      options->patternCellBG = psStatsOptionFromString(psMetadataLookupStr(NULL, recipe, "PATTERN.CELL.BG"));
    }
    if (psMetadataLookup(format, "PATTERN.CELL.MEAN")) {
      options->patternCellMean = psStatsOptionFromString(psMetadataLookupStr(NULL, format, "PATTERN.CELL.MEAN"));
    }
    else {
      options->patternCellMean = psStatsOptionFromString(psMetadataLookupStr(NULL, recipe, "PATTERN.CELL.MEAN"));
    }

    if (psMetadataLookup(format, "PATTERN.CONTINUITY.WIDTH")) {
      options->patternContinuityEdgeWidth = psMetadataLookupS32(NULL, format, "PATTERN.CONTINUITY.WIDTH");
    }
    else {
      options->patternContinuityEdgeWidth = psMetadataLookupS32(NULL, recipe, "PATTERN.CONTINUITY.WIDTH");
    }

    // Option to enable the background continuity
    options->doBackgroundContinuity = psMetadataLookupBool(NULL, recipe, "BACKGROUND.CONTINUITY");


    // Remnance options
    options->remnanceSize = psMetadataLookupF32(NULL, recipe, "REMNANCE.SIZE");
    options->remnanceThresh = psMetadataLookupS32(NULL, recipe, "REMNANCE.THRESH");

    // per-class normalization source (just a reference; don't free)
    options->normClass = psMetadataLookupStr(NULL, recipe, "NORM.CLASS");

    options->maskstat_static   = psMetadataLookupU16(NULL, recipe, "MASKSTAT.STATIC");
    options->maskstat_dynamic  = psMetadataLookupU16(NULL, recipe, "MASKSTAT.DYNAMIC");
    options->maskstat_magic    = psMetadataLookupU16(NULL, recipe, "MASKSTAT.MAGIC");
    options->maskstat_advisory = psMetadataLookupU16(NULL, recipe, "MASKSTAT.ADVISORY");

    
    return options;
}
