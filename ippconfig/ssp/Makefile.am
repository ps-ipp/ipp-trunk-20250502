
installdir = $(datadir)/ippconfig/ssp

install_files = \
	camera.config \
	format.config \
	ppImage.config \
	psastro.config \
	psphot.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
