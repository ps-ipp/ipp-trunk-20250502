bin_PROGRAMS = ppSim ppSimSequence

# Force recompilation of ppSimVersion.c, since it gets the version information
ppSimVersion.c: ppSimVersionDefinitions.h
ppSimVersionDefinitions.h: ppSimVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPSIM ppSimVersionDefinitions.h.in ppSimVersionDefinitions.h
FORCE: ;

ppSim_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PSPHOT_CFLAGS) $(PSASTRO_CFLAGS) $(ppSim_CFLAGS)
ppSim_LDFLAGS = $(PSLIB_LIBS) $(PSMODULE_LIBS) $(PSPHOT_LIBS) $(PSASTRO_LIBS)
ppSim_SOURCES = \
	ppSim.c			  \
	ppSimArguments.c	  \
	ppSimCreate.c		  \
	ppSimLoadStars.c          \
	ppSimMakeStars.c          \
	ppSimMakeStarCluster.c 	  \
	ppSimMakeStarGrid.c       \
	ppSimMakeGalaxies.c       \
	ppSimMakeBiassec.c        \
	ppSimMakeBias.c           \
	ppSimMakeDark.c           \
	ppSimMakeSky.c            \
	ppSimInsertStars.c        \
	ppSimInsertGalaxies.c     \
	ppSimAddOverscan.c        \
	ppSimAddNoise.c           \
	ppSimSaturate.c           \
	ppSimBounds.c             \
	ppSimStars.c              \
	ppSimSmoothReadout.c      \
	ppSimSetPSF.c             \
	ppSimUtils.c              \
	ppSimLoop.c		  \
	ppSimMergeReadouts.c	  \
	ppSimMergeSources.c	  \
	ppSimMosaicChip.c	  \
	ppSimRandomGaussian.c	  \
	ppSimBadPixels.c          \
	ppSimBadCTE.c             \
	ppSimVersion.c

ppSimSequence_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(PSASTRO_CFLAGS) $(ppSim_CFLAGS)
ppSimSequence_LDFLAGS = $(PSLIB_LIBS) $(PSMODULE_LIBS) $(PSASTRO_LIBS)
ppSimSequence_SOURCES = \
	ppSimSequence.c		\
	ppSimSequenceBias.c	\
	ppSimSequenceDark.c	\
	ppSimSequenceFlat.c	\
	ppSimSequenceObject.c

noinst_HEADERS = \
	ppSim.h \
	ppSimSequence.h


CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
