bin_PROGRAMS = ppCoord

# Force recompilation of ppCoordVersion.c, since it gets the version information
ppCoordVersion.c: ppCoordVersionDefinitions.h
ppCoordVersionDefinitions.h: ppCoordVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPCOORD ppCoordVersionDefinitions.h.in ppCoordVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppCoordVersionDefinitions.h


ppCoord_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPCOORD_CFLAGS)
ppCoord_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPCOORD_LIBS)

ppCoord_SOURCES =		\
	ppCoord.c		\
	ppCoordArguments.c	\
	ppCoordCamera.c	\
	ppCoordData.c		\
	ppCoordLoop.c		\
	ppCoordVersion.c            

noinst_HEADERS = \
	ppCoord.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

