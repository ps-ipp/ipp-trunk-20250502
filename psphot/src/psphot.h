/* This file defines the library functions available to external programs.  It must be included
 * by programs which are compiled against psphot functions.
 */

#ifndef PSPHOT_H
#define PSPHOT_H

#include <psmodules.h>
#include "psphotErrorCodes.h"

#define PSPHOT_RECIPE "PSPHOT" // Name of the recipe to use

#define PSPHOT_RECIPE_PSF_FAKE_ALLOW "PSF.FAKE.ALLOW" // Name for recipe component permitting fake PSFs

# define READOUT_OR_INTERNAL(VIEW,FILE)((FILE)->mode == PM_FPA_MODE_INTERNAL) ? (FILE)->readout : pmFPAviewThisReadout((VIEW), (FILE)->fpa)

typedef enum {
    PSPHOT_SINGLE,
    PSPHOT_FORCED,
    PSPHOT_FULL_FORCE,
    PSPHOT_MAKE_PSF,
    PSPHOT_MODEL_TEST,
    PSPHOT_MINIMAL,
} psphotImageLoopMode;

// top-level psphot functions
const char     *psphotCVSName(void);
psString        psphotVersion(void);
psString        psphotSource(void);
psString        psphotVersionLong(void);
bool            psphotVersionHeader(psMetadata *header);
bool            psphotVersionHeaderFull(psMetadata *header);
void            psphotVersionPrint(void);

bool            psphotImageLoop (pmConfig *config, psphotImageLoopMode mode);

bool            psphotModelTest (pmConfig *config, const pmFPAview *view, psMetadata *recipe);
bool            psphotInit (void);
bool            psphotReadout (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotReadoutFindPSF(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources);
bool            psphotReadoutKnownSources(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources);
bool            psphotReadoutForcedKnownSources(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources);
bool            psphotReadoutMinimal(pmConfig *config, const pmFPAview *view, const char *filerule);

bool            psphotReadoutCleanup (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotReadoutCleanupReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotReadoutCleanupMinimal (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotReadoutCleanupReadoutMinimal (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotDefineFiles (pmConfig *config, pmFPAfile *input);
void            psphotFilesActivate(pmConfig *config, bool state);

bool            psphotSetMaskBits (pmConfig *config);
bool            psphotSetMaskRecipe (pmConfig *config, psImageMaskType maskValue, psImageMaskType markValue);

// XXX test functions
psArray        *psphotFakeSources (void);
bool            psphotMaskCosmicRayFootprintCheck (psArray *sources);

// psphotReadout functions
bool            psphotAddPhotcode (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotAddPhotcodeReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool            psphotSetMaskAndVariance (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotSetMaskAndVarianceReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotUpdateVariance (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotUpdateVarianceReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotModelBackground (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotModelBackgroundReadoutFileIndex (pmConfig *config, const pmFPAview *view, const char *filerule, int index);
bool            psphotModelBackground_Threaded (psThreadJob *job);
bool            psphotLoadBackgroundModel (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotLoadBackgroundModelReadoutFileIndex (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool            psphotMaskBackground (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotMaskBackgroundReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotSubtractBackground (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotSubtractBackgroundReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotFindDetections (pmConfig *config, const pmFPAview *view, const char *filerule, bool firstPass);
bool            psphotFindDetectionsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool firstPass);

bool            psphotSourceStats (pmConfig *config, const pmFPAview *view, const char *filerule, bool setWindow);
bool            psphotSourceStatsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool setWindow);

bool            psphotFullForceSourceStats (pmConfig *config, const pmFPAview *view, const char *filerule, bool setWindow);
bool            psphotFullForceSourceStatsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool setWindow);

bool            psphotDeblendSatstars (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotDeblendSatstarsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotBasicDeblend (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotBasicDeblendReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool            psphotRoughClass (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotRoughClassReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotRoughClassRegion (int nRegion, psRegion *region, psArray *sources, psMetadata *analysis, psMetadata *recipe, const bool havePSF);

bool            psphotImageQuality (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotImageQualityReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotChoosePSF (pmConfig *config, const pmFPAview *view, const char *filerule, bool newSources);
bool            psphotChoosePSFReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool newSources);

bool            psphotGuessModels (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotGuessModelsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool            psphotMergeSources (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotMergeSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool            psphotFitSourcesLinear (pmConfig *config, const pmFPAview *view, const char *filerule, bool final, bool skipNegativeFluxSources);
bool            psphotFitSourcesLinearReadout (psMetadata *recipe, pmReadout *readout, psArray *sources, pmPSF *psf, bool final, pmSourceFitVarMode fitVarMode, bool skipNegativeFluxSources);

bool            psphotSourceSize (pmConfig *config, const pmFPAview *view, const char *filerule, bool getPSFsize);
bool            psphotSourceSizeReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool getPSFsize);

bool            psphotBlendFit (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotBlendFitReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotBlendFit_Threaded (psThreadJob *job);

bool            psphotReplaceAllSources (pmConfig *config, const pmFPAview *view, const char *filerule, bool ignoreState);
bool            psphotReplaceAllSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool ignoreState);

bool            psphotAddNoise (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotSubNoise (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotAddOrSubNoise (pmConfig *config, const pmFPAview *view, const char *filerule, bool add);
bool            psphotAddOrSubNoiseReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool add);
bool            psphotAddOrSubNoise_Threaded (psThreadJob *job);

bool            psphotChooseAnalysisOptions (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotChooseAnalysisOptionsReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotChooseAnalysisOptionsByObject(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects);

bool            psphotExtendedSourceAnalysis (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotExtendedSourceAnalysisReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotExtendedSourceAnalysis_Threaded (psThreadJob *job);

bool            psphotExtendedSourceFits (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotExtendedSourceFitsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotExtendedSourceFits_Threaded (psThreadJob *job);

bool            psphotGalaxyParams (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotGalaxyParamsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotGalaxyParams_Threaded (psThreadJob *job);

bool            psphotApResid (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotApResidReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotMagnitudes (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotMagnitudesReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, pmReadout *readout, psArray *sources, pmPSF *psf, int index);
bool            psphotMagnitudes_Threaded (psThreadJob *job);

bool            psphotLensing (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotLensingReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);
bool            psphotLensingPSFtrendsReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotEfficiency (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotEfficiencyReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool            psphotPSFWeights(pmConfig *config, pmReadout *readout, const pmFPAview *view, psArray *sources);
bool            psphotPSFWeights_Threaded (psThreadJob *job);

bool            psphotSkyReplace (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotSkyReplaceReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool            psphotSourceFreePixels (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotSourceFreePixelsReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index);

// in psphotSourceStats.c:
bool            psphotSourceStats_Threaded (psThreadJob *job);
bool            psphotSourceStatsUpdate (psArray *sources, pmConfig *config, pmReadout *readout);
bool            psphotSetMomentsWindow (psMetadata *recipe, psMetadata *analysis, psArray *sources, psImageMaskType maskVal);

// in psphotChoosePSF.c:
bool            psphotPSFstats (pmReadout *readout, pmPSF *psf);
bool            psphotPSFstatsSources (pmReadout *readout, psArray *sources, pmPSF *psf);
bool            psphotMomentsStats (pmReadout *readout, psArray *sources);

bool            psphotKronMasked (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotKronMaskedReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, pmReadout *readout, psArray *sources, pmPSF *psf);

// in psphotGuessModel.c
bool            psphotGuessModel_Threaded (psThreadJob *job);

// in psphotMergeSources.c:
bool            psphotLoadExtSources (pmConfig *config, const pmFPAview *view, const char *filerule);
psArray        *psphotLoadPSFSources (pmConfig *config, const pmFPAview *view);
bool            psphotRepairLoadedSources (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotCheckExtSources (pmConfig *config, const pmFPAview *view, const char *filerule);

// generate the detection structure for the supplied array of sources
bool            psphotDetectionsFromSources (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *sources);

// generate the detection structure for the supplied array of sources
bool            psphotSetSourceParams (pmConfig *config, psArray *sources, pmPSF *psf);

// in psphotModelBackground.c:
// Create a background model for a readout, without saving the result as a pmFPAfile on config->files.  Otherwise identical to psphotModelBackgroundFileIndex.
psImage        *psphotModelBackgroundReadoutNoFile(pmReadout *readout, const pmConfig *config);
psImageBinning *psphotBackgroundBinning(const psImage *image, const pmConfig *config);

// in psphotReplaceUnfit.c:
bool            psphotRemoveAllSourcesByArray (const psArray *sources, const psMetadata *recipe);
bool            psphotRemoveAllSources (pmConfig *config, const pmFPAview *view, const char *filerule, bool ignoreState);
bool            psphotRemoveAllSourcesReadout (pmConfig *config, const pmFPAview *view, const char *filename, int index, psMetadata *recipe, bool ignoreState);
bool            psphotReplaceUnfitSources (psArray *sources, psImageMaskType maskVal);

// thread-related:
bool            psphotSetThreads (void);
bool            psphotChooseCellSizes (int *Cx, int *Cy, pmReadout *readout, int nThreads);
bool            psphotCoordToCell (int *group, int *cell, float x, float y, int Cx, int Cy);
psArray        *psphotAssignSources (int Cx, int Cy, psArray *sources);

// used by psphotFindDetections
pmReadout      *psphotSignificanceImage (pmReadout *readout, psMetadata *recipe, psImageMaskType maskVal);
psArray        *psphotFindPeaks (pmReadout *significance, pmReadout *readout, psMetadata *recipe, const float threshold, const int nMax, int *totalPeaks, bool firstPass);
bool            psphotFindFootprints (pmDetections *detections, pmReadout *significance, pmReadout *readout, psMetadata *recipe, const float threshold, const int pass, psImageMaskType maskVal);
psErrorCode     psphotCullPeaks(const pmReadout *readout, const pmReadout *signifRO, const psMetadata *recipe, psArray *footprints);

// in psphotApResid.c:
pmTrend2D      *psphotApResidTrend (float *apResidSysErr, pmReadout *readout, int Nx, int Ny, psVector *xPos, psVector *yPos, psVector *apResid, psVector *dMag);
bool            psphotApResidMags_Threaded (psThreadJob *job);

// basic support functions
void            psphotModelClassInit (void);
bool            psphotGrowthCurve (pmReadout *readout, pmPSF *psf, bool ignore, psImageMaskType maskVal);

// functions to set the correct source pixels
bool            psphotInitRadiusPSF (psMetadata *recipe, pmReadout *readout);
bool            psphotInitRadiusEXT (psMetadata *recipe, pmReadout *readout);

bool            psphotCheckRadiusPSF (pmReadout *readout, pmSource *source, pmModel *model, psImageMaskType markVal);
bool            psphotCheckRadiusPSFBlend (pmReadout *readout, pmSource *source, pmModel *model, psImageMaskType markVal, float dR);
bool            psphotSetRadiusFootprint (float *radius, pmReadout *readout, pmSource *source, psImageMaskType markVal, float factor);
bool            psphotSetRadiusModel (pmModel *model, pmReadout *readout, pmSource *source, psImageMaskType markVal, bool deep);

bool            psphotDumpTest (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotDumpMoments (psMetadata *recipe, psArray *sources);
psMetadata     *psphotDefineHeader (psMetadata *recipe);
int             psphotSaveImage (psMetadata *header, psImage *image, char *filename);
bool            psphotDumpConfig (pmConfig *config);
pmReadout      *psphotSelectBackground (pmConfig *config, const pmFPAview *view, int index);
pmReadout      *psphotSelectBackgroundStdev (pmConfig *config, const pmFPAview *view, int index);

// PSF / DBL / EXT evaluation functions
bool            psphotEvalPSF (pmSource *source, pmModel *model);
bool            psphotEvalDBL (pmSource *source, pmModel *model);
bool            psphotEvalEXT (pmSource *source, pmModel *model);

//  functions to support the source fitting process
bool            psphotInitLimitsPSF (psMetadata *recipe, pmReadout *readout);
bool            psphotInitLimitsEXT (psMetadata *recipe, pmReadout *readout);
bool            psphotFitBlend (pmReadout *readout, pmSource *source, pmPSF *psf, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal);
bool            psphotFitBlob (pmReadout *readout, pmSource *source, psArray *sources, pmPSF *psf, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal);
bool            psphotFitPSF (pmReadout *readout, pmSource *source, pmPSF *psf, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal);
pmModel        *psphotFitEXT (pmModel *guessModel, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal);
psArray        *psphotFitDBL (pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal);

// functions to support simultaneous multi-source fitting
bool            psphotFitSet (pmSource *oneSrc, pmModel *oneModel, char *fitset, pmSourceFitMode mode, psImageMaskType maskVal);

// plotting functions (available if libkapa is installed)
bool            psphotPlotMoments (pmConfig *config, pmFPAview *view, psArray *sources);
bool            psphotPlotPSFModel (pmConfig *config, pmFPAview *view, psArray *sources);
bool            psphotFitInit (int nThreads);
bool            psphotFitSummary (void);
bool            psphotFitSummaryExtended (void);
bool            psphotFitInitExtended (void);

bool            psphotLoadPSF (pmConfig *config, const pmFPAview *view, const char *filerule);
bool            psphotLoadPSFReadout (pmConfig *config, const pmFPAview *view, const char *outFilename, const char *inFilename, int index);

bool            psphotSetHeaderNstars (psMetadata *recipe, psArray *sources);
bool            psphotRadialPlot (int *kapa, const char *filename, pmSource *source);
bool            psphotSourcePlots (pmReadout *readout, psArray *sources, psMetadata *recipe);
bool            psphotMosaicSubimage (psImage *outImage, pmSource *source, int Xo, int Yo, int DX, int DY, bool normalize);

bool            psphotMakeResiduals (psArray *sources, psMetadata *recipe, pmPSF *psf, psImageMaskType maskVal);

pmModel        *psphotPSFConvModel (pmReadout *readout, pmSource *source, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);

psKernel       *psphotKernelFromPSF (pmSource *source, int nPix);

// functions related to extended source analysis
bool            psphotRadialProfile (pmSource *source, psMetadata *recipe, float skynoise, psImageMaskType maskVal);
bool            psphotRadialProfilesByAngles (pmSource *source, int Nsec, float Rmax);
float           psphotRadiusFromProfile (pmSource *source, psVector *radius, psVector *flux, float fluxMin, float fluxMax);
bool            psphotRadiiFromProfiles (pmSource *source, float fluxMin, float fluxMax);
bool            psphotEllipticalProfile (pmSource *source, bool RAW_RADIUS);
bool            psphotEllipticalContour (pmSource *source);
bool            psphotLimitRadialApertures(psMetadata *recipe, long nSources);

// psphotVisual functions
bool            psphotVisualShowImage (pmReadout *readout);
bool            psphotVisualShowBackground (pmConfig *config, const pmFPAview *view, pmReadout *readout);
bool            psphotVisualShowSignificance (psImage *image, float min, float max);
bool            psphotVisualShowLogSignificance (psImage *image, float min, float max);
bool            psphotVisualShowPeaks (pmDetections *detections);
bool            psphotVisualShowSources (psArray *sources);
bool            psphotVisualShowFootprints (pmDetections *detections);
bool            psphotVisualShowMoments (psArray *sources);
bool            psphotVisualPlotMoments (psMetadata *recipe, psMetadata *analysis, psArray *sources);
bool            psphotVisualShowRoughClass (psArray *sources);
bool            psphotVisualShowPSFStars (psMetadata *recipe, pmPSF *psf, psArray *sources);
bool            psphotVisualShowSatStars (psMetadata *recipe, pmPSF *psf, psArray *sources);
bool            psphotVisualShowPSFModel (pmReadout *readout, pmPSF *psf);
bool            psphotVisualPlotRadialProfile (int myKapa, pmSource *source, psImageMaskType maskVal, pmSourceMode showmode);
bool            psphotVisualPlotRadialProfiles (psMetadata *recipe, psArray *sources, pmSourceMode showmode);
bool            psphotVisualShowFlags (psArray *sources);
bool            psphotVisualShowSourceSize (pmReadout *readout, psArray *sources);
bool            psphotVisualPlotSourceSizeAlt (psMetadata *recipe, psMetadata *analysis, psArray *sources);
bool            psphotVisualShowResidualImage (pmReadout *readout, bool reshow);
bool            psphotVisualShowObjectRegions (pmReadout *readout, psMetadata *recipe, psArray *sources);
bool            psphotVisualPlotApResid (psArray *sources, float mean, float error, bool useApMag);
bool            psphotVisualPlotChisq (psArray *sources);
bool            psphotVisualPlotSourceSize (psMetadata *recipe, psMetadata *analysis, psArray *sources);
bool            psphotVisualShowPetrosians (psArray *sources);
bool            psphotVisualEraseOverlays (int channel, char *overlay);
bool            psphotVisualClose(void);

int             psphotKapaChannel (int channel);
void            plotline (int myKapa, Graphdata *graphdata, float x0, float y0, float x1, float y1);


bool psphotPetrosian (pmSource *source, psMetadata *recipe, float skynoise, psImageMaskType maskVal);
bool psphotPetrosianRadialBins (pmSource *source, float radiusMax, float skynoise);
bool psphotPetrosianStats (pmSource *source);

// currently disabled:
// bool            psphotIsophotal (pmSource *source, psMetadata *recipe, psImageMaskType maskVal);
// bool            psphotAnnuli (pmSource *source, psMetadata *recipe, psImageMaskType maskVal);
// bool            psphotKron (pmSource *source, psMetadata *recipe, psImageMaskType maskVal);

// XXX visualization functions related to radial profiles (disabled)
bool psphotPetrosianVisualProfileByAngle (psVector *radius, psVector *flux);
bool psphotPetrosianVisualProfileRadii (psVector *radius, psVector *flux, psVector *radiusBin, psVector *fluxBin, float peakFlux, float RadiusRef);
bool psphotPetrosianVisualEllipticalContour (pmSourceRadialFlux *radFlux, pmSourceExtendedPars *extpars);
bool psphotPetrosianVisualStats (psVector *radBin, psVector *fluxBin,
                               psVector *refRadius, psVector *meanSB,
                               psVector *petRatio, psVector *petRatioErr, psVector *fluxSum,
                               float petRadius, float ratioForRadius,
                               float petFlux, float radiusForFlux);

bool psphotRadialBins (psMetadata *recipe, pmSource *source, float radiusMax, float skynoise);

int psphotKapaOpen (void);
bool psphotKapaClose (void);
bool psphotImageBackgroundCellHistogram (psVector *values, float mean, float sigma, int ix, int iy);
bool psphotDiagnosticPlots (const pmConfig *config, const char *name, ...);

bool psphotMakeFluxScale (psImage *image, psMetadata *recipe, pmPSF *psf);
bool psphotMakeGrowthCurve (pmReadout *readout, psMetadata *recipe, pmPSF *psf, psArray *sources);
bool psphotDumpPSFStars (pmReadout *readout, pmPSFtry *try, float radius, psImageMaskType maskVal, psImageMaskType markVal);

bool psphotLoadSRCTEXT (pmFPA *fpa, pmConfig *config);

bool psphotCheckStarDistribution (psArray *sources, psArray *stars, pmPSFOptions *options);
int psphotSupplementStars (psArray *stars, psArray *sources, psImageBinning *binning, int ix, int iy);

pmConfig *psphotForcedArguments(int argc, char **argv);
bool psphotForcedReadout(pmConfig *config, const pmFPAview *view, const char *filerule);

pmConfig *psphotFullForceArguments(int argc, char **argv);
bool psphotFullForceReadout(pmConfig *config, const pmFPAview *view, const char *filerule);

pmConfig *psphotMinimalArguments(int argc, char **argv);
bool psphotReadoutMinimal(pmConfig *config, const pmFPAview *view, const char *filerule);

pmConfig *psphotMakePSFArguments(int argc, char **argv);
bool psphotMakePSFReadout(pmConfig *config, const pmFPAview *view, const char *filerule);

pmConfig *psphotModelTestArguments(int argc, char **argv);
bool psphotModelTestReadout(pmConfig *config, const pmFPAview *view, const char *filerule);

bool psphotFullForceSummaryReadout (pmConfig * config, const pmFPAview *view);

int psphotFileruleCount(const pmConfig *config, const char *filerule);
bool psphotFileruleCountSet(const pmConfig *config, const char *filerule, int num);

bool psphotAddKnownSources (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *inSources);


/**** psphotStack prototypes ****/

pmConfig *psphotStackArguments(int argc, char **argv);
bool psphotStackParseCamera (pmConfig *config);
bool psphotStackImageLoop (pmConfig *config);
bool psphotStackReadout (pmConfig *config, const pmFPAview *view);
bool psphotStackUpdateReadout (pmConfig *config, const pmFPAview *view);
bool psphotStackChisqImage (pmConfig *config, const pmFPAview *view, const char *ruleDet, const char *ruleCnv);
bool psphotStackChisqImageAddReadout(const pmConfig *config, // Configuration
				     const pmFPAview *view,
				     const char *filename, 
				     pmReadout **chiReadout,
				     int index);
bool psphotStackAllocateOutput( const pmConfig *config, pmFPAview *view, psMetadata *recipe);

bool psphotStackRemoveChisqFromInputs (pmConfig *config, const char *filerule);
bool psphotStackMatchPSFsetup (pmConfig *config, const pmFPAview *view, const char *filerule, const char *fPSF);
bool psphotStackMatchPSFsetupReadout (pmConfig *config, const pmFPAview *view, const char *filerule, const char *fPSF, int index);
bool psphotStackLoadWCS(pmConfig *config, const pmFPAview *view, const char *filerule);
bool pmFPAfileRemoveSingle(psMetadata *files, const char *name, int num);

psArray *psphotMatchSources (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotMatchSourcesReadout (psArray *objects, pmConfig *config, const pmFPAview *view, const char *filerule, int index);
bool psphotMatchSourcesToObjects (psArray *objects, psArray *sources, float RADIUS);
bool psphotFilterMatchedSources (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects);
psArray *psphotLinkSources (pmConfig *config, const pmFPAview *view, const char *filerule);

bool psphotFitSourcesLinearStack (pmConfig *config, psArray *objects, bool final);

typedef enum {
    PSPHOT_CNV_SRC_NONE,
    PSPHOT_CNV_SRC_AUTO,
    PSPHOT_CNV_SRC_CNV,
    PSPHOT_CNV_SRC_RAW,
} psphotStackConvolveSource;

/// Options for stacking process
typedef struct {
    // Setup
    
    int numCols;                            // size of image (X)
    int numRows;                            // size of image (Y)

    int num;                            // Number of inputs
    bool convolve;                      // Convolve images?
    psphotStackConvolveSource convolveSource;

    // Prepare
    pmPSF *psf;                         // Target PSF
    psVector *inputSeeing;              // Input seeing FWHMs
    psVector *inputMask;                // Mask for inputs

    psVector *targetSeeing;		// Target seeing FWHMs
    psArray *sourceLists;               // Individual lists of sources for matching
    psVector *norm;                     // Normalisation for each image
    psArray *psfs;

    // Convolve
    psArray *kernels;                   // PSF-matching kernels --- required in the stacking
    psArray *regions;                   // PSF-matching regions --- required in the stacking
    psVector *matchChi2;                // chi^2 for stamps from matching
} psphotStackOptions;

typedef struct {
    float   Q;
    float   NSigma;
    float   clampSN;
    int     extModelType;
} psphotGalaxyShapeOptions;

/*** psphotStackMatchPSF prototypes ***/
bool psphotStackMatchPSFs (pmConfig *config, const pmFPAview *view);
bool psphotStackMatchPSFsReadout (pmConfig *config, const pmFPAview *view, psphotStackOptions *options, int index);
bool psphotStackMatchPSFsPrepare (pmConfig *config, const pmFPAview *view, psphotStackOptions *options, int index);

bool psphotStackMatchPSFsNext (pmConfig *config, const pmFPAview *view, const char *filerule, int lastSize);
bool psphotStackMatchPSFsNextReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, int lastSize);
int psphotStackMatchPSFsEntries (pmConfig *config, const pmFPAview *view, const char *filerule);

// psphotStackMatchPSFsUtils
// psVector *SetOptWidths (bool *optimum, psMetadata *recipe);
// pmReadout *makeFakeReadout(pmConfig *config, pmReadout *raw, psArray *sources, pmPSF *psf, psImageMaskType maskVal, int fullSize);
// bool rescaleData(pmReadout *readout, pmConfig *config, psphotStackOptions *options, int index);
// bool renormKernel(pmReadout *readout, psphotStackOptions *options, int index);
bool saveMatchData (pmReadout *readout, psphotStackOptions *options, int index);
bool matchKernel(pmConfig *config, pmReadout *cnv, pmReadout *raw, psphotStackOptions *options, int index);
// bool dumpImageDiff(pmReadout *readoutConv, pmReadout *readoutFake, pmReadout *readoutRef, int index, char *rootname);
// bool dumpImage(pmReadout *readoutOut, pmReadout *readoutRef, int index, char *rootname);
// bool loadKernel (pmConfig *config, pmReadout *readoutCnv, psphotStackOptions *options, int index);

bool psphotStackSetInputsToSkip(pmConfig *config, const pmFPAview *view, const char *filerule, bool set) ;

bool psphotStackRenormaliseVariance(const pmConfig *config, pmReadout *readout);

bool psphotStackPSF(const pmConfig *config, psphotStackOptions *options);

psphotStackOptions *psphotStackOptionsAlloc (int num);
psphotStackConvolveSource psphotStackConvolveSourceFromString (const char *string);
pmFPAfile *psphotStackGetConvolveSource (pmConfig *config, psphotStackOptions *options, int index);

bool psphotCopySources (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc);
bool psphotCopySourcesReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index);
bool psphotCopyEfficiency (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc);
bool psphotCopyEfficiencyReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index);

bool psphotRadialApertures (pmConfig *config, const pmFPAview *view, const char *filerule, int entry);
bool psphotRadialAperturesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, int entry);
bool psphotRadialApertures_Threaded (psThreadJob *job);
bool psphotRadialApertureSource (pmSource *source, pmReadout *readout, int entry, psVector *pixRadius2, psVector *pixFlux, psVector *pixVer);
// bool psphotRadialApertureSource (pmSource *source, int entry);

bool psphotExtendedSourceAnalysisByObject (pmConfig *config, psArray *objects, const pmFPAview *view, const char *filerule);
bool psphotRadialAperturesByObject (pmConfig *config, psArray *objects, const pmFPAview *view, const char *filerule, int nMatchedPSF);

bool psphotStackObjectsUnifyPosition (psArray *objects);

bool psphotFitSersicIndex (pmModel *model, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal);

bool psphotFitSersicIndexPCM (pmPCMdata *pcm, pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);
pmModel *psphotFitPCM (pmReadout *readout, pmSource *source, pmSourceFitOptions *fitOptions, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);

bool psphotCleanInputs (pmConfig *config, const pmFPAview *view, const char *filerule);

bool psphotResetModels (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotResetModelsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool psphotRedefinePixels (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotRedefinePixelsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe);

bool psphotSourceChildren (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc);
bool psphotSourceChildrenReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index);
psArray *psphotSourceChildrenByObject (pmConfig *config, const pmFPAview *view, const char *fileruleOut, const char *fileruleSrc, psArray *objectsSrc, bool sourcesSubtracted);

bool psphotSourceParents (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc);
bool psphotSourceParentsReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index);

bool psphotCopyPeaks (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc);
bool psphotCopyPeaksReadout (pmConfig *config, const pmFPAview *view, const char *ruleOut, const char *ruleSrc, int index);

bool psphotSersicModelClassGuessPCM (pmPCMdata *pcm, pmSource *source);
void psphotSersicModelClassInit ();
void psphotSersicModelClassCleanup ();

bool psphotSetRadiusMoments (float *fitRadius, float *windowRadius, pmReadout *readout, pmSource *source, psImageMaskType markVal);
bool psphotSetRadiusMomentsExact (float *fitRadius, float *windowRadius, pmReadout *readout, pmSource *source, psImageMaskType markVal);

bool psphotFootprintSaddles(pmReadout *readout, psArray *footprints);
bool psphotMaskFootprint (pmReadout *readout, pmSource *source, psImageMaskType markVal);

bool psphotKronIterate (pmConfig *config, const pmFPAview *view, const char *filerule, int pass);
bool psphotKronIterateReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char * filerule, pmReadout *readout, psArray *sources, pmPSF *psf, int index, int pass);
bool psphotKronIterate_Threaded (psThreadJob *job);

bool psphotKronFlux (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotKronFluxReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char *filerule, pmReadout *readout, psArray *sources);
bool psphotKronFlux_Threaded (psThreadJob *job);
bool psphotKronFluxSource (pmSource *source, psImageMaskType maskVal);

bool psphotPetroFlux (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotPetroFluxReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char * filerule, pmReadout *readout, psArray *sources);
bool psphotPetroFlux_Threaded (psThreadJob *job);
bool psphotPetroFluxSource (pmSource *source, psImageMaskType maskVal);

bool psphotGalaxyShape (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotGalaxyShapeReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char * filerule, pmReadout *readout, psArray *sources, pmPSF *psf);
bool psphotGalaxyShape_Threaded (psThreadJob *job);
bool psphotGalaxyShapeGrid (pmSource *source, pmSourceFitOptions *fitOptions, psphotGalaxyShapeOptions *opt, psImageMaskType maskVal, int psfSize);
bool psphotGalaxyShapeSource (pmPCMdata *pcm, pmSource *source, pmSourceGalaxyFits *galaxyFits, psImageMaskType maskVal, int psfSize, bool saveResults);
psphotGalaxyShapeOptions *psphotGalaxyShapeOptionsAlloc();
bool psphotGalaxyShapeOptionsSet(pmSource *source, pmSourceGalaxyFits *galaxyFits, psphotGalaxyShapeOptions *defaultOptions);

bool psphotRadialProfileWings (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotRadialProfileWingsReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, pmReadout *readout, psArray *sources);
bool psphotRadialProfileWings_Threaded (psThreadJob *job);

bool psphotStackObjectsSelectForAnalysis (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects);

bool psphotSetNFrames (pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotSetNFramesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool psphotGenerateModelVariance (pmConfig *config, const pmFPAview *view, pmFPAfile *file, int index, psMetadata *recipe, pmReadout *readout, psArray *sources);
pmSourceFitVarMode psphotGetFitVarMode (psMetadata *recipe);
bool psphotFreeModelVariance (pmReadout *readout, psArray *sources);

bool psphotModelBackgroundReadout(psImage *model,  // Model image
				  psImage *modelStdev, // Model stdev image
				  psMetadata *analysis, // Analysis metadata for outputs
				  pmReadout *readout, // Readout for which to generate a background model
				  psImageBinning *binning, // Binning parameters
				  const pmConfig *config,// Configuration
				  bool useVarianceImage
    );

bool psphotSatstarProfileModel (pmSource *source, psImageMaskType maskVal);
bool psphotSatstarProfileCreate (pmSource *source, psVector **logRmodelOut, psVector **logFmodelOut, psVector *logR, psVector *flux, float Rmax);
bool psphotSatstarProfileOp (pmSource *source, psImageMaskType maskVal, float FACTOR, pmModelOpMode mode, bool add);
bool psphotVisualRadialProfileSatstar (pmSource *source, psImageMaskType maskVal);
bool psphotAddOrSubSatstarsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int fileIndex, psMetadata *recipe, bool add);
bool psphotSatstarPhotometry (pmSource *source);

bool psphotSourceMemory(pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotSourceMemoryReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index);

bool psphotChipParams(pmConfig *config, const pmFPAview *view, const char *filerule);
bool psphotChipParams_Threaded(psThreadJob *job);

const char * psphotGetFilerule(const char *baseRule);
extern bool psphotINpsphotStack;


#endif
