bin_PROGRAMS = ppVizPattern

# Force recompilation of ppVizPatternVersion.c, since it gets the version information
ppVizPatternVersion.c: ppVizPatternVersionDefinitions.h
ppVizPatternVersionDefinitions.h: ppVizPatternVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPVIZPATTERN ppVizPatternVersionDefinitions.h.in ppVizPatternVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppVizPatternVersionDefinitions.h


ppVizPattern_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPVIZPATTERN_CFLAGS)
ppVizPattern_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPVIZPATTERN_LIBS)

ppVizPattern_SOURCES =		\
	ppVizPattern.c		\
	ppVizPatternArguments.c	\
	ppVizPatternCamera.c	\
	ppVizPatternData.c		\
	ppVizPatternLoop.c		\
	ppVizPatternVersion.c            

noinst_HEADERS = \
	ppVizPattern.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

