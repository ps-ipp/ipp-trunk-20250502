lib_LTLIBRARIES = libpsastro.la

# Force recompilation of psastroVersion.c, since it gets the version information
psastroVersion.c: psastroVersionDefinitions.h
psastroVersionDefinitions.h: psastroVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PSASTRO psastroVersionDefinitions.h.in psastroVersionDefinitions.h
FORCE: ;

libpsastro_la_CFLAGS = $(PSASTRO_CFLAGS) $(PPSTATS_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
libpsastro_la_LDFLAGS = $(PSASTRO_LIBS) $(PPSTATS_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

bin_PROGRAMS = psastro psastroExtract psastroModel psastroModelFit gpcModel psastroTIO

psastro_CFLAGS = $(PSASTRO_CFLAGS) $(PPSTATS_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psastro_LDFLAGS = $(PSASTRO_LIBS) $(PPSTATS_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psastro_LDADD = libpsastro.la

psastroExtract_CFLAGS = $(PSASTRO_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psastroExtract_LDFLAGS = $(PSASTRO_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psastroExtract_LDADD = libpsastro.la

psastroModel_CFLAGS = $(PSASTRO_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psastroModel_LDFLAGS = $(PSASTRO_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psastroModel_LDADD = libpsastro.la

psastroModelFit_CFLAGS = $(PSASTRO_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psastroModelFit_LDFLAGS = $(PSASTRO_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psastroModelFit_LDADD = libpsastro.la

gpcModel_CFLAGS = $(PSASTRO_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
gpcModel_LDFLAGS = $(PSASTRO_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
gpcModel_LDADD = libpsastro.la

psastroTIO_CFLAGS = $(PSASTRO_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
psastroTIO_LDFLAGS = $(PSASTRO_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
psastroTIO_LDADD = libpsastro.la

psastro_SOURCES = \
	psastro.c		    \
        psastroArguments.c	    \
	psastroParseCamera.c   	    \
	psastroDataLoad.c           \
	psastroDataSave.c           \
	psastroMetadataStats.c      \
	psastroCleanup.c

psastroExtract_SOURCES = \
	psastroExtract.c		    \
        psastroExtractArguments.c	    \
	psastroExtractParseCamera.c   \
	psastroExtractDataLoad.c      \
	psastroExtractAnalysis.c      \
	psastroExtractStar.c      \
	psastroExtractStars.c      \
	psastroExtractGhosts.c      \
	psastroCleanup.c

psastroModel_SOURCES = \
	psastroModel.c		    \
        psastroModelArguments.c	    \
	psastroModelParseCamera.c   \
	psastroModelDataLoad.c      \
	psastroModelAnalysis.c      \
	psastroModelBoresite.c      \
	psastroModelFitBoresite.c   \
	psastroModelAdjust.c        \
	psastroModelDataSave.c      \
	psastroCleanup.c

psastroTIO_SOURCES = \
	psastroTIO.c		    \
        psastroTIOArguments.c	    \
	psastroTIOParseCamera.c   \
	psastroTIODataLoad.c      \
	psastroTIODataSave.c      \
	psastroCleanup.c

psastroModelFit_SOURCES = \
	psastroModelFit.c \
	psastroModelBoresite.c      \
	psastroModelFitBoresite.c

gpcModel_SOURCES = gpcModel.c

libpsastro_la_SOURCES = \
	psastroGalaxyShapeErrors.c  \
	psastroMaskUpdates.c        \
	psastroMaskUtils.c          \
	psastroChooseGlintStars.c   \
	psastroLoadCrosstalk.c \
	psastroLoadGlints.c   \
	psastroLoadGhosts.c         \
	psastroGhostUtils.c         \
	psastroErrorCodes.c         \
	psastroVersion.c            \
	psastroDefineFiles.c        \
	psastroAnalysis.c           \
	psastroAstromGuess.c        \
	psastroLoadRefstars.c       \
	psastroChooseRefstars.c     \
	psastroConvert.c	    \
	psastroChipAstrom.c         \
	psastroOneChipGrid.c	    \
	psastroOneChipFit.c	    \
	psastroRemoveClumps.c	    \
	psastroUtils.c	       	    \
	psastroTestFuncs.c          \
	psastroLuminosityFunction.c \
	psastroRefstarSubset.c      \
	psastroFixChips.c           \
	psastroFixChipsTest.c       \
	psastroUseModel.c           \
	psastroMosaicAstrom.c       \
	psastroMosaicDistortion.c   \
	psastroMosaicFPtoTP.c       \
	psastroMosaicGradients.c    \
	psastroMosaicCorrectDistortion.c   \
	psastroMosaicChipAstrom.c   \
	psastroMosaicOneChip.c      \
	psastroMosaicSetAstrom.c    \
	psastroMosaicSetMatch.c     \
	psastroFindChip.c           \
	psastroZeroPoint.c    	    \
	psastroDemoDump.c           \
	psastroDemoPlot.c

include_HEADERS = \
	psastro.h \
	psastroErrorCodes.h

noinst_HEADERS = \
	psastroInternal.h \
	psastroStandAlone.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

### Error codes.
BUILT_SOURCES = psastroErrorCodes.h psastroErrorCodes.c psastroVersionDefinitions.h
CLEANFILES = psastroErrorCodes.h psastroErrorCodes.c
EXTRA_DIST = psastroErrorCodes.dat psastroErrorCodes.h.in psastroErrorCodes.c.in

psastroErrorCodes.h : psastroErrorCodes.dat psastroErrorCodes.h.in
	$(ERRORCODES) --data=psastroErrorCodes.dat --outdir=. psastroErrorCodes.h

psastroErrorCodes.c : psastroErrorCodes.dat psastroErrorCodes.c.in psastroErrorCodes.h
	$(ERRORCODES) --data=psastroErrorCodes.dat --outdir=. psastroErrorCodes.c
