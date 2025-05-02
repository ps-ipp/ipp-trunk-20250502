bin_PROGRAMS = ppConfigDump

ppConfigDump_CPPFLAGS = $(PSLIB_CFLAGS) $(PSMODULE_CFLAGS) $(ppConfigDump_CFLAGS)
ppConfigDump_LDFLAGS = $(PSLIB_LIBS) $(PSMODULE_LIBS) -lz
ppConfigDump_SOURCES = ppConfigDump.c

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
