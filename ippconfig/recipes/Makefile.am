
installdir = $(datadir)/ippconfig/recipes

install_files = \
	filerules-simple.mdc \
	filerules-mef.mdc \
	filerules-split.mdc \
	fitstypes.mdc \
	addstar.config \
	masks.config \
	masks.16bit.config \
	reductionClasses.mdc \
	rejections.config \
	dqStatsTool.config \
	ppImage.config \
	ppMerge.config \
	ppStack.config \
	ppSmooth.config \
	ppStats.config \
	psastro.config \
	psphot.config \
	pswarp.config \
	psvideophot.config \
	ppSim.config \
	ppSub.config \
	gDiff.config \
	dvocorr.config \
	censor.config \
	jpeg.mdc \
        ppStatsFromMetadata.config \
	ppNoiseMap.config \
	ppSkycell.config \
	ppVizPSF.mdc \
	ppBackground.mdc \
	pstamp.config \
	fpcamera.config \
	remote_hosts.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
