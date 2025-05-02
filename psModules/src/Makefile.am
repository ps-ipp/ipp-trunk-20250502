SUBDIRS = $(SRCDIRS)
lib_LTLIBRARIES = libpsmodules.la

libpsmodules_la_CPPFLAGS = $(SRCINC)
libpsmodules_la_LIBADD = $(SRCSUBLIBS) $(PSMODULES_LIBS)
libpsmodules_la_DEPENDENCIES = $(SRCSUBLIBS)
libpsmodules_la_SOURCES = 
libpsmodules_la_LDFLAGS = -version-info $(PSMODULES_LT_VERSION)

pkginclude_HEADERS = \
	psmodules.h

CLEANFILES = *~
