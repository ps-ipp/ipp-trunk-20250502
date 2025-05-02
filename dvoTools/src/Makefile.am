bin_PROGRAMS = dvoMakeCorr dvoApplyCorr

noinst_HEADERS = \
	dvoMakeCorr.h \
	dvoApplyCorr.h

dvoMakeCorr_CFLAGS = $(DVOCORR_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
dvoMakeCorr_LDFLAGS = $(DVOCORR_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
dvoMakeCorr_SOURCES = \
	dvoMakeCorr.c \
	dvoMakeCorrArguments.c \
	dvoMakeCorrParseCamera.c \
	dvoMakeCorrLoop.c \
	dvoMakeCorrUnbin.c \
	dvoMakeCorrCleanup.c \
	dvoMakeCorrOptions.c \
	dvoMakeCorrVersion.c

dvoApplyCorr_CFLAGS = $(DVOCORR_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
dvoApplyCorr_LDFLAGS = $(DVOCORR_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
dvoApplyCorr_SOURCES = \
	dvoApplyCorr.c \
	dvoApplyCorrArguments.c \
	dvoApplyCorrParseCamera.c \
	dvoApplyCorrLoop.c \
	dvoApplyCorrReadout.c \
	dvoApplyCorrCleanup.c \
	dvoApplyCorrOptions.c \
	dvoApplyCorrVersion.c

CLEANFILES = *~

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
