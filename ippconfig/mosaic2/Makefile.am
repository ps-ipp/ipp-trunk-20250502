
installdir = $(datadir)/ippconfig/mosaic2

install_files = \
	dvo.config \
	dvo.layout \
	camera.config \
	format.config \
	ppImage.config \
        psastro.config \
	psphot.config \
	ppMerge.config \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
