bin_PROGRAMS = ppMerge

# Force recompilation of ppMergeVersion.c, since it gets the version information
ppMergeVersion.c: ppMergeVersionDefinitions.h
ppMergeVersionDefinitions.h: ppMergeVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPMERGE ppMergeVersionDefinitions.h.in ppMergeVersionDefinitions.h
FORCE: ;

ppMerge_CFLAGS = $(PPMERGE_CFLAGS) $(PPSTATS_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppMerge_LDFLAGS = $(PPMERGE_LIBS) $(PPSTATS_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

ppMerge_SOURCES =		\
	ppMerge.c		\
	ppMergeArguments.c	\
	ppMergeCamera.c		\
	ppMergeFiles.c		\
	ppMergeScaleZero.c	\
	ppMergeFileGroup.c	\
	ppMergeReadChunk.c	\
	ppMergeLoop.c		\
	ppMergeSetThreads.c	\
	ppMergeMask.c		\
	ppMergeVersion.c

#	ppMergeLoop.c		

noinst_HEADERS =		\
	ppMerge.h

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
