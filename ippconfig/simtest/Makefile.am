
installdir = $(datadir)/ippconfig/simtest

install_files = \
	camera.config \
	format.config \
	format_relphot.config \
	ppImage.config \
	psastro.config \
	psphot.config \
	rejections.config \
	dvo.config \
	dvo.layout \
	gDiff.config \
	ppSim.config \
	ppSub.config \
	ppMerge.config \
	ppStack.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
