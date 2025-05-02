BUILT_SOURCES = pslib.config
EXTRA_DIST = pslib.config.template

bin_SCRIPTS = pslib-setsvnversion.pl

# this is done instead of using autoconf to delay the expansion of datadir
# until --prefix is known
pslib.config: $(srcdir)/pslib.config.template
	$(PERL) -pe 's|DATADIR|$(pkgdatadir)|' $? > $@

pslibconfdir = $(sysconfdir)/@PACKAGE_NAME@
pslibconf_DATA = pslib.config

#install-data-hook:
#	chmod 0755 $(pslibconfdir)

CLEANFILES = pslib.config
