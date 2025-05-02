
installdir = $(datadir)/ippconfig/sdssmosaic/

install_files = \
	dvo.config \
	dvo.layout \
	camera.config \
	format_split.config \
	format_mef.config \
	cmp.config \
	cmf.config \
	ppImage.config \
	ppMerge.config \
	psastro.config \
	psphot.config \
	pswarp.config \
	reductionClasses.mdc \
	addstar.config \
	rejections.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
