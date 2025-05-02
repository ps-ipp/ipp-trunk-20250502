SUBDIRS = $(SRCDIRS)

lib_LTLIBRARIES = libpslib.la

libpslib_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) 
libpslib_la_LIBADD = $(SRCSUBLIBS) $(PSLIB_LIBS)
libpslib_la_DEPENDENCIES = $(SRCSUBLIBS)
libpslib_la_SOURCES =
libpslib_la_LDFLAGS = -version-info $(PSLIB_LT_VERSION)


EXTRA_DIST = mainpage.dox psErrorCodes_en.dat

pkginclude_HEADERS = \
	pslib.h \
	pslib_strict.h

install-exec-local: libpslib.la
	$(PERL) -pe "s|(^dependency_libs=.*)|dependency_libs=\'$(PSLIB_LIBS)\'|" libpslib.la > libpslib.la.temp
	cp libpslib.la.temp libpslib.la
	rm -f libpslib.la.temp

CLEANFILES = *~ *.bb *.bbg *.da
