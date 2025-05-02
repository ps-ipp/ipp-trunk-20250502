bin_PROGRAMS = pedestal

pedestal_CPPFLAGS = $(PSLIB_CFLAGS) $(pedestal_CFLAGS)
pedestal_LDFLAGS = $(PSLIB_LIBS)
pedestal_SOURCES = \
	pedestal.c

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
