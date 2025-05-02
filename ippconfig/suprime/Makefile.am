
installdir = $(datadir)/ippconfig/suprime

install_files = \
	dvo.config \
	dvo.layout \
	camera.config \
	format_raw.config \
	format_mef.config \
	format_split.config \
	ppImage.config \
	ppMerge.config \
	psastro.config \
	psphot.config \
	pswarp.config \
	ppStack.config \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
