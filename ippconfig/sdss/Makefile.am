
installdir = $(datadir)/ippconfig/sdss

install_files = \
	dvo.config \
	dvo.layout \
	camera.config \
	format.config \
	cmp.config \
	cmf.config \
	ppImage.config \
	ppMerge.config \
	psastro.config \
	psphot.config \
	pswarp.config \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
