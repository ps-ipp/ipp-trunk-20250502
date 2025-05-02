bin_PROGRAMS = ppstamp pstampdump

include_HEADERS = \
	pstamp.h

noinst_HEADERS = \
	ppstamp.h  \
	pstampint.h \
        pstampErrorCodes.h

ppstamp_CPPFLAGS = $(PPSTAMP_CFLAGS)
pstamprequest_CPPFLAGS = $(PPSTAMP_CFLAGS)
pstampdump_CPPFLAGS = $(PPSTAMP_CFLAGS)

ppstamp_LDFLAGS = $(PPSTAMP_LIBS)
pstamprequest_LDFLAGS = $(PPSTAMP_LIBS)
pstampdump_LDFLAGS = $(PPSTAMP_LIBS)

ppstamp_SOURCES = \
	ppstamp.c \
	ppstampArguments.c \
	ppstampCleanup.c \
        pstampErrorCodes.c \
	ppstampMakeStamp.c \
	ppstampMosaic.c \
	ppstampOptions.c \
	ppstampParseCamera.c \
        ppstampRegion.c \
	ppstampVersion.c \
	pstampGetROI.c

pstamprequest_SOURCES = \
	pstamprequest.c \
	pstampGetROI.c \
	ppstampOptions.c

pstampdump_SOURCES = \
        pstampdump.c


clean-local:
	-rm -f TAGS

# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

### Error codes.
BUILT_SOURCES = pstampErrorCodes.h pstampErrorCodes.c
CLEANFILES = pstampErrorCodes.h pstampErrorCodes.c

pstampErrorCodes.h : pstampErrorCodes.dat pstampErrorCodes.h.in
	$(ERRORCODES) --data=pstampErrorCodes.dat --outdir=. pstampErrorCodes.h

pstampErrorCodes.c : pstampErrorCodes.dat pstampErrorCodes.c.in pstampErrorCodes.h
	$(ERRORCODES) --data=pstampErrorCodes.dat --outdir=. pstampErrorCodes.c
