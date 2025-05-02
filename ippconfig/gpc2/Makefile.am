
installdir = $(datadir)/ippconfig/gpc2

install_files = \
	dvo.config \
	camera.config \
	format_default.config \
	format_mef.config \
	format_20180131.config \
	format_20220616.config \
        ghost.model.mdc \
        crosstalk_rules.mdc \
	ppImage.config \
        ppMerge.config \
	psphot.config \
	pswarp.config \
	ppStack.config \
	psastro.config

install_DATA = $(install_files)

install-data-hook:
	chmod 0755 $(installdir)

EXTRA_DIST = $(install_files)

ACLOCAL_AMFLAGS = -I m4
