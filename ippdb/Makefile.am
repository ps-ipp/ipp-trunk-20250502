SUBDIRS = src tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= ippdb.pc

EXTRA_DIST = ippdb.pc.in Doxyfile.in

if HAVE_DOXYGEN

man_MANS = \
    $(top_builddir)/docs/man/man3/ippdb.3 \
    $(top_builddir)/docs/man/man3/pzDataStoreRow.3 \
    $(top_builddir)/docs/man/man3/summitExpRow.3 \
    $(top_builddir)/docs/man/man3/summitImfileRow.3 \
    $(top_builddir)/docs/man/man3/pzDownloadExpRow.3 \
    $(top_builddir)/docs/man/man3/pzDownloadImfileRow.3 \
    $(top_builddir)/docs/man/man3/newExpRow.3 \
    $(top_builddir)/docs/man/man3/newImfileRow.3 \
    $(top_builddir)/docs/man/man3/rawExpRow.3 \
    $(top_builddir)/docs/man/man3/rawImfileRow.3 \
    $(top_builddir)/docs/man/man3/guidePendingExpRow.3 \
    $(top_builddir)/docs/man/man3/chipRunRow.3 \
    $(top_builddir)/docs/man/man3/chipProcessedImfileRow.3 \
    $(top_builddir)/docs/man/man3/chipMaskRow.3 \
    $(top_builddir)/docs/man/man3/camRunRow.3 \
    $(top_builddir)/docs/man/man3/camProcessedExpRow.3 \
    $(top_builddir)/docs/man/man3/camMaskRow.3 \
    $(top_builddir)/docs/man/man3/fakeRunRow.3 \
    $(top_builddir)/docs/man/man3/fakeProcessedImfileRow.3 \
    $(top_builddir)/docs/man/man3/fakeMaskRow.3 \
    $(top_builddir)/docs/man/man3/warpRunRow.3 \
    $(top_builddir)/docs/man/man3/warpSkyCellMapRow.3 \
    $(top_builddir)/docs/man/man3/warpSkyfileRow.3 \
    $(top_builddir)/docs/man/man3/warpMaskRow.3 \
    $(top_builddir)/docs/man/man3/diffRunRow.3 \
    $(top_builddir)/docs/man/man3/diffInputSkyfileRow.3 \
    $(top_builddir)/docs/man/man3/diffSkyfileRow.3 \
    $(top_builddir)/docs/man/man3/stackRunRow.3 \
    $(top_builddir)/docs/man/man3/stackInputSkyfileRow.3 \
    $(top_builddir)/docs/man/man3/stackSumSkyfileRow.3 \
    $(top_builddir)/docs/man/man3/detRunRow.3 \
    $(top_builddir)/docs/man/man3/detInputExpRow.3 \
    $(top_builddir)/docs/man/man3/detProcessedImfileRow.3 \
    $(top_builddir)/docs/man/man3/detProcessedExpRow.3 \
    $(top_builddir)/docs/man/man3/detStackedImfileRow.3 \
    $(top_builddir)/docs/man/man3/detNormalizedStatImfileRow.3 \
    $(top_builddir)/docs/man/man3/detNormalizedImfileRow.3 \
    $(top_builddir)/docs/man/man3/detNormalizedExpRow.3 \
    $(top_builddir)/docs/man/man3/detResidImfileRow.3 \
    $(top_builddir)/docs/man/man3/detResidExpRow.3 \
    $(top_builddir)/docs/man/man3/detRunSummaryRow.3 \
    $(top_builddir)/docs/man/man3/detRegisteredImfileRow.3 \
    $(top_builddir)/docs/man/man3/detCorrectedExpRow.3 \
    $(top_builddir)/docs/man/man3/detCorrectedImfileRow.3 \
    $(top_builddir)/docs/man/man3/magicRunRow.3 \
    $(top_builddir)/docs/man/man3/magicInputSkyfileRow.3 \
    $(top_builddir)/docs/man/man3/magicTreeRow.3 \
    $(top_builddir)/docs/man/man3/magicNodeResultRow.3 \
    $(top_builddir)/docs/man/man3/magicMaskRow.3 \
    $(top_builddir)/docs/man/man3/calDBRow.3 \
    $(top_builddir)/docs/man/man3/calRunRow.3 \
    $(top_builddir)/docs/man/man3/flatcorrRunRow.3 \
    $(top_builddir)/docs/man/man3/flatcorrChipLinkRow.3 \
    $(top_builddir)/docs/man/man3/flatcorrCamLinkRow.3 \
    $(top_builddir)/docs/man/man3/pstampDataStoreRow.3 \
    $(top_builddir)/docs/man/man3/pstampProjectRow.3 \
    $(top_builddir)/docs/man/man3/pstampRequestRow.3 \
    $(top_builddir)/docs/man/man3/pstampJobRow.3 


docs/man/man3/ippdb.3 docs/man/man3/pzDataStoreRow.3 docs/man/man3/summitExpRow.3 docs/man/man3/summitImfileRow.3 docs/man/man3/pzDownloadExpRow.3 docs/man/man3/pzDownloadImfileRow.3 docs/man/man3/newExpRow.3 docs/man/man3/newImfileRow.3 docs/man/man3/rawExpRow.3 docs/man/man3/rawImfileRow.3 docs/man/man3/guidePendingExpRow.3 docs/man/man3/chipRunRow.3 docs/man/man3/chipProcessedImfileRow.3 docs/man/man3/chipMaskRow.3 docs/man/man3/camRunRow.3 docs/man/man3/camProcessedExpRow.3 docs/man/man3/camMaskRow.3 docs/man/man3/fakeRunRow.3 docs/man/man3/fakeProcessedImfileRow.3 docs/man/man3/fakeMaskRow.3 docs/man/man3/warpRunRow.3 docs/man/man3/warpSkyCellMapRow.3 docs/man/man3/warpSkyfileRow.3 docs/man/man3/warpMaskRow.3 docs/man/man3/diffRunRow.3 docs/man/man3/diffInputSkyfileRow.3 docs/man/man3/diffSkyfileRow.3 docs/man/man3/stackRunRow.3 docs/man/man3/stackInputSkyfileRow.3 docs/man/man3/stackSumSkyfileRow.3 docs/man/man3/detRunRow.3 docs/man/man3/detInputExpRow.3 docs/man/man3/detProcessedImfileRow.3 docs/man/man3/detProcessedExpRow.3 docs/man/man3/detStackedImfileRow.3 docs/man/man3/detNormalizedStatImfileRow.3 docs/man/man3/detNormalizedImfileRow.3 docs/man/man3/detNormalizedExpRow.3 docs/man/man3/detResidImfileRow.3 docs/man/man3/detResidExpRow.3 docs/man/man3/detRunSummaryRow.3 docs/man/man3/detRegisteredImfileRow.3 docs/man/man3/detCorrectedExpRow.3 docs/man/man3/detCorrectedImfileRow.3 docs/man/man3/magicRunRow.3 docs/man/man3/magicInputSkyfileRow.3 docs/man/man3/magicTreeRow.3 docs/man/man3/magicNodeResultRow.3 docs/man/man3/magicMaskRow.3 docs/man/man3/calDBRow.3 docs/man/man3/calRunRow.3 docs/man/man3/flatcorrRunRow.3 docs/man/man3/flatcorrChipLinkRow.3 docs/man/man3/flatcorrCamLinkRow.3 docs/man/man3/pstampDataStoreRow.3 docs/man/man3/pstampProjectRow.3 docs/man/man3/pstampRequestRow.3 docs/man/man3/pstampJobRow.3:
	$(DOXYGEN)

endif

clean-local:
	-rm -rf docs
