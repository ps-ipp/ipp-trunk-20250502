bin_PROGRAMS = ppNormCalc

ppNormCalc_CFLAGS = $(PPNORM_CFLAGS) $(PSMODULE_CFLAGS) $(PSLIB_CFLAGS)
ppNormCalc_LDFLAGS = $(PPNORM_LIBS) $(PSMODULE_LIBS) $(PSLIB_LIBS)

ppNormCalc_SOURCES =		\
	ppNormCalc.c

noinst_HEADERS =

CLEANFILES = *~

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`
