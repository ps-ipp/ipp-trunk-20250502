#ifndef PS_MODULES_H
#define PS_MODULES_H

#include <pslib.h>

// the following headers are from psModule:extras
#include <psPipe.h>
#include <psIOBuffer.h>
#include <psVectorBracket.h>
#include <pmKapaPlots.h>
#include <pmVisual.h>
#include <pmVisualUtils.h>
#include <ippStages.h>
#include <ippDiffMode.h>
#include <pmCensor.h>

// XXX the following headers define constructs needed by the elements below
#include <pmConfig.h>
#include <pmDetrendDB.h>
#include <pmDetrendThreads.h>
#include <pmHDU.h>
#include <pmFPA.h>
#include <pmFPALevel.h>
#include <pmFPAview.h>
#include <pmFPAfile.h>

// the following headers are from psModule:config
#include <pmErrorCodes.h>
#include <pmConfigRecipes.h>
#include <pmConfigCamera.h>
#include <pmConfigCommand.h>
#include <pmConfigMask.h>
#include <pmConfigDump.h>
#include <pmConfigRun.h>
#include <pmConfigRecipeValue.h>
#include <pmVersion.h>

// the following headers are from psModule:concepts
#include <pmConcepts.h>
#include <pmConceptsStandard.h>
#include <pmConceptsRead.h>
#include <pmConceptsWrite.h>
#include <pmConceptsCopy.h>
#include <pmConceptsPhotcode.h>
#include <pmConceptsAverage.h>
#include <pmConceptsUpdate.h>

// the following headers are from psModule:camera
#include <pmHDUUtils.h>
#include <pmHDUGenerate.h>
#include <pmFPAFlags.h>
#include <pmFPAfileDefine.h>
#include <pmFPAfileFitsIO.h>
#include <pmFPAfileFringeIO.h>
#include <pmFPAfileIO.h>
#include <pmFPARead.h>
#include <pmFPAConstruct.h>
#include <pmFPACopy.h>
#include <pmFPAHeader.h>
#include <pmFPAMaskWeight.h>
#include <pmFPAMosaic.h>
#include <pmFPARead.h>
#include <pmFPAWrite.h>
#include <pmFPA_JPEG.h>
#include <pmFPAExtent.h>
#include <pmFPACalibration.h>
#include <pmReadoutStack.h>
#include <pmFPAUtils.h>
#include <pmCellSquish.h>
#include <pmFPABin.h>

// the following headers are from psModule:detrend
#include <pmFlatField.h>
#include <pmFlatNormalize.h>
#include <pmFringeStats.h>
#include <pmMaskBadPixels.h>
#include <pmMaskStats.h>
#include <pmNonLinear.h>
#include <pmNewNonLinear.h>
#include <pmOverscan.h>
#include <pmBias.h>
#include <pmShutterCorrection.h>
// #include <pmSkySubtract.h>
#include <pmDark.h>
#include <pmRemnance.h>
#include <pmPattern.h>
#include <pmPatternIO.h>

// the following headers are from psModule:astrom
#include <pmAstrometryWCS.h>
#include <pmAstrometryUtils.h>
#include <pmAstrometryRegions.h>
#include <pmAstrometryObjects.h>
#include <pmAstrometryModel.h>
#include <pmAstrometryRefstars.h>
#include <pmAstrometryDistortion.h>
#include <pmAstrometryVisual.h>
#include <pmKHcorrect.h>

// the following headers are from psModule:imcombine
#include <pmStack.h>
#include <pmSubtractionTypes.h>
#include <pmStackReject.h>
#include <pmSubtraction.h>
#include <pmSubtractionThreads.h>
#include <pmSubtractionStamps.h>
#include <pmSubtractionKernels.h>
#include <pmSubtractionDeconvolve.h>
#include <pmSubtractionAnalysis.h>
#include <pmSubtractionMatch.h>
#include <pmSubtractionIO.h>
#include <pmSubtractionParams.h>
#include <pmSubtractionMask.h>
#include <pmSubtractionEquation.h>
#include <pmReadoutCombine.h>
#include <pmSubtractionVisual.h>
#include <pmStackVisual.h>

// the following headers are from psModule:objects
#include <pmTrend2D.h>
#include <pmResiduals.h>
#include <pmGrowthCurve.h>

#include <pmSpan.h>
#include <pmFootprintSpans.h>
#include <pmFootprint.h>
#include <pmPeaks.h>
#include <pmDetections.h>
#include <pmMoments.h>

#include <pmModelFuncs.h>
#include <pmModelClass.h>
#include <pmModel.h>
#include <pmModel_CentralPixel.h>

#include <pmSourceMasks.h>
#include <pmSourceExtendedPars.h>
#include <pmSourceSatstar.h>
#include <pmSourceDiffStats.h>
#include <pmSourceLensing.h>
#include <pmSource.h>
#include <pmSourceFitModel.h>
#include <pmPSF.h>
#include <pmPSFtry.h>
#include <pmPhotObj.h>
#include <pmSourceUtils.h>
#include <pmSourceIO.h>
#include <pmSourceSky.h>
#include <pmSourceFitSet.h>
#include <pmSourceContour.h>
#include <pmSourcePlots.h>
#include <pmPSF_IO.h>
#include <pmModelUtils.h>
#include <pmSourcePhotometry.h>
#include <pmGrowthCurveGenerate.h>
#include <pmSourceVisual.h>
#include <pmSourceMatch.h>
#include <pmDetEff.h>
#include <pmPCMdata.h>

// The following headers are from random locations, here because they cross bounds
#include <pmReadoutFake.h>
#include <pmPSFEnvelope.h>
#include <pmThreadTools.h>

#endif
