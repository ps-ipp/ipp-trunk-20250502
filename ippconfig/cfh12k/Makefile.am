
installdir = $(datadir)/ippconfig/cfh12k

install_files = \
	dvo.config \
	dvo.layout \
	rejections.config \
	camera.config \
	format_mef.config \
	format_mef_early.config \
	format_split.config \
	format_split_early.config \
	format_split_cmf.config \
	ppImage.config \
	ppMerge.config \
	psastro.config \
	psphot.config \
	dark.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
