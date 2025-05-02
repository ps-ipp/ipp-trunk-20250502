
installdir = $(datadir)/ippconfig/gpc1

install_files = \
	dvo.config \
	dvo.layout \
	camera.config \
	format_orig.config \
	format_20080925.config \
	format_20080929.config \
	format_20081011.config \
	format_20090120.config \
	format_20090220.config \
	format_20091209.config \
	format_20100122.config \
	format_20100228.config \
	format_20100723.config \
	format_20220827.config \
	format_mef.config \
	format_relphot.config \
	ghost.model.mdc \
	ppImage.config \
        ppMerge.config \
	ppStack.config \
	psphot.config \
	psastro.config \
        pswarp.config \
	psvideophot.config \
        crosstalk_rules.mdc \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
