#Makefile for fft functions of psLib
#
noinst_LTLIBRARIES = libpslibfft.la

libpslibfft_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(FFTW3_CFLAGS)
libpslibfft_la_SOURCES = \
	psFFT.c \
	psImageFFT.c \
	psVectorFFT.c

EXTRA_DIST = fft.i

pkginclude_HEADERS = \
	psFFT.h \
	psVectorFFT.h \
	psImageFFT.h

CLEANFILES = *~ *.bb *.bbg *.da
