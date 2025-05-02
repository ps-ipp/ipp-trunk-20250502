bin_PROGRAMS = ppBackground ppBackgroundStack

# Force recompilation of ppBackgroundVersion.c, since it gets the version information
ppBackgroundVersion.c: ppBackgroundVersionDefinitions.h
ppBackgroundVersionDefinitions.h: ppBackgroundVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPBACKGROUND ppBackgroundVersionDefinitions.h.in ppBackgroundVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppBackgroundVersionDefinitions.h


ppBackground_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PPSTATS_CFLAGS) $(PSPHOT_CFLAGS) $(PPBACKGROUND_CFLAGS)
ppBackground_LDFLAGS  = $(PSLIB_LIBS)   $(PSMODULE_LIBS)   $(PPSTATS_LIBS)   $(PSPHOT_LIBS)   $(PPBACKGROUND_LIBS)

ppBackgroundStack_CPPFLAGS = $(ppBackground_CPPFLAGS)
ppBackgroundStack_LDFLAGS = $(ppBackground_LDFLAGS)

ppBackground_SOURCES =		\
	ppBackground.c		\
	ppBackgroundArguments.c	\
	ppBackgroundCamera.c	\
	ppBackgroundData.c	\
	ppBackgroundLoop.c	\
	ppBackgroundVersion.c   \
	ppBackgroundRestore.c	\
	ppBackgroundErrorCodes.c	\
	ppBackgroundExit.c

ppBackgroundStack_SOURCES = \
	ppBackgroundStack.c \
	ppBackgroundStackArguments.c \
	ppBackgroundStackCamera.c \
	ppBackgroundStackData.c \
	ppBackgroundStackLoop.c \
	ppBackgroundStackMath.c \
	ppBackgroundVersion.c \
	ppBackgroundErrorCodes.c \
	ppBackgroundExit.c


noinst_HEADERS = \
	ppBackground.h		\
	ppBackgroundErrorCodes.h

### Error codes.
BUILT_SOURCES = ppBackgroundErrorCodes.h ppBackgroundErrorCodes.c
CLEANFILES = ppBackgroundErrorCodes.h ppBackgroundErrorCodes.c

ppBackgroundErrorCodes.h : ppBackgroundErrorCodes.dat ppBackgroundErrorCodes.h.in
	$(ERRORCODES) --data=ppBackgroundErrorCodes.dat --outdir=. ppBackgroundErrorCodes.h

ppBackgroundErrorCodes.c : ppBackgroundErrorCodes.dat ppBackgroundErrorCodes.c.in ppBackgroundErrorCodes.h
	$(ERRORCODES) --data=ppBackgroundErrorCodes.dat --outdir=. ppBackgroundErrorCodes.c


clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

