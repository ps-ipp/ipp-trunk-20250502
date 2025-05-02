noinst_LTLIBRARIES = libpsmodulesobjects.la

libpsmodulesobjects_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS) -I../pslib/
libpsmodulesobjects_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulesobjects_la_SOURCES  = \
	pmDetections.c \
	pmSpan.c \
	pmFootprint.c \
	pmFootprintArrayGrow.c \
	pmFootprintArraysMerge.c \
	pmFootprintAssignPeaks.c \
	pmFootprintCullPeaks.c \
	pmFootprintFind.c \
	pmFootprintSpans.c \
	pmFootprintFindAtPoint.c \
	pmFootprintIDs.c \
	pmPeaks.c \
	pmMoments.c \
	pmModel.c \
	pmModelClass.c \
	pmModelUtils.c \
	pmModel_CentralPixel.c \
	pmSource.c \
	pmPhotObj.c \
	pmSourceMasks.c \
	pmSourceMoments.c \
	pmSourceDiffStats.c \
	pmSourceExtendedPars.c \
	pmSourceSatstar.c \
	pmSourceUtils.c \
	pmSourceSky.c \
	pmSourceContour.c \
	pmSourceFitModel.c \
	pmSourceFitSet.c \
	pmSourcePhotometry.c \
	pmSourceOutputs.c \
	pmSourceIO.c \
	pmSourceIO_RAW.c \
	pmSourceIO_OBJ.c \
	pmSourceIO_SX.c \
	pmSourceIO_CMP.c \
	pmSourceIO_CFF.c \
	pmSourceIO_SMPDATA.c \
	pmSourceIO_PS1_DEV_0.c \
	pmSourceIO_PS1_DEV_1.c \
	pmSourceIO_PS1_CAL_0.c \
	pmSourceIO_CMF_PS1_V1.c \
	pmSourceIO_CMF_PS1_V2.c \
	pmSourceIO_CMF_PS1_V3.c \
	pmSourceIO_CMF_PS1_V4.c \
	pmSourceIO_CMF_PS1_V5.c \
	pmSourceIO_CMF_PS1_SV1.c \
	pmSourceIO_CMF_PS1_SV2.c \
	pmSourceIO_CMF_PS1_SV3.c \
	pmSourceIO_CMF_PS1_SV4.c \
	pmSourceIO_CMF_PS1_DV1.c \
	pmSourceIO_CMF_PS1_DV2.c \
	pmSourceIO_CMF_PS1_DV3.c \
	pmSourceIO_CMF_PS1_DV4.c \
	pmSourceIO_CMF_PS1_DV5.c \
	pmSourceIO_MatchedRefs.c \
	pmSourceIO_Glints.c \
	pmSourceIO_Ghosts.c \
	pmSourcePlots.c \
	pmSourcePlotPSFModel.c \
	pmSourcePlotMoments.c \
	pmSourcePlotApResid.c \
	pmSourceVisual.c \
	pmResiduals.c \
	pmPSF.c \
	pmPSF_IO.c \
	pmPSFtry.c \
	pmPSFtryModel.c \
	pmPSFtryFitEXT.c \
	pmPSFtryMakePSF.c \
	pmPSFtryFitPSF.c \
	pmPSFtryMetric.c \
	pmPCMdata.c \
	pmPCM_MinimizeChisq.c \
	pmSourceFitPCM.c \
	pmTrend2D.c \
	pmGrowthCurveGenerate.c \
	pmGrowthCurve.c \
	pmSourceMatch.c \
	pmSourceLensing.c \
	pmDetEff.c \
	pmSourceGroups.c \
	models/pmModel_GAUSS.c \
	models/pmModel_PGAUSS.c \
	models/pmModel_PS1_V1.c \
	models/pmModel_QGAUSS.c \
	models/pmModel_RGAUSS.c \
	models/pmModel_SERSIC.c \
	models/pmModel_EXP.c \
	models/pmModel_DEV.c \
	models/pmModel_TRAIL.c \
	models/pmModel_HSC_V1.c

pkginclude_HEADERS = \
	pmDetections.h \
	pmSpan.h \
	pmFootprint.h \
	pmFootprintSpans.h \
	pmPeaks.h \
	pmMoments.h \
	pmModelFuncs.h \
	pmModel.h \
	pmModelClass.h \
	pmModelUtils.h \
	pmModel_CentralPixel.h \
	pmSource.h \
	pmPhotObj.h \
	pmSourceMasks.h \
	pmSourceDiffStats.h \
	pmSourceExtendedPars.h \
	pmSourceUtils.h \
	pmSourceSky.h \
	pmSourceContour.h \
	pmSourceFitModel.h \
	pmSourceFitSet.h \
	pmSourcePhotometry.h \
	pmSourceOutputs.h \
	pmSourceIO.h \
	pmSourceSatstar.h \
	pmSourcePlots.h \
	pmSourceVisual.h \
	pmResiduals.h \
	pmPSF.h \
	pmPSF_IO.h \
	pmPSFtry.h \
	pmPCMdata.h \
	pmTrend2D.h \
	pmGrowthCurve.h \
	pmGrowthCurveGenerate.h \
	pmSourceMatch.h \
	pmSourceLensing.h \
	pmDetEff.h \
	pmSourceGroups.h \
	pmSourceInternal.h \
	models/pmModel_GAUSS.h \
	models/pmModel_PGAUSS.h \
	models/pmModel_PS1_V1.h \
	models/pmModel_QGAUSS.h \
	models/pmModel_RGAUSS.h \
	models/pmModel_SERSIC.h \
	models/pmModel_EXP.h \
	models/pmModel_DEV.h \
	models/pmModel_TRAIL.h \
	models/pmModel_HSC_V1.h

CLEANFILES = *~

# pmSourceID_CMF_* functions use a common framework
BUILT_SOURCES = \
pmSourceIO_CMF_PS1_V1.c \
pmSourceIO_CMF_PS1_V2.c \
pmSourceIO_CMF_PS1_V3.c \
pmSourceIO_CMF_PS1_V4.c \
pmSourceIO_CMF_PS1_V5.c \
pmSourceIO_CMF_PS1_DV1.c \
pmSourceIO_CMF_PS1_DV2.c \
pmSourceIO_CMF_PS1_DV3.c \
pmSourceIO_CMF_PS1_DV4.c \
pmSourceIO_CMF_PS1_DV5.c \
pmSourceIO_CMF_PS1_SV1.c \
pmSourceIO_CMF_PS1_SV2.c \
pmSourceIO_CMF_PS1_SV3.c \
pmSourceIO_CMF_PS1_SV4.c

pmSourceIO_CMF_PS1_V1.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_V1 pmSourceIO_CMF_PS1_V1.c

pmSourceIO_CMF_PS1_V2.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_V2 pmSourceIO_CMF_PS1_V2.c

pmSourceIO_CMF_PS1_V3.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_V3 pmSourceIO_CMF_PS1_V3.c

pmSourceIO_CMF_PS1_V4.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_V4 pmSourceIO_CMF_PS1_V4.c

pmSourceIO_CMF_PS1_V5.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_V5 pmSourceIO_CMF_PS1_V5.c

pmSourceIO_CMF_PS1_DV1.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_DV1 pmSourceIO_CMF_PS1_DV1.c

pmSourceIO_CMF_PS1_DV2.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_DV2 pmSourceIO_CMF_PS1_DV2.c

pmSourceIO_CMF_PS1_DV3.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_DV3 pmSourceIO_CMF_PS1_DV3.c

pmSourceIO_CMF_PS1_DV4.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_DV4 pmSourceIO_CMF_PS1_DV4.c

pmSourceIO_CMF_PS1_DV5.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_DV5 pmSourceIO_CMF_PS1_DV5.c

pmSourceIO_CMF_PS1_SV1.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_SV1 pmSourceIO_CMF_PS1_SV1.c

pmSourceIO_CMF_PS1_SV2.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_SV2 pmSourceIO_CMF_PS1_SV2.c

pmSourceIO_CMF_PS1_SV3.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_SV3 pmSourceIO_CMF_PS1_SV3.c

pmSourceIO_CMF_PS1_SV4.c : pmSourceIO_CMF.c.in mksource.pl
	mksource.pl pmSourceIO_CMF.c.in PS1_SV4 pmSourceIO_CMF_PS1_SV4.c

# EXTRA_DIST = pmErrorCodes.h.in pmErrorCodes.dat pmErrorCodes.c.in
