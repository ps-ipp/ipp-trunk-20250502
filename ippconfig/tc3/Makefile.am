
installdir = $(datadir)/ippconfig/tc3

install_files = \
	dvo.config        \
	camera.config     \
	format.config     \
	format_ps2.config \
	format_tc1.config \
	ppImage.config    \
	ppMerge.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
