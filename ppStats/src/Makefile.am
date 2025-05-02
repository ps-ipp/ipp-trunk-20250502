lib_LTLIBRARIES = libppStats.la

# Force recompilation of ppStatsVersion.c, since it gets the version information
ppStatsVersion.c: ppStatsVersionDefinitions.h
ppStatsVersionDefinitions.h: ppStatsVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPSTATS ppStatsVersionDefinitions.h.in ppStatsVersionDefinitions.h
FORCE: ;

libppStats_la_CFLAGS = $(PPSTATS_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
libppStats_la_LDFLAGS = $(PPSTATS_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

bin_PROGRAMS = ppStats ppStatsFromMetadata

ppStats_CFLAGS 	= $(PPSTATS_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppStats_LDFLAGS = $(PPSTATS_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
ppStats_LDADD 	= libppStats.la

ppStatsFromMetadata_CFLAGS  = $(PPSTATS_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppStatsFromMetadata_LDFLAGS = $(PPSTATS_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

ppStats_SOURCES =		\
	ppStats.c	        

ppStatsFromMetadata_SOURCES =		\
	ppStatsFromMetadata.c	        \
	ppStatsFromMetadataEntries.c	\
	ppStatsFromMetadataParse.c	\
	ppStatsFromMetadataStats.c	\
	ppStatsFromMetadataPrint.c	

libppStats_la_SOURCES = 		\
	ppStatsFPA.c			\
	ppStatsData.c			\
	ppStatsFringe.c			\
	ppStatsLoop.c			\
	ppStatsChip.c			\
	ppStatsCell.c			\
	ppStatsReadout.c		\
	ppStatsUtils.c  		\
	ppStatsPixels.c			\
	ppStatsMetadata.c		\
	ppStatsSetupFromRecipe.c	\
	ppStatsSetupFromArgs.c          \
	ppStatsVersion.c

include_HEADERS = 			\
	ppStats.h

noinst_HEADERS = 		\
	ppStatsInternal.h

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
