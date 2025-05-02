SUBDIRS = m4 $(SUBDIR)

bin_SCRIPTS = pslib-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= pslib.pc

EXTRA_DIST = \
    Doxyfile.in \
    pslib-config.in \
    pslib.pc.in \
    autogen.sh

if HAVE_DOXYGEN
install-data-hook: doxygen
	-$(mkdir_p) $(mandir)/man3
	chmod 0755 $(mandir)/man3
	-cp $(top_builddir)/docs/man/man3/* $(mandir)/man3

doxygen:
	$(DOXYGEN)
endif

CLEANFILES = *~ *.bb *.bbg *.da DoxygenLog

clean-local:
	-rm -rf docs

test: check

ACLOCAL_AMFLAGS = -I m4
