default: libdata

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SDIR    =       $(HOME)/lib.data
include ../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

# general numerical functions (libdata) #####################
srcs = \
$(SDIR)/book.$(ARCH).o                  \
$(SDIR)/page.$(ARCH).o                  \
$(SDIR)/fft.$(ARCH).o			\
$(SDIR)/svdcmp.$(ARCH).o		\
$(SDIR)/svdcmp_bond_new.$(ARCH).o	\
$(SDIR)/bracket.$(ARCH).o		\
$(SDIR)/spline.$(ARCH).o		\
$(SDIR)/SplineOps.$(ARCH).o		\
$(SDIR)/MedImageOps.$(ARCH).o		\
$(SDIR)/ChebyshevOps.$(ARCH).o		\
$(SDIR)/mrqmin.$(ARCH).o		\
$(SDIR)/mrq2dmin.$(ARCH).o		\
$(SDIR)/nnet.$(ARCH).o			\
$(SDIR)/precess.$(ARCH).o		\
$(SDIR)/sort_funcs.$(ARCH).o		\
$(SDIR)/starfuncs.$(ARCH).o		\
$(SDIR)/hermitian.$(ARCH).o		\
$(SDIR)/graphtools.$(ARCH).o            \
$(SDIR)/queues.$(ARCH).o		\
$(SDIR)/PlotVectors.$(ARCH).o		\
$(SDIR)/open_kapa.$(ARCH).o             \
$(SDIR)/style_args.$(ARCH).o

# moved to libohana
# $(SDIR)/gaussian.$(ARCH).o		\

# fix malloc
#$(SDIR)/svdcmp_bond_raw.$(ARCH).o		\

#$(SDIR)/open_graph.$(ARCH).o            \
#$(SDIR)/open_image.$(ARCH).o            \

# dependancy rules for include files ########################
incs = \
$(INC)/opihi.h \
$(INC)/external.h \
$(INC)/shell.h \
$(INC)/dvomath.h \
$(INC)/display.h 

$(srcs): $(incs)

$(LIB)/libdata.$(ARCH).a: $(srcs)
$(LIB)/libdata.$(ARCH).$(DLLTYPE): $(srcs)

$(DESTLIB)/libdata.a: $(LIB)/libdata.$(ARCH).a
$(DESTLIB)/libdata.$(DLLTYPE): $(LIB)/libdata.$(ARCH).$(DLLTYPE)

libdata: $(DESTLIB)/libdata.a $(DESTLIB)/libdata.$(DLLTYPE)

uninstall:
	rm -f $(DESTLIB)/libdata.a
	rm -f $(DESTLIB)/libdata.$(DLLTYPE)
