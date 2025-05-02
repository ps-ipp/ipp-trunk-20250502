SUBDIRS = src test

bin_SCRIPTS = psmodules-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= psmodules.pc

EXTRA_DIST = \
	Doxyfile.in \
	psmodules-config.in \
	psmodules.pc.in \
	autogen.sh

if HAVE_DOXYGEN
install-data-hook: doxygen
	-$(mkdir_p) $(mandir)/man3
	chmod 0755 $(mandir)/man3
	-cp $(top_builddir)/docs/man/man3/* $(mandir)/man3

doxygen:
	$(DOXYGEN)
endif

CLEANFILES = $(prefix)/docs/psmodules/* *~

test: check
