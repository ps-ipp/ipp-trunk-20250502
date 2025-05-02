
bin_PROGRAMS = fpcamera

noinst_HEADERS = fpcamera.h fpcameraErrorCodes.h

fpcamera_CFLAGS = $(FPCAMERA_CFLAGS) $(PPSTATS_CFLAGS) $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
fpcamera_LDFLAGS = $(FPCAMERA_LIBS) $(PPSTATS_LIBS) $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

fpcamera_SOURCES = \
	fpcamera.c		    \
        fpcameraArguments.c	    \
	fpcameraVersion.c           \
	fpcameraParseCamera.c       \
	fpcameraDataLoad.c          \
	fpcameraAnalysis.c          \
	fpcameraDataSave.c          \
	fpcameraErrorCodes.c        \
	fpcameraReadAstrometry.c    \
	fpcameraLoadRefstars.c      \
	fpcameraChooseRefstars.c    \
	fpcameraAstrometryHeaders.c \
	fpcameraMetadataStats.c     \
	fpcameraCleanup.c

unused = \
	fpcameraMaskUpdates.c        \
	fpcameraMaskUtils.c          \
	fpcameraChooseGlintStars.c   \
	fpcameraLoadCrosstalk.c \
	fpcameraLoadGlints.c   \
	fpcameraLoadGhosts.c         \
	fpcameraGhostUtils.c         \
	fpcameraDefineFiles.c        \
	fpcameraAstromGuess.c        \
	fpcameraLoadRefstars.c       \
	fpcameraChooseRefstars.c     \
	fpcameraConvert.c	    \
	fpcameraChipAstrom.c         \
	fpcameraOneChipGrid.c	    \
	fpcameraOneChipFit.c	    \
	fpcameraRemoveClumps.c	    \
	fpcameraUtils.c	       	    \
	fpcameraTestFuncs.c          \
	fpcameraLuminosityFunction.c \
	fpcameraRefstarSubset.c      \
	fpcameraFixChips.c           \
	fpcameraFixChipsTest.c       \
	fpcameraUseModel.c           \
	fpcameraFindChip.c           \
	fpcameraZeroPoint.c    	    \
	fpcameraDemoDump.c           \
	fpcameraDemoPlot.c

CLEANFILES = fpcameraErrorCodes.h fpcameraErrorCodes.c
EXTRA_DIST = fpcameraErrorCodes.dat fpcameraErrorCodes.h.in fpcameraErrorCodes.c.in

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

BUILT_SOURCES = fpcameraErrorCodes.h fpcameraErrorCodes.c fpcameraVersionDefinitions.h

fpcameraErrorCodes.h : fpcameraErrorCodes.dat fpcameraErrorCodes.h.in
	$(ERRORCODES) --data=fpcameraErrorCodes.dat --outdir=. fpcameraErrorCodes.h

fpcameraErrorCodes.c : fpcameraErrorCodes.dat fpcameraErrorCodes.c.in fpcameraErrorCodes.h
	$(ERRORCODES) --data=fpcameraErrorCodes.dat --outdir=. fpcameraErrorCodes.c

# Force recompilation of fpcameraVersion.c, since it gets the version information
fpcameraVersion.c: fpcameraVersionDefinitions.h
fpcameraVersionDefinitions.h: fpcameraVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl FPCAMERA fpcameraVersionDefinitions.h.in fpcameraVersionDefinitions.h

FORCE: ;

# cat fpcameraVersionDefinitions.h.in | tr '@' '"' > fpcameraVersionDefinitions.h
