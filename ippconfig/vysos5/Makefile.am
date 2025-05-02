
installdir = $(datadir)/ippconfig/vysos5

install_files = \
	camera.config \
	format.config \
	format_flipped.config \
	psphot.config \
	psastro.config \
	ppImage.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
