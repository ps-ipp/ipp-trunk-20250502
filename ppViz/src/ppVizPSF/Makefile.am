bin_PROGRAMS = ppVizPSF

# Force recompilation of ppVizPSFVersion.c, since it gets the version information
ppVizPSFVersion.c: ppVizPSFVersionDefinitions.h
ppVizPSFVersionDefinitions.h: ppVizPSFVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPVIZPSF ppVizPSFVersionDefinitions.h.in ppVizPSFVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppVizPSFVersionDefinitions.h


ppVizPSF_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPVIZPSF_CFLAGS)
ppVizPSF_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPVIZPSF_LIBS)

ppVizPSF_SOURCES =		\
	ppVizPSF.c		\
	ppVizPSFArguments.c	\
	ppVizPSFCamera.c	\
	ppVizPSFData.c		\
	ppVizPSFLoop.c		\
	ppVizPSFVersion.c            

noinst_HEADERS = \
	ppVizPSF.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

