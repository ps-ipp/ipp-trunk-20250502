default: libastrocmd

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SRC     =       $(HOME)/cmd.astro
include ../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

# astro user commands ########################
srcs = \
$(SRC)/init.$(ARCH).o             \
$(SRC)/altaz.$(ARCH).o	           \
$(SRC)/biassub.$(ARCH).o	   \
$(SRC)/cgrid.$(ARCH).o		   \
$(SRC)/coords.$(ARCH).o	   \
$(SRC)/cdot.$(ARCH).o		   \
$(SRC)/cline.$(ARCH).o             \
$(SRC)/cneedles.$(ARCH).o		   \
$(SRC)/cplot.$(ARCH).o		   \
$(SRC)/crotation.$(ARCH).o	   \
$(SRC)/csystem.$(ARCH).o	   \
$(SRC)/ctimes.$(ARCH).o	   \
$(SRC)/cval.$(ARCH).o		   \
$(SRC)/czplot.$(ARCH).o	   \
$(SRC)/cdensify.$(ARCH).o	   \
$(SRC)/cdhistogram.$(ARCH).o	   \
$(SRC)/drizzle.$(ARCH).o	   \
$(SRC)/flux.$(ARCH).o		   \
$(SRC)/fitplx.$(ARCH).o	   \
$(SRC)/fitpm.$(ARCH).o	   \
$(SRC)/fitpm_irls.$(ARCH).o  \
$(SRC)/fitplx_irls.$(ARCH).o  \
$(SRC)/astrom_ops.$(ARCH).o	   \
$(SRC)/fixwrap.$(ARCH).o	   \
$(SRC)/fixcols.$(ARCH).o	   \
$(SRC)/fiximage.$(ARCH).o	   \
$(SRC)/forcedphot.$(ARCH).o	   \
$(SRC)/gauss.$(ARCH).o		   \
$(SRC)/getvel.$(ARCH).o	           \
$(SRC)/getlst.$(ARCH).o	           \
$(SRC)/medianmap.$(ARCH).o	   \
$(SRC)/mkgauss.$(ARCH).o	   \
$(SRC)/mksersic.$(ARCH).o	   \
$(SRC)/galradius.$(ARCH).o	   \
$(SRC)/galradbins.$(ARCH).o	   \
$(SRC)/galsectors.$(ARCH).o	   \
$(SRC)/galprofiles.$(ARCH).o	   \
$(SRC)/getcoords.$(ARCH).o	   \
$(SRC)/elliprofile.$(ARCH).o	   \
$(SRC)/ringflux.$(ARCH).o	   \
$(SRC)/petrosian.$(ARCH).o	   \
$(SRC)/kronflux.$(ARCH).o	   \
$(SRC)/multifit.$(ARCH).o	   \
$(SRC)/objload.$(ARCH).o	   \
$(SRC)/outline.$(ARCH).o	   \
$(SRC)/parallax_factor.$(ARCH).o   \
$(SRC)/polar.$(ARCH).o		   \
$(SRC)/precess.$(ARCH).o	   \
$(SRC)/profile.$(ARCH).o	   \
$(SRC)/radec.$(ARCH).o	           \
$(SRC)/region.$(ARCH).o	           \
$(SRC)/rotcurve.$(ARCH).o	   \
$(SRC)/scale.$(ARCH).o		   \
$(SRC)/sexigesimal.$(ARCH).o	   \
$(SRC)/sersic.$(ARCH).o	   \
$(SRC)/spec.$(ARCH).o		   \
$(SRC)/specpairfit.$(ARCH).o	   \
$(SRC)/spexseq.$(ARCH).o	   \
$(SRC)/spex1dgas.$(ARCH).o	   \
$(SRC)/spex2dgas.$(ARCH).o	   \
$(SRC)/mkclusters.$(ARCH).o	   \
$(SRC)/star.$(ARCH).o		   \
$(SRC)/transform.$(ARCH).o         \
$(SRC)/vshimage.$(ARCH).o         \
$(SRC)/shimage.$(ARCH).o         \
$(SRC)/wcs.$(ARCH).o         \
$(SRC)/imsub.$(ARCH).o		   \
$(SRC)/jdtolst.$(ARCH).o		   \
$(SRC)/imfit.$(ARCH).o		   \
$(SRC)/imfit-fgauss.$(ARCH).o	   \
$(SRC)/imfit-pgauss.$(ARCH).o	   \
$(SRC)/imfit-pgauss-psf.$(ARCH).o	   \
$(SRC)/imfit-qgauss.$(ARCH).o	   \
$(SRC)/imfit-qgauss-psf.$(ARCH).o	   \
$(SRC)/imfit-sgauss.$(ARCH).o	   \
$(SRC)/imfit-sgauss-psf.$(ARCH).o  \
$(SRC)/imfit-qfgauss.$(ARCH).o	   \
$(SRC)/imfit-qrgauss.$(ARCH).o	   \
$(SRC)/imfit-rgauss.$(ARCH).o	   \
$(SRC)/imfit-fgauss-pol.$(ARCH).o	   \
$(SRC)/imfit-rgauss-pol.$(ARCH).o	   \
$(SRC)/imfit-trail.$(ARCH).o

# dependancy rules for include files ########################
incs = \
$(INC)/opihi.h \
$(INC)/external.h \
$(INC)/shell.h \
$(INC)/dvomath.h \
$(INC)/display.h 

$(srcs): $(incs)

$(LIB)/libastrocmd.$(ARCH).a: $(srcs)
$(LIB)/libastrocmd.$(ARCH).$(DLLTYPE): $(srcs)

$(DESTLIB)/libastrocmd.a: $(LIB)/libastrocmd.$(ARCH).a
$(DESTLIB)/libastrocmd.$(DLLTYPE): $(LIB)/libastrocmd.$(ARCH).$(DLLTYPE)

libastrocmd: $(DESTLIB)/libastrocmd.a $(DESTLIB)/libastrocmd.$(DLLTYPE)

uninstall:
	rm -f $(DESTLIB)/libastrocmd.a
	rm -f $(DESTLIB)/libastrocmd.$(DLLTYPE)

