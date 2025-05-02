bin_PROGRAMS = ppSub ppSubKernel ppSubConvolve

# Force recompilation of ppSubVersion.c, since it gets the version information
ppSubVersion.c: ppSubVersionDefinitions.h
ppSubVersionDefinitions.h: ppSubVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPSUB ppSubVersionDefinitions.h.in ppSubVersionDefinitions.h
FORCE: ;

ppSub_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSTATS_CFLAGS) $(PSPHOT_CFLAGS) $(PPSUB_CFLAGS)
ppSub_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPSTATS_LIBS)   $(PSPHOT_LIBS)   $(PPSUB_LIBS)

ppSub_SOURCES =				\
	ppSub.c				\
	ppSubArguments.c		\
	ppSubVersion.c			\
	ppSubBackground.c		\
	ppSubVarianceRescale.c		\
	ppSubCamera.c			\
	ppSubData.c			\
	ppSubErrorCodes.c		\
	ppSubExit.c			\
	ppSubFiles.c			\
	ppSubLoop.c			\
	ppSubDefineOutput.c		\
	ppSubInputDetections.c		\
	ppSubExtras.c			\
	ppSubFlagNeighbors.c		\
	ppSubMakePSF.c			\
	ppSubMatchPSFs.c		\
	ppSubMaskStats.c                \
	ppSubReadoutInverse.c		\
	ppSubReadoutJpeg.c		\
	ppSubReadoutPhotometry.c	\
	ppSubReadoutForcedPhot.c	\
	ppSubReadoutStats.c		\
	ppSubReadoutSubtract.c		\
	ppSubSetMasks.c			\
	ppSubThreshold.c

ppSubKernel_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSUB_CFLAGS)
ppSubKernel_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPSUB_LIBS)

ppSubKernel_SOURCES =		\
	ppSubKernel.c

ppSubConvolve_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSTATS_CFLAGS) $(PSPHOT_CFLAGS) $(PPSUB_CFLAGS)
ppSubConvolve_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPSTATS_LIBS)   $(PSPHOT_LIBS)   $(PPSUB_LIBS)

ppSubConvolve_SOURCES =		\
	ppSubConvolve.c		\
	ppSubExit.c		\
	ppSubVersion.c

noinst_HEADERS = \
	ppSub.h


### Error codes.
BUILT_SOURCES = ppSubErrorCodes.h ppSubErrorCodes.c
CLEANFILES = ppSubErrorCodes.h ppSubErrorCodes.c

ppSubErrorCodes.h : ppSubErrorCodes.dat ppSubErrorCodes.h.in
	$(ERRORCODES) --data=ppSubErrorCodes.dat --outdir=. ppSubErrorCodes.h

ppSubErrorCodes.c : ppSubErrorCodes.dat ppSubErrorCodes.c.in ppSubErrorCodes.h
	$(ERRORCODES) --data=ppSubErrorCodes.dat --outdir=. ppSubErrorCodes.c


clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
