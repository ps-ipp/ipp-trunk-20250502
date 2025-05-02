
installdir = $(datadir)/ippconfig/esowfi

install_files = \
	dvo.config \
	camera.config \
	format_split.config \
	format_together.config \
	ppImage.config \
	psphot.config \
	psastro.config \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
