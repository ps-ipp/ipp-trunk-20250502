
installdir = $(datadir)/ippconfig/lbc_red

install_files = \
	dvo.config \
	dvo.layout \
	camera.config \
	format_mef.config \
	ppImage.config \
	ppMerge.config \
	psastro.config \
	psphot.config \
	filerules.mdc \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
