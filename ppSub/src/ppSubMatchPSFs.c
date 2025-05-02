/** @file ppSubMatchPSFs.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

#define COVAR_FRAC 0.01                 // Truncation fraction for covariance matrix

// Normalise a region on an image
static void normaliseRegion(psImage *image, // Image to normalise
                            const psRegion *region, // Region of image to normalise
                            float norm  // Normalisation
                            )
{
    if (!image) {
        return;
    }
    psAssert(region, "Expect region");
    psImage *subImage = psImageSubset(image, *region); // Sub-image
    psBinaryOp(subImage, subImage, "*", psScalarAlloc(norm, PS_TYPE_F32));
    psFree(subImage);
    return;
}

// Measure the PSF for an image
static float subImagePSF(ppSubData *data, // Processing data
                         const pmReadout *ro, // Readout for which to measure PSF
                         psArray *sources     // Sources with positions at which to measure PSF
    )
{
    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    psAssert(ro, "Need readout");
    psAssert(sources, "Need sources.");

    pmFPAfile *photFile = psMetadataLookupPtr(NULL, config->files, "PSPHOT.INPUT"); // Photometry file
    psAssert(photFile, "Need photometry file.");
    if (!pmFPACopy(photFile->fpa, ro->parent->parent->parent)) {
        psError(PPSUB_ERR_CONFIG, false, "Unable to copy FPA for photometry");
        return NAN;
    }

    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *photRO = pmFPAviewThisReadout(view, photFile->fpa); // Readout to photometer

    if (psMetadataLookup(photRO->analysis, "PSPHOT.DETECTIONS")) {
        psMetadataRemoveKey(photRO->analysis, "PSPHOT.DETECTIONS");
    }
    if (psMetadataLookup(photRO->parent->parent->analysis, "PSPHOT.PSF")) {
        psMetadataRemoveKey(photRO->parent->parent->analysis, "PSPHOT.PSF");
    }

    // Extract the loaded sources from the associated readout, and generate PSF
    // Here, we assume the image is background-subtracted
    if (!psphotReadoutFindPSF(config, view, "PSPHOT.INPUT", sources)) {
        psErrorStackPrint(stderr, "Unable to determine PSF");
        psWarning("Unable to determine PSF.");
        psFree(view);
        return NAN;
    }
    psFree(view);

    psMetadata *header = psMetadataLookupMetadata(NULL, photRO->analysis, "PSPHOT.HEADER");
    psAssert(header, "Require header.");
    float fwhm = psMetadataLookupF32(NULL, header, "FWHM_MAJ");

    if (psMetadataLookup(photRO->analysis, "PSPHOT.DETECTIONS")) {
        psMetadataRemoveKey(photRO->analysis, "PSPHOT.DETECTIONS");
    }
    if (psMetadataLookup(photRO->parent->parent->analysis, "PSPHOT.PSF")) {
        psMetadataRemoveKey(photRO->parent->parent->analysis, "PSPHOT.PSF");
    }

    return fwhm;
}

// Scale the kernel parameters according to the PSFs
static bool subScaleKernel(ppSubData *data, // Processing data
                           psVector *kernelWidths, // Widths for kernel
                           int *kernelSize,        // Size of kernel
                           int *stampSize          // Size of stamps (footprint)
    )
{
    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    // Nothing to do if pre-calculated kernel exists
    pmFPAview *view = ppSubViewReadout(); // View to readout
    pmReadout *kernelRO = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT.KERNELS"); // RO with kernel
    if (kernelRO) {
        psFree(view);
        return true;
    }

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");

    // Input images
    pmReadout *inRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT"); // Input readout
    pmReadout *refRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF"); // Reference readout

    // Input sources
    pmReadout *inSourceRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.SOURCES");
    pmReadout *refSourceRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.SOURCES");
    if (!inSourceRO || !refSourceRO) {
        psError(PPSUB_ERR_DATA, false, "Unable to scale kernel, since no sources were provided.");
        psFree(view);
        return false;
    }

    pmDetections *inDetections = psMetadataLookupPtr(NULL, inSourceRO->analysis, "PSPHOT.DETECTIONS"); // Input sources
    pmDetections *refDetections = psMetadataLookupPtr(NULL, refSourceRO->analysis, "PSPHOT.DETECTIONS"); // Ref sources

    psFree(view);

    if (!inDetections || !refDetections) {
        psError(PPSUB_ERR_DATA, false, "Unable to set FWHM or scale kernel, since no sources were provided.");
        return false;
    }

    psArray *inSources = inDetections->allSources;
    psAssert (inSources, "missing inSources?");

    psArray *refSources = refDetections->allSources;
    psAssert (refSources, "missing refSources?");

    float inFWHM = psMetadataLookupF32(NULL, inRO->parent->parent->concepts, "CHIP.SEEING"); // FWHM for input
    if (!isfinite(inFWHM) || inFWHM == 0.0) {
        inFWHM = subImagePSF(data, inRO, inSources);
    }
    float refFWHM = psMetadataLookupF32(NULL, refRO->parent->parent->concepts, "CHIP.SEEING"); // FWHM for ref
    if (!isfinite(refFWHM) || refFWHM == 0.0) {
        refFWHM = subImagePSF(data, refRO, refSources);
    }
    psLogMsg("ppSub", PS_LOG_INFO, "Input FWHM: %f\nReference FWHM: %f\n", inFWHM, refFWHM);
    if (!isfinite(inFWHM) || !isfinite(refFWHM)) {
#ifdef SET_QUALITY_INSTEAD_OF_ERROR
        psErrorStackPrint(stderr, "Cannot determine FHWM for images, giving up.");
        int error = psErrorCodeLast(); // Error code
        ppSubDataQuality(data, error, PPSUB_FILES_ALL);
        return true;
#else
        psError(PPSUB_ERR_DATA, false, "Cannot determine FHWM for images, giving up.");
        return false;
#endif
    }

    // we need to register the FWHM values for use downstream
    pmSubtractionSetFWHMs(inFWHM, refFWHM);

    // is auto-scaling needed?
    bool scale = psMetadataLookupBool(NULL, recipe, "SCALE");
    float scaleRef = psMetadataLookupF32(NULL, recipe, "SCALE.REF"); // Reference for scaling
    float scaleMin = psMetadataLookupF32(NULL, recipe, "SCALE.MIN"); // Minimum for scaling
    float scaleMax = psMetadataLookupF32(NULL, recipe, "SCALE.MAX"); // Maximum for scaling

    if (scale && (!isfinite(scaleRef) || !isfinite(scaleMin) || !isfinite(scaleMax))) {
        psError(PPSUB_ERR_ARGUMENTS, false,
                "auto-scale selected but scale parameters (SCALE.REF=%f, SCALE.MIN=%f, SCALE.MAX=%f) not set in recipe.",
                scaleRef, scaleMin, scaleMax);
        return false;
    }

    pmSubtractionParamScaleOptions(scale, scaleRef, scaleMin, scaleMax);

    // if (!pmSubtractionParamsScale(kernelSize, stampSize, kernelWidths)) {
    //     psError(PPSUB_ERR_DATA, false, "Unable to scale parameters.");
    //     return false;
    // }

    return true;
}

pmSubtractionMode subModeFromString (char *string) {

    if (!strcasecmp(string, "AUTO")) return PM_SUBTRACTION_MODE_UNSURE;
    if (!strcasecmp(string, "DUAL")) return PM_SUBTRACTION_MODE_DUAL;
    if (!strcasecmp(string, "SINGLE_AUTO")) return PM_SUBTRACTION_MODE_SINGLE_AUTO;
    if (!strcasecmp(string, "SINGLE1")) return PM_SUBTRACTION_MODE_1;
    if (!strcasecmp(string, "SINGLE2")) return PM_SUBTRACTION_MODE_2;
    if (!strcasecmp(string, "1")) return PM_SUBTRACTION_MODE_1;
    if (!strcasecmp(string, "2")) return PM_SUBTRACTION_MODE_2;

    return PM_SUBTRACTION_MODE_UNSURE;
}

bool ppSubMatchPSFs(ppSubData *data)
{
    bool mdok = false;

    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSUB_RECIPE); // Recipe for ppSim
    psAssert(recipe, "We checked this earlier, so it should be here.");

    bool noConvolve = psMetadataLookupBool(&mdok, recipe, "NOCONVOLVE"); // Do not use convolved images.
    if (noConvolve) {
        psWarning("not matching PSFs because NOCONVOLVE is TRUE\n");
        return true;
    }

    pmFPAview *view = ppSubViewReadout(); // View to readout

    // Input images
    pmReadout *inRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT"); // Input readout
    pmReadout *refRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF"); // Reference readout

    // Output image holders
    pmReadout *inConv = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.CONV"); // Input convolved
    if (!inConv) {
        pmCell *cell = pmFPAfileThisCell(config->files, view, "PPSUB.INPUT.CONV"); // Cell for convolved input
        inConv = pmReadoutAlloc(cell);
        psFree(inConv);
    }
    pmReadout *refConv = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.CONV"); // Reference convolved
    if (!refConv) {
        pmCell *cell = pmFPAfileThisCell(config->files, view, "PPSUB.REF.CONV"); // Cell for convolved ref.
        refConv = pmReadoutAlloc(cell);
        psFree(refConv);
    }

    // Load pre-calculated kernel, if available
    pmReadout *kernelRO = pmFPAfileThisReadout(config->files, view, "PPSUB.OUTPUT.KERNELS"); // RO with kernel

    // Sources in image, used for stamps: these must be loaded from previous analysis stages
    pmReadout *inSourceRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.SOURCES");
    pmReadout *refSourceRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.SOURCES");

    pmDetections *inDetections  = inSourceRO  ? psMetadataLookupPtr(&mdok, inSourceRO->analysis,  "PSPHOT.DETECTIONS") : NULL; // Input sources
    pmDetections *refDetections = refSourceRO ? psMetadataLookupPtr(&mdok, refSourceRO->analysis, "PSPHOT.DETECTIONS") : NULL; // Ref sources

    psFree(view);

    pmDetections *detections = NULL;    // Merged detection set
    if (inDetections && refDetections) {
        psArray *inSources  = inDetections->allSources;
        psArray *refSources = refDetections->allSources;

        psAssert (inSources, "missing in sources?");
        psAssert (refSources, "missing ref sources?");

        detections = pmDetectionsAlloc();
        float radius = psMetadataLookupF32(NULL, recipe, "SOURCE.RADIUS"); // Matching radius
        psArray *lists = psArrayAlloc(2); // Source lists
        lists->data[0] = psMemIncrRefCounter(inSources);
        lists->data[1] = psMemIncrRefCounter(refSources);
        // XXX MEH changed to get only match (true to cull single), no apparent need of unmatched sources and can cause trouble
        detections->allSources = pmSourceMatchMerge(lists, radius, true);
        psFree(lists);
        if (!detections->allSources) {
            psFree(detections);
            int error = psErrorCodeLast(); // Error code
            if (error == PM_ERR_OBJECTS) {
                psErrorStackPrint(stderr, "Unable to match source lists");
                psWarning("Unable to match source lists --- suspect bad data quality.");
                ppSubDataQuality(data, error, PPSUB_FILES_ALL);
                return true;
            } else {
                psError(error, false, "Unable to merge source lists");
                return false;
            }
        }
    }
    if (!detections && inDetections) {
        detections = psMemIncrRefCounter(inDetections);
    }
    if (!detections && refDetections) {
        detections = psMemIncrRefCounter(refDetections);
    }

    psMetadataAddPtr(inConv->analysis,  PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "Merged source list", detections);
    psMetadataAddPtr(refConv->analysis, PS_LIST_TAIL, "PSPHOT.DETECTIONS", PS_META_REPLACE | PS_DATA_UNKNOWN, "Merged source list", detections);
    psFree(detections);                    // Drop reference

    int footprint = psMetadataLookupS32(NULL, recipe, "STAMP.FOOTPRINT"); // Stamp half-size
    int stride = psMetadataLookupS32(NULL, recipe, "STRIDE"); // Size of convolution patches
    float regionSize = psMetadataLookupF32(NULL, recipe, "REGION.SIZE"); // Size of iso-kernel regs
    float spacing = psMetadataLookupF32(NULL, recipe, "STAMP.SPACING"); // Typical stamp spacing
    float threshold = psMetadataLookupF32(NULL, recipe, "STAMP.THRESHOLD"); // Threshold for stmps

    const char *typeStr = psMetadataLookupStr(NULL, recipe, "KERNEL.TYPE"); // Kernel type
    psAssert(typeStr, "We put it here in ppSubArguments.c");
    pmSubtractionKernelsType type = pmSubtractionKernelsTypeFromString(typeStr); // Type of kernel
    if (type == PM_SUBTRACTION_KERNEL_NONE) {
        psError(PPSUB_ERR_ARGUMENTS, true, "Unrecognised kernel type: %s", typeStr);
        return false;
    }

    int size = psMetadataLookupS32(NULL, recipe, "KERNEL.SIZE"); // Kernel half-size
    int order = psMetadataLookupS32(NULL, recipe, "SPATIAL.ORDER"); // Spatial polynomial order
    psVector *widths = psMetadataLookupPtr(NULL, recipe, "ISIS.WIDTHS"); // ISIS Gaussian widths
    psVector *orders = psMetadataLookupPtr(NULL, recipe, "ISIS.ORDERS"); // ISIS Polynomial orders

    int inner = psMetadataLookupS32(NULL, recipe, "INNER"); // Inner radius
    int ringsOrder = psMetadataLookupS32(NULL, recipe, "RINGS.ORDER"); // RINGS polynomial order
    int binning = psMetadataLookupS32(NULL, recipe, "SPAM.BINNING"); // Binning for SPAM kernel
    float penalty = psMetadataLookupF32(NULL, recipe, "PENALTY"); // Penalty for wideness

    int iter = psMetadataLookupS32(NULL, recipe, "ITER"); // Rejection iterations
    float rej = psMetadataLookupF32(NULL, recipe, "REJ"); // Rejection threshold
    float kernelErr = psMetadataLookupF32(NULL, recipe, "KERNEL.ERR"); // Relative systematic error in kernel
    float normFrac = psMetadataLookupF32(NULL, recipe, "NORM.FRAC"); // Fraction of window for normalisn windw
    float sysErr = psMetadataLookupF32(NULL, recipe, "SYS.ERR"); // Relative systematic error in images
    float skyErr = psMetadataLookupF32(NULL, recipe, "SKY.ERR"); // Additional error in sky
    float covarFrac = psMetadataLookupF32(NULL, recipe, "COVAR.FRAC"); // Fraction for covariance calculation

    float badFrac = psMetadataLookupF32(NULL, recipe, "BADFRAC"); // Maximum bad fraction
    float poorFrac = psMetadataLookupF32(&mdok, recipe, "POOR.FRACTION"); // Fraction for "poor"

    // Options which control the generation of optimal parameters
    bool optimum = psMetadataLookupBool(&mdok, recipe, "OPTIMUM"); // Derive optimum parameters?
    float optMin = psMetadataLookupF32(&mdok, recipe, "OPTIMUM.MIN"); // Minimum width for search
    float optMax = psMetadataLookupF32(&mdok, recipe, "OPTIMUM.MAX"); // Maximum width for search
    float optStep = psMetadataLookupF32(&mdok, recipe, "OPTIMUM.STEP"); // Step for search
    psVector *optWidths = NULL;         // Vector with FWHMs for optimum search
    if (optimum) {
        optWidths = psVectorCreate(optWidths, optMin, optMax, optStep, PS_TYPE_F32);
    }
    int optOrder = psMetadataLookupS32(&mdok, recipe, "OPTIMUM.ORDER"); // Order for search
    float optThresh = psMetadataLookupF32(&mdok, recipe, "OPTIMUM.TOL"); // Tolerance for search

    psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Bits to mask in inputs
    psImageMaskType maskPoor = pmConfigMaskGet("CONV.POOR", config); // Bits to mask for poor pixels
    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels

    // we have three things to control the convolution choice:

    // 1) in the recipe, the keyword DUAL can be TRUE or FALSE.  if TRUE, DUAL is used. If not
    // DUAL, then we are doing SINGLE convolution (but see CONVOLVE.TARGET below).

    // 2) if the -convolve option is given on the command line, the specified input (1 or 2) is
    // the one convolved.  Allowed values are 1 or 2.

    // 3) if the -convolve option is NOT given, then the value of the recipe keyword
    // CONVOLVE.TARGET is used.  this may have the value DUAL, AUTO, SINGLE1, SINGLE2, 1, 2.
    // thus, DUAL convolution can be turned on with either DUAL = T or CONVOLVE.TARGET = DUAL

    bool dual = psMetadataLookupBool(&mdok, recipe, "DUAL"); // Dual convolution?
    pmSubtractionMode subMode;          // Subtraction mode
    if (dual) {
        subMode = PM_SUBTRACTION_MODE_DUAL;
    } else {
        char *convolveName = psMetadataLookupStr(&mdok, recipe, "CONVOLVE.TARGET"); // recipe value for target
        int convolve = psMetadataLookupS32(&mdok, config->arguments, "-convolve"); // override with command-line option
        switch (convolve) {
          case 0:
            // convolve is 0 if it is not supplied on the command line
            subMode = subModeFromString(convolveName);
            break;
          case 1:
            subMode = PM_SUBTRACTION_MODE_1;
            break;
          case 2:
            subMode = PM_SUBTRACTION_MODE_2;
            break;
          default:
            psError(PPSUB_ERR_ARGUMENTS, false, "Invalid value for -convolve");
            return false;
        }
    }

    if (!subScaleKernel(data, widths, &size, &footprint)) {
        psError(PPSUB_ERR_DATA, false, "Unable to scale kernel parameters");
        return false;
    }

    int threads = psMetadataLookupS32(NULL, config->arguments, "NTHREADS"); // Number of threads
    if (threads > 0) {
        pmSubtractionThreadsInit();
    }

    if (inRO->covariance) {
        psKernel *truncated = psImageCovarianceTruncate(inRO->covariance, COVAR_FRAC);
        psFree(inRO->covariance);
        inRO->covariance = truncated;
    }
    if (refRO->covariance) {
        psKernel *truncated = psImageCovarianceTruncate(refRO->covariance, COVAR_FRAC);
        psFree(refRO->covariance);
        refRO->covariance = truncated;
    }

    // Data doesn't exist until it's created in pmSubtraction...
    inConv->data_exists = inConv->parent->data_exists = inConv->parent->parent->data_exists = false;
    refConv->data_exists = refConv->parent->data_exists = refConv->parent->parent->data_exists = false;

    // Match the PSFs
    bool success = false;               // Operation was successful?
    if (kernelRO) {
        success = pmSubtractionMatchPrecalc(inConv, refConv, inRO, refRO, kernelRO->analysis,
                                            stride, kernelErr, covarFrac, maskVal, maskBad, maskPoor,
                                            poorFrac, badFrac);
    } else {
        success = pmSubtractionMatch(inConv, refConv, inRO, refRO, footprint, stride, regionSize,
                                     spacing, threshold, detections ? detections->allSources : NULL,
                                     data->stamps, type, size, order, widths, orders, inner, ringsOrder,
                                     binning, penalty, optimum, optWidths, optOrder, optThresh, iter, rej,
                                     normFrac, sysErr, skyErr, kernelErr, covarFrac,
                                     maskVal, maskBad, maskPoor, poorFrac, badFrac, subMode);
    }

# ifdef TESTING
    // XXX for testing
    psphotSaveImage (NULL, refRO->image,    "refRO.im.fits");
    psphotSaveImage (NULL, refRO->variance, "refRO.wt.fits");
    psphotSaveImage (NULL, refRO->mask,     "refRO.mk.fits");

    psphotSaveImage (NULL, inRO->image,    "inRO.im.fits");
    psphotSaveImage (NULL, inRO->variance, "inRO.wt.fits");
    psphotSaveImage (NULL, inRO->mask,     "inRO.mk.fits");

    psphotSaveImage (NULL, inConv->image,    "inConv.im.fits");
    psphotSaveImage (NULL, inConv->variance, "inConv.wt.fits");
    psphotSaveImage (NULL, inConv->mask,     "inConv.mk.fits");

    psphotSaveImage (NULL, refConv->image,    "refConv.im.fits");
    psphotSaveImage (NULL, refConv->variance, "refConv.wt.fits");
    psphotSaveImage (NULL, refConv->mask,     "refConv.mk.fits");
# endif

    psFree(optWidths);
    pmSubtractionThreadsFinalize();

    if (!success) {
        int error = psErrorCodeLast(); // Error code
        if (error == PM_ERR_STAMPS) {
            psErrorStackPrint(stderr, "Unable to find stamps");
            psWarning("Unable to find stamps --- suspect bad data quality.");
            ppSubDataQuality(data, error, PPSUB_FILES_ALL);
            return true;
        } else if (error == PM_ERR_SMALL_AREA) {
            psErrorStackPrint(stderr, "Insufficient area for PSF matching");
            psWarning("Insufficient area for PSF matching --- suspect bad data quality.");
            ppSubDataQuality(data, error, PPSUB_FILES_ALL);
            return true;
        // XX } else if (error == PM_ERR_DATA) {
        // XX     psErrorStackPrint(stderr, "Unable to solve for the kernel to match images");
        // XX     psWarning("Failed to find PSF match kernel --- suspect bad data quality.");
        // XX     ppSubDataQuality(data, error, PPSUB_FILES_ALL);
        // XX     return true;
        } else {
            psError(PPSUB_ERR_DATA, false, "Unable to match images.");
            return false;
        }
    }

    // Need to be careful with the normalisation
    // We will normalise everything to the normalisation of the *input* image
    {
        // Since the entries are MULTI, we have to retrieve them differently
        psMetadataIterator *regIter = psMetadataIteratorAlloc(inConv->analysis, PS_LIST_HEAD,
                                                              "^" PM_SUBTRACTION_ANALYSIS_REGION "$");
        psMetadataIterator *modeIter = psMetadataIteratorAlloc(inConv->analysis, PS_LIST_HEAD,
                                                              "^" PM_SUBTRACTION_ANALYSIS_MODE "$");
        psMetadataIterator *normIter = psMetadataIteratorAlloc(inConv->analysis, PS_LIST_HEAD,
                                                              "^" PM_SUBTRACTION_ANALYSIS_NORM "$");
        psMetadataItem *regItem;        // Item with region
        while ((regItem = psMetadataGetAndIncrement(regIter))) {
            psAssert(regItem->type == PS_DATA_REGION && regItem->data.V, "Expect region type");
            psRegion *region = regItem->data.V; // Region of interest
            psMetadataItem *modeItem = psMetadataGetAndIncrement(modeIter); // Item with mode
            psAssert(modeItem && modeItem->type == PS_DATA_S32, "Expect subtraction mode");
            pmSubtractionMode mode = modeItem->data.S32; // Subtraction mode
            psMetadataItem *normItem = psMetadataGetAndIncrement(normIter); // Item with normalisation
            psAssert(normItem && normItem->type == PS_DATA_F32 && isfinite(normItem->data.F32),
                     "Expect normalisation");
            float norm = normItem->data.F32; // Normalisation

            switch (mode) {
              case PM_SUBTRACTION_MODE_1: // Convolved the input to match template
              case PM_SUBTRACTION_MODE_DUAL: // Convolved both; template should have flux conserved
                psLogMsg("ppSub", PS_LOG_INFO, "Correcting image for normalisation of %f\n", norm);
                normaliseRegion(inConv->image, region, 1.0 / norm);
                normaliseRegion(refConv->image, region, 1.0 / norm);
                normaliseRegion(inConv->variance, region, 1.0 / PS_SQR(norm));
                normaliseRegion(refConv->variance, region, 1.0 / PS_SQR(norm));
                break;
              case PM_SUBTRACTION_MODE_2:       // Convolved the template to match input
                // We're already happy!
                psLogMsg("ppSub", PS_LOG_INFO, "Image normalisation is correct\n");
                break;
              default:
                psAbort("Invalid subtraction mode: %x", mode);
            }
        }
        psFree(regIter);
        psFree(modeIter);
        psFree(normIter);
    }


    pmConceptsCopyFPA(inConv->parent->parent->parent, inRO->parent->parent->parent, true, true);
    pmConceptsCopyFPA(refConv->parent->parent->parent, refRO->parent->parent->parent, true, true);

    if (inConv->covariance) {
        psKernel *truncated = psImageCovarianceTruncate(inConv->covariance, COVAR_FRAC);
        psFree(inConv->covariance);
        inConv->covariance = truncated;
    }
    if (refConv->covariance) {
        psKernel *truncated = psImageCovarianceTruncate(refConv->covariance, COVAR_FRAC);
        psFree(refConv->covariance);
        refConv->covariance = truncated;
    }

    if (inConv->variance) {
        psImageCovarianceTransfer(inConv->variance, inConv->covariance);
    }
    if (refConv->variance) {
        psImageCovarianceTransfer(refConv->variance, refConv->covariance);
    }

    return true;
}
