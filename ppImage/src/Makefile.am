
bin_PROGRAMS = ppImage

noinst_HEADERS  = ppImage.h ppImageErrorCodes.h

# Force recompilation of ppImageVersion.c, since it gets the version information
ppImageVersion.c: ppImageVersionDefinitions.h
ppImageVersionDefinitions.h: ppImageVersionDefinitions.h.in FORCE
	pslib-setsvnversion.pl PPIMAGE ppImageVersionDefinitions.h.in ppImageVersionDefinitions.h
FORCE: ;

BUILT_SOURCES = ppImageErrorCodes.h ppImageErrorCodes.c ppImageVersionDefinitions.h

ppImage_CFLAGS = $(PPIMAGE_CFLAGS) $(PPSTATS_CFLAGS) $(PSASTRO_CFLAGS) $(PSPHOT_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppImage_LDFLAGS = $(PPIMAGE_LIBS) $(PSASTRO_LIBS) $(PPSTATS_LIBS) $(PSPHOT_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)
ppImage_SOURCES = \
	ppImage.c \
	ppImageArguments.c \
	ppImageParseCamera.c \
	ppImageLoop.c \
	ppImageCleanup.c \
	ppImageOptions.c \
	ppImageDetrendReadout.c \
	ppImageDetrendRecord.c \
	ppImageDetrendNonLinear.c \
	ppImageDetrendNewNonLinear.c \
	ppImageDetrendFringe.c \
	ppImageDetrendFree.c \
	ppImageDetrendPattern.c \
	ppImageErrorCodes.c \
	ppImageRebinReadout.c \
	ppImageMosaic.c \
	ppImageMosaicBackground.c \
	ppImageMaskStats.c \
	ppImagePhotom.c \
	ppImageAstrom.c \
	ppImageAddstar.c \
	ppImageStatsOutput.c \
	ppImagePixelStats.c \
	ppImageMetadataStats.c \
	ppImageReplaceBackground.c \
	ppImageMeasureCrosstalk.c \
	ppImageCorrectCrosstalk.c \
	ppImageDefineFile.c \
	ppImageSetMaskBits.c \
	ppImageSetThreads.c \
	ppImageBurntoolMask.c \
	ppImageParityFlip.c \
	ppImageCheckCTE.c \
	ppImageCheckNoise.c \
	ppImageFileCheck.c \
	ppImageVersion.c \
	ppImageMemory.c \
        ppImageAddNoise.c \
        ppImageSquashNANs.c \
        ppImageRandomGaussian.c \
	ppImageAuxiliaryMask.c \
	ppImageBurntoolApply.c

CLEANFILES = *~ ppImageErrorCodes.h ppImageErrorCodes.c

EXTRA_DIST = ppImageErrorCodes.dat ppImageErrorCodes.c.in ppImageErrorCodes.h.in


clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

ppImageErrorCodes.h : ppImageErrorCodes.dat ppImageErrorCodes.h.in
	$(ERRORCODES) --data=ppImageErrorCodes.dat --outdir=. ppImageErrorCodes.h

ppImageErrorCodes.c : ppImageErrorCodes.dat ppImageErrorCodes.c.in ppImageErrorCodes.h
	$(ERRORCODES) --data=ppImageErrorCodes.dat --outdir=. ppImageErrorCodes.c
