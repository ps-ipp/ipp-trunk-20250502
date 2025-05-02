
installdir = $(datadir)/ippconfig/hsc

install_files = \
	camera.config \
	dvo.config \
	format_raw.config \
	format_mef.config \
	format_cmf.config \
	ghost.model.mdc \
	ppImage.config \
	ppMerge.config \
	psastro.config \
	psphot.config \
	ppStats.config \
	ppStack.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
