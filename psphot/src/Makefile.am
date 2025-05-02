lib_LTLIBRARIES = libpsphot.la

# Force recompilation of psphotVersion.c, since it gets the version information
psphotVersion.c: psphotVersionDefinitions.h
psphotVersionDefinitions.h: psphotVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PSPHOT psphotVersionDefinitions.h.in psphotVersionDefinitions.h
FORCE: ;

libpsphot_la_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
libpsphot_la_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

bin_PROGRAMS = psphot psphotForced psphotFullForce psphotFullForceSummary psphotMinimal psphotMakePSF psphotStack psphotModelTest psmakecff
# bin_PROGRAMS = psphotPetrosianStudy psphotTest psphotMomentsStudy 

psphot_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphot_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphot_LDADD = libpsphot.la

psphotForced_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotForced_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotForced_LDADD = libpsphot.la

psphotFullForce_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotFullForce_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotFullForce_LDADD = libpsphot.la

psphotFullForceSummary_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotFullForceSummary_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotFullForceSummary_LDADD = libpsphot.la

psphotMinimal_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotMinimal_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotMinimal_LDADD = libpsphot.la

psphotMakePSF_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotMakePSF_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotMakePSF_LDADD = libpsphot.la

psphotStack_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotStack_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotStack_LDADD = libpsphot.la

psphotModelTest_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psphotModelTest_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psphotModelTest_LDADD = libpsphot.la

psmakecff_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psmakecff_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psmakecff_LDADD = libpsphot.la


# psphotMomentsStudy_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
# psphotMomentsStudy_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
# psphotMomentsStudy_LDADD = libpsphot.la

# psphotPetrosianStudy_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
# psphotPetrosianStudy_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
# psphotPetrosianStudy_LDADD = libpsphot.la

# psphotTest_CFLAGS = $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
# psphotTest_LDFLAGS = $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
# psphotTest_LDADD = libpsphot.la

# standard psphot for generic photometry
psphot_SOURCES = \
        psphot.c                \
	psphotArguments.c	\
	psphotParseCamera.c     \
	psphotImageLoop.c	\
	psphotMosaicChip.c	\
	psphotCleanup.c

# forced photometry of specified positions given a specified psf
psphotForced_SOURCES = \
        psphotForced.c             \
	psphotForcedArguments.c	   \
	psphotParseCamera.c        \
	psphotImageLoop.c	   \
	psphotMosaicChip.c	   \
	psphotCleanup.c

# generalized forced photometry (including Kron, Petro, and Models) of specified positions given a specified psf
psphotFullForce_SOURCES = \
        psphotFullForce.c          \
	psphotFullForceArguments.c \
	psphotParseCamera.c        \
	psphotImageLoop.c	   \
	psphotMosaicChip.c	   \
	psphotCleanup.c

# combine full force results from several inputs
psphotFullForceSummary_SOURCES = \
        psphotFullForceSummary.c \
        psphotFullForceSummaryReadout.c

# forced photometry of specified positions given a specified psf
psphotMinimal_SOURCES = \
        psphotMinimal.c            \
	psphotMinimalArguments.c	   \
	psphotParseCamera.c        \
	psphotImageLoop.c	   \
	psphotMosaicChip.c	   \
	psphotCleanup.c

# a psphot-variant that simply generates the PSF model
psphotMakePSF_SOURCES = \
        psphotMakePSF.c            \
	psphotMakePSFArguments.c   \
	psphotParseCamera.c        \
	psphotImageLoop.c   \
	psphotMosaicChip.c	   \
	psphotCleanup.c

# a psphot-variant for stack photometry
psphotStack_SOURCES = \
        psphotStack.c                 \
	psphotStackArguments.c        \
	psphotStackParseCamera.c      \
	psphotStackImageLoop.c        \
	psphotStackReadout.c	      \
	psphotStackUpdateReadout.c	      \
	psphotStackChisqImage.c	      \
	psphotFitSourcesLinearStack.c \
	psphotSourceMatch.c           \
	psphotStackMatchPSFs.c        \
	psphotStackMatchPSFsUtils.c   \
	psphotStackMatchPSFsPrepare.c \
	psphotStackMatchPSFsNext.c    \
	psphotStackOptions.c          \
	psphotStackObjects.c          \
	psphotStackPSF.c	      \
	psphotStackAllocateOutput.c   \
	psphotCleanup.c

# a psphot-variant that simply generates the PSF model
psphotModelTest_SOURCES = \
        psphotModelTest.c            \
	psphotModelTestArguments.c   \
	psphotParseCamera.c        \
	psphotImageLoop.c   \
	psphotMosaicChip.c	   \
	psphotCleanup.c

# a program that takes a cmf file and makes a cff file (input file for psphotFullForce
psmakecff_SOURCES = \
	psmakecff.c

# psphotTest_SOURCES = \
#         psphotTest.c
# 
# psphotMomentsStudy_SOURCES = \
#         psphotMomentsStudy.c
# 
# psphotPetrosianStudy_SOURCES = \
#         psphotPetrosianStudy.c

libpsphot_la_SOURCES = \
	psphotSetThreads.c             \
	psphotErrorCodes.c	       \
	psphotImageQuality.c           \
	psphotVisual.c                 \
	psphotCullPeaks.c	       \
	psphotFootprintSaddles.c       \
	psphotVersion.c		       \
	psphotModelGroupInit.c	       \
	psphotMaskReadout.c	       \
	psphotDefineFiles.c	       \
	psphotReadout.c		       \
	psphotReadoutFindPSF.c	       \
	psphotReadoutKnownSources.c    \
	psphotReadoutForcedKnownSources.c    \
	psphotReadoutMinimal.c	       \
	psphotForcedReadout.c	       \
	psphotFullForceReadout.c       \
	psphotFullForce.SourceStats.c \
	psphotMakePSFReadout.c	       \
	psphotModelBackground.c	       \
	psphotModelTestReadout.c       \
	psphotMaskBackground.c	       \
	psphotSubtractBackground.c     \
	psphotFindDetections.c	       \
	psphotFindPeaks.c	       \
	psphotFindFootprints.c	       \
	psphotSignificanceImage.c      \
	psphotSourceStats.c	       \
	psphotRoughClass.c	       \
	psphotBasicDeblend.c	       \
	psphotChoosePSF.c	       \
	psphotGuessModels.c            \
	psphotFitSourcesLinear.c       \
	psphotBlendFit.c	       \
	psphotReplaceUnfit.c	       \
	psphotApResid.c		       \
	psphotMakeGrowthCurve.c	       \
	psphotMagnitudes.c	       \
	psphotLensing.c	       \
	psphotSetMaskBits.c	       \
	psphotSkyReplace.c	       \
	psphotEvalPSF.c		       \
	psphotEvalFLT.c		       \
	psphotSourceFits.c	       \
	psphotRadiusChecks.c	       \
	psphotChooseAnalysisOptions.c  \
	psphotOutput.c		       \
	psphotFakeSources.c	       \
	psphotExtendedSourceAnalysis.c \
	psphotExtendedSourceAnalysisByObject.c \
	psphotExtendedSourceFits.c     \
	psphotSersicModelClass.c       \
	psphotKernelFromPSF.c	       \
	psphotFitSet.c		       \
	psphotSourceFreePixels.c       \
	psphotSummaryPlots.c           \
	psphotMergeSources.c	       \
	psphotLoadPSF.c	               \
	psphotLoadSRCTEXT.c            \
	psphotReadoutCleanup.c	       \
	psphotStatsFile.c	       \
	psphotSourcePlots.c	       \
	psphotRadialPlot.c	       \
	psphotKronIterate.c            \
	psphotKronFlux.c            \
	psphotPetroFlux.c            \
	psphotGalaxyShape.c            \
	psphotRadialProfileWings.c     \
	psphotDeblendSatstars.c	       \
	psphotMosaicSubimage.c	       \
	psphotMakeResiduals.c	       \
	psphotSourceSize.c	       \
	psphotDiagnosticPlots.c	       \
	psphotMakeFluxScale.c	       \
	psphotCheckStarDistribution.c  \
	psphotThreadTools.c  	       \
	psphotAddNoise.c               \
	psphotRadialProfile.c	       \
        psphotRadialProfileByAngles.c  \
	psphotRadiiFromProfiles.c      \
        psphotEllipticalContour.c      \
        psphotEllipticalProfile.c      \
	psphotRadialBins.c	       \
	psphotRadialApertures.c	       \
	psphotPetrosian.c	       \
        psphotPetrosianRadialBins.c    \
        psphotPetrosianStats.c         \
        psphotPetrosianVisual.c        \
	psphotEfficiency.c	       \
	psphotSetNFrames.c	       \
	psphotSourceMemory.c	       \
	psphotChipParams.c		\
	psphotGalaxyParams.c

# not currently used
#	psphotIsophotal.c	       \
#	psphotAnnuli.c		       \
#	psphotKron.c		       \
#	psphotKronMasked.c             \
#

# test versions
#       psphotPetrosianProfile.c       \
#       psphotPetrosianAnalysis.c      \
#

include_HEADERS = \
	psphot.h \
	psphotErrorCodes.h

noinst_HEADERS = \
	psphotInternal.h \
	psphotStandAlone.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

# Error codes.
BUILT_SOURCES = psphotErrorCodes.h psphotErrorCodes.c psphotVersionDefinitions.h
CLEANFILES = psphotErrorCodes.h psphotErrorCodes.c
EXTRA_DIST = psphotErrorCodes.dat psphotErrorCodes.c.in psphotErrorCodes.h.in \
	models/pmModel_STRAIL.c \
	models/pmModel_TEST1.c

psphotErrorCodes.h : psphotErrorCodes.dat psphotErrorCodes.h.in
	$(ERRORCODES) --data=psphotErrorCodes.dat --outdir=. psphotErrorCodes.h

psphotErrorCodes.c : psphotErrorCodes.dat psphotErrorCodes.c.in psphotErrorCodes.h
	$(ERRORCODES) --data=psphotErrorCodes.dat --outdir=. psphotErrorCodes.c
