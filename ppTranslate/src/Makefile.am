bin_PROGRAMS = ppMops ppMonet

# Force recompilation of ppMopsVersion.c, since it gets the version information
ppTranslateVersion.c: ppTranslateVersionDefinitions.h
ppTranslateVersionDefinitions.h: ppTranslateVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPTRANSLATE ppTranslateVersionDefinitions.h.in ppTranslateVersionDefinitions.h
FORCE: ;

ppMops_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(PPMOPS_CFLAGS)
ppMops_LDFLAGS  = $(PSLIB_LIBS) $(PPMOPS_LIBS)

ppMops_SOURCES =		\
	ppMops.c		\
	ppTranslateVersion.c	\
	ppMopsArguments.c	\
	ppMopsDetections.c	\
	ppMopsRead.c		\
	ppMopsWrite.c		\
	ppMopsMerge.c		\
	ppMopsGetSkyChipPsfVersion.c

ppMonet_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(PPMONET_CFLAGS)
ppMonet_LDFLAGS  = $(PSLIB_LIBS) $(PPMONET_LIBS)

ppMonet_SOURCES =		\
	ppMonet.c		\
	ppTranslateVersion.c	\
	ppMonetArguments.c


noinst_HEADERS = \
	ppTranslateVersion.h \
	ppMops.h \
	ppMonet.h


clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
