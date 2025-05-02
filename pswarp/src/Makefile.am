bin_PROGRAMS = pswarp

# Force recompilation of pswarpVersion.c, since it gets the version information
pswarpVersion.c: pswarpVersionDefinitions.h
pswarpVersionDefinitions.h: pswarpVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PSWARP pswarpVersionDefinitions.h.in pswarpVersionDefinitions.h
FORCE: ;

pswarp_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSTATS_CFLAGS) $(PSPHOT_CFLAGS) $(PSWARP_CFLAGS)
pswarp_LDFLAGS = $(PSLIB_LIBS) $(PSMODULE_LIBS) $(PPSTATS_LIBS) $(PSPHOT_LIBS) $(PSWARP_LIBS)

pswarp_SOURCES = \
	pswarp.c			\
	pswarpArguments.c		\
	pswarpExitCode.c		\
	pswarpLoop.c			\
	pswarpLoopBackground.c		\
	pswarpCleanup.c			\
	pswarpStatsFile.c		\
	pswarpDefine.c			\
	pswarpDefineBackground.c	\
	pswarpDefineSkycell.c	        \
	pswarpDefineLayout.c	        \
	pswarpErrorCodes.c		\
	pswarpLoadAstrometry.c          \
	pswarpMakePSF.c			\
	pswarpMapGrid.c			\
	pswarpMaskStats.c               \
	pswarpMatchRange.c		\
	pswarpOverlaps.c		\
	pswarpOptions.c	        	\
	pswarpParseCamera.c		\
	pswarpPixelsLit.c		\
	pswarpSetMaskBits.c		\
	pswarpSetThreads.c	        \
	pswarpTransformReadout.c	\
	pswarpTransformSources.c 	\
	pswarpTransformTile.c		\
	pswarpUpdateStatistics.c        \
	pswarpUpdateMetadata.c          \
	pswarpVersion.c            	\
	pswarpFiles.c

noinst_HEADERS = \
	pswarp.h \
	pswarpErrorCodes.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

### Error codes.
BUILT_SOURCES = pswarpErrorCodes.h pswarpErrorCodes.c
CLEANFILES = pswarpErrorCodes.h pswarpErrorCodes.c

pswarpErrorCodes.h : pswarpErrorCodes.dat pswarpErrorCodes.h.in
	$(ERRORCODES) --data=pswarpErrorCodes.dat --outdir=. pswarpErrorCodes.h

pswarpErrorCodes.c : pswarpErrorCodes.dat pswarpErrorCodes.c.in pswarpErrorCodes.h
	$(ERRORCODES) --data=pswarpErrorCodes.dat --outdir=. pswarpErrorCodes.c
