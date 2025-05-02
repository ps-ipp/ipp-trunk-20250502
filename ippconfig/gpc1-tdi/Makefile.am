
installdir = $(datadir)/ippconfig/gpc1-tdi

install_files = \
	camera.config \
	format_tdi_20120416.config 

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
