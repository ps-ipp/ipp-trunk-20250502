bin_PROGRAMS = ppSkycell

# Force recompilation of ppSkycellVersion.c, since it gets the version information
ppSkycellVersion.c: ppSkycellVersionDefinitions.h
ppSkycellVersionDefinitions.h: ppSkycellVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPSKYCELL ppSkycellVersionDefinitions.h.in ppSkycellVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppSkycellVersionDefinitions.h


ppSkycell_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSKYCELL_CFLAGS)
ppSkycell_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPSKYCELL_LIBS)

ppSkycell_SOURCES =		\
	ppSkycell.c		\
	ppSkycellArguments.c	\
	ppSkycellCamera.c	\
	ppSkycellData.c		\
	ppSkycellLoop.c		\
	ppSkycellVersion.c            

noinst_HEADERS = \
	ppSkycell.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

