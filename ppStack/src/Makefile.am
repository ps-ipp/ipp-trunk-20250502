bin_PROGRAMS = ppStack ppStackMedian

# Force recompilation of ppStackVersion.c, since it gets the version information
ppStackVersion.c: ppStackVersionDefinitions.h
ppStackVersionDefinitions.h: ppStackVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPSTACK ppStackVersionDefinitions.h.in ppStackVersionDefinitions.h
FORCE: ;

ppStack_CFLAGS 	= $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PSPHOT_CFLAGS) $(PPSTATS_CFLAGS) $(PPSTACK_CFLAGS)
ppStack_LDFLAGS = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PSPHOT_LIBS)   $(PPSTATS_LIBS)   $(PPSTACK_LIBS)

ppStackMedian_CFLAGS 	= $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PSPHOT_CFLAGS) $(PPSTATS_CFLAGS) $(PPSTACK_CFLAGS)
ppStackMedian_LDFLAGS = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PSPHOT_LIBS)   $(PPSTATS_LIBS)   $(PPSTACK_LIBS)

ppStack_SOURCES =		\
	ppStack.c		\
	ppStackArguments.c	\
	ppStackCamera.c		\
	ppStackFiles.c		\
	ppStackLoop.c		\
	ppStackPSF.c		\
	ppStackReadout.c	\
	ppStackVersion.c	\
	ppStackMatch.c		\
	ppStackSources.c	\
	ppStackThread.c		\
	ppStackOptions.c	\
	ppStackSetup.c		\
	ppStackPrepare.c	\
	ppStackConvolve.c	\
	ppStackLoopByPercent.c  \
	ppStackCombinePercent.c \
	ppStackCombinePrepare.c	\
	ppStackCombineInitial.c	\
	ppStackCombineAlternate.c	\
	ppStackReject.c		\
	ppStackCombineFinal.c	\
	ppStackCleanup.c	\
	ppStackPhotometry.c	\
	ppStackFinish.c		\
	ppStackTarget.c		\
	ppStackUpdateHeader.c	\
	ppStackJPEGs.c		\
	ppStackStats.c		\
	ppStackErrorCodes.c

ppStackMedian_SOURCES =		\
	ppStackMedian.c		\
	ppStackArguments.c	\
	ppStackCamera.c		\
	ppStackFiles.c		\
	ppStackMedianLoop.c		\
	ppStackPSF.c		\
	ppStackReadout.c	\
	ppStackVersion.c	\
	ppStackMatch.c		\
	ppStackSources.c	\
	ppStackThread.c		\
	ppStackOptions.c	\
	ppStackSetup.c		\
	ppStackPrepare.c	\
	ppStackConvolve.c	\
	ppStackCombinePrepare.c	\
	ppStackCombineInitial.c	\
	ppStackCombineAlternate.c	\
	ppStackReject.c		\
	ppStackCombineFinal.c	\
	ppStackCleanup.c	\
	ppStackPhotometry.c	\
	ppStackFinish.c		\
	ppStackTarget.c		\
	ppStackUpdateHeader.c	\
	ppStackJPEGs.c		\
	ppStackStats.c		\
	ppStackErrorCodes.c

noinst_HEADERS = 		\
	ppStack.h		\
	ppStackLoop.h		\
	ppStackOptions.h	\
	ppStackThread.h		\
	ppStackErrorCodes.h

### Error codes.
BUILT_SOURCES = ppStackErrorCodes.h ppStackErrorCodes.c
CLEANFILES = ppStackErrorCodes.h ppStackErrorCodes.c

ppStackErrorCodes.h : ppStackErrorCodes.dat ppStackErrorCodes.h.in
	$(ERRORCODES) --data=ppStackErrorCodes.dat --outdir=. ppStackErrorCodes.h

ppStackErrorCodes.c : ppStackErrorCodes.dat ppStackErrorCodes.c.in ppStackErrorCodes.h
	$(ERRORCODES) --data=ppStackErrorCodes.dat --outdir=. ppStackErrorCodes.c


clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
