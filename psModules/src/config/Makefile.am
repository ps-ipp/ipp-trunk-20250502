noinst_LTLIBRARIES = libpsmodulesconfig.la

# Force recompilation of pmVersion.c, since it gets the version information
pmVersion.c: pmVersionDefinitions.h
pmVersionDefinitions.h: pmVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PSMODULES pmVersionDefinitions.h.in pmVersionDefinitions.h
FORCE: ;

libpsmodulesconfig_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS)
libpsmodulesconfig_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulesconfig_la_SOURCES  = \
	pmConfig.c \
	pmConfigRecipes.c \
	pmConfigCamera.c \
	pmConfigCommand.c \
	pmConfigMask.c \
	pmConfigDump.c \
	pmConfigRun.c \
	pmConfigRecipeValue.c \
	pmVersion.c \
	pmErrorCodes.c

pkginclude_HEADERS = \
	pmConfig.h \
	pmConfigRecipes.h \
	pmConfigCamera.h \
	pmConfigCommand.h \
	pmConfigMask.h \
	pmConfigDump.h \
	pmConfigRun.h \
	pmConfigRecipeValue.h \
	pmVersion.h \
	pmErrorCodes.h

# Error codes.
BUILT_SOURCES = pmErrorCodes.h pmErrorCodes.c pmVersionDefinitions.h
CLEANFILES = *~ pmErrorCodes.h pmErrorCodes.c pmVersionDefinitions.h

pmErrorCodes.h : pmErrorCodes.dat pmErrorCodes.h.in
	$(ERRORCODES) --data=pmErrorCodes.dat --outdir=. pmErrorCodes.h

pmErrorCodes.c : pmErrorCodes.dat pmErrorCodes.c.in pmErrorCodes.h
	$(ERRORCODES) --data=pmErrorCodes.dat --outdir=. pmErrorCodes.c

EXTRA_DIST = pmErrorCodes.h.in pmErrorCodes.dat pmErrorCodes.c.in
