bin_PROGRAMS = ppNoiseMap

noinst_HEADERS = \
	ppNoiseMap.h 

# Force recompilation of ppNoiseMapVersion.c, since it gets the version information
ppNoiseMapVersion.c: ppNoiseMapVersionDefinitions.h
ppNoiseMapVersionDefinitions.h: ppNoiseMapVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPNOISEMAP ppNoiseMapVersionDefinitions.h.in ppNoiseMapVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppNoiseMapVersionDefinitions.h

ppNoiseMap_CFLAGS = $(PPNOISEMAP_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppNoiseMap_LDFLAGS = $(PPNOISEMAP_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
ppNoiseMap_SOURCES = \
	ppNoiseMap.c \
	ppNoiseMapArguments.c \
	ppNoiseMapParseCamera.c \
	ppNoiseMapLoop.c \
	ppNoiseMapCleanup.c \
	ppNoiseMapVersion.c \
	ppNoiseMapReadout.c \
	ppNoiseMapStats.c \
	ppNoiseMapDefineFile.c

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
