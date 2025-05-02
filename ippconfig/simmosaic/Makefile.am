
installdir = $(datadir)/ippconfig/simmosaic

install_files = \
	camera.config \
	format_split.config \
	format_together.config \
	ppImage.config \
	psastro.config \
	psphot.config \
	rejections.config \
	dvo.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
