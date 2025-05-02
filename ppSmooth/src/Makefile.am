bin_PROGRAMS = ppSmooth

noinst_HEADERS = ppSmooth.h 

# Force recompilation of ppSmoothVersion.c, since it gets the version information
ppSmoothVersion.c: ppSmoothVersionDefinitions.h
ppSmoothVersionDefinitions.h: ppSmoothVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPSMOOTH ppSmoothVersionDefinitions.h.in ppSmoothVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppSmoothVersionDefinitions.h

ppSmooth_CFLAGS = $(PPSMOOTH_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppSmooth_LDFLAGS = $(PPSMOOTH_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
ppSmooth_SOURCES = \
	ppSmooth.c \
	ppSmoothArguments.c \
	ppSmoothParseCamera.c \
	ppSmoothLoop.c \
	ppSmoothCleanup.c \
	ppSmoothReadout.c \
	ppSmoothDefineFile.c \
	ppSmoothVersion.c

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
