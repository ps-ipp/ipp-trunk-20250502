bin_PROGRAMS = ppArith

# Force recompilation of ppArithVersion.c, since it gets the version information
ppArithVersion.c: ppArithVersionDefinitions.h
ppArithVersionDefinitions.h: ppArithVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPARITH ppArithVersionDefinitions.h.in ppArithVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppArithVersionDefinitions.h


ppArith_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSTATS_CFLAGS) $(PPARITH_CFLAGS)
ppArith_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPSTATS_LIBS)   $(PPARITH_LIBS)

ppArith_SOURCES =		\
	ppArith.c		\
	ppArithArguments.c	\
	ppArithLoop.c		\
	ppArithReadout.c	\
	ppArithVersion.c            

noinst_HEADERS = \
	ppArith.h

clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

