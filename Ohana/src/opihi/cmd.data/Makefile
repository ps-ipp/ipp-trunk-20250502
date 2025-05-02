default: libdatacmd

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
SRC     =       $(HOME)/cmd.data
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
DATA    =       $(DESTDATA)/mana
include ../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

# data user commands ########################
srcs = \
$(SRC)/init.$(ARCH).o          \
$(SRC)/accum.$(ARCH).o		\
$(SRC)/antialias.$(ARCH).o		\
$(SRC)/applyfit1d.$(ARCH).o	\
$(SRC)/applyfit2d.$(ARCH).o	\
$(SRC)/applyfit3d.$(ARCH).o	\
$(SRC)/box.$(ARCH).o		\
$(SRC)/book.$(ARCH).o		\
$(SRC)/book_commands.$(ARCH).o	\
$(SRC)/bisection.$(ARCH).o		\
$(SRC)/cast.$(ARCH).o		\
$(SRC)/chebyshev.$(ARCH).o		\
$(SRC)/chebyshev_commands.$(ARCH).o	\
$(SRC)/center.$(ARCH).o	\
$(SRC)/clear.$(ARCH).o		\
$(SRC)/imclip.$(ARCH).o		\
$(SRC)/close.$(ARCH).o		\
$(SRC)/concat.$(ARCH).o	\
$(SRC)/contour.$(ARCH).o	\
$(SRC)/create.$(ARCH).o	\
$(SRC)/cumulative.$(ARCH).o	\
$(SRC)/cursor.$(ARCH).o	\
$(SRC)/cut.$(ARCH).o		\
$(SRC)/delete.$(ARCH).o	\
$(SRC)/densify.$(ARCH).o	\
$(SRC)/device.$(ARCH).o	\
$(SRC)/dft2d.$(ARCH).o	\
$(SRC)/dimendown.$(ARCH).o	\
$(SRC)/dimenup.$(ARCH).o	\
$(SRC)/distribute.$(ARCH).o	\
$(SRC)/dbconnect.$(ARCH).o	\
$(SRC)/dbselect.$(ARCH).o	\
$(SRC)/dbinsert.$(ARCH).o	\
$(SRC)/dbadmin.$(ARCH).o	\
$(SRC)/dbupdate.$(ARCH).o	\
$(SRC)/erase.$(ARCH).o		\
$(SRC)/extract.$(ARCH).o	\
$(SRC)/fft1d.$(ARCH).o		\
$(SRC)/fft2d.$(ARCH).o		\
$(SRC)/fit1d.$(ARCH).o		\
$(SRC)/fit1d_irls.$(ARCH).o		\
$(SRC)/fit2d.$(ARCH).o		\
$(SRC)/fit3d.$(ARCH).o		\
$(SRC)/gaussj.$(ARCH).o	\
$(SRC)/gaussdeviate.$(ARCH).o	\
$(SRC)/grid.$(ARCH).o		\
$(SRC)/gridify.$(ARCH).o       \
$(SRC)/grow.$(ARCH).o		\
$(SRC)/ungridify.$(ARCH).o     \
$(SRC)/histogram.$(ARCH).o	\
$(SRC)/tdhistogram.$(ARCH).o	\
$(SRC)/hermitian1d.$(ARCH).o	\
$(SRC)/hermitian2d.$(ARCH).o	\
$(SRC)/idxread.$(ARCH).o	 	\
$(SRC)/imcut.$(ARCH).o	 	\
$(SRC)/imhist.$(ARCH).o	\
$(SRC)/impeaks.$(ARCH).o	\
$(SRC)/imsmooth.$(ARCH).o	\
$(SRC)/imsmooth.generic.$(ARCH).o	\
$(SRC)/imsmooth.2d.$(ARCH).o	\
$(SRC)/imconvolve.$(ARCH).o	\
$(SRC)/imresample.$(ARCH).o	\
$(SRC)/imcollapse.$(ARCH).o	\
$(SRC)/integrate.$(ARCH).o	\
$(SRC)/interpolate.$(ARCH).o	\
$(SRC)/interpolate_presort.$(ARCH).o	\
$(SRC)/join.$(ARCH).o		\
$(SRC)/jpeg.$(ARCH).o		\
$(SRC)/kapamemory.$(ARCH).o		\
$(SRC)/kern.$(ARCH).o		\
$(SRC)/keyword.$(ARCH).o	\
$(SRC)/labels.$(ARCH).o	\
$(SRC)/limits.$(ARCH).o	\
$(SRC)/line.$(ARCH).o		\
$(SRC)/list_buffers.$(ARCH).o	\
$(SRC)/list_header.$(ARCH).o	\
$(SRC)/list_vectors.$(ARCH).o	\
$(SRC)/load.$(ARCH).o		\
$(SRC)/lookup.$(ARCH).o	\
$(SRC)/matrix.$(ARCH).o	\
$(SRC)/match1d.$(ARCH).o	\
$(SRC)/match2d.$(ARCH).o	\
$(SRC)/mkrgb.$(ARCH).o	\
$(SRC)/mcreate.$(ARCH).o	\
$(SRC)/medacc.$(ARCH).o	\
$(SRC)/mgaussdev.$(ARCH).o	\
$(SRC)/mget.$(ARCH).o		\
$(SRC)/mget3d.$(ARCH).o		\
$(SRC)/mslice.$(ARCH).o		\
$(SRC)/mlayer.$(ARCH).o		\
$(SRC)/minterpolate.$(ARCH).o	\
$(SRC)/medimage.$(ARCH).o	\
$(SRC)/medimage_commands.$(ARCH).o \
$(SRC)/medimage_calc.$(ARCH).o \
$(SRC)/mset.$(ARCH).o		\
$(SRC)/needles.$(ARCH).o	\
$(SRC)/nnet.$(ARCH).o		\
$(SRC)/nnet_commands.$(ARCH).o	\
$(SRC)/nnet_train.$(ARCH).o	\
$(SRC)/nnet_apply.$(ARCH).o	\
$(SRC)/peak.$(ARCH).o		\
$(SRC)/periodogram.$(ARCH).o	\
$(SRC)/periodogram-fm.$(ARCH).o	\
$(SRC)/plot.$(ARCH).o		\
$(SRC)/dot.$(ARCH).o		\
$(SRC)/point.$(ARCH).o		\
$(SRC)/ps.$(ARCH).o		\
$(SRC)/pdf.$(ARCH).o		\
$(SRC)/print_vectors.$(ARCH).o 	\
$(SRC)/mprint.$(ARCH).o 	\
$(SRC)/queuedelete.$(ARCH).o	\
$(SRC)/queuedrop.$(ARCH).o	\
$(SRC)/queuelist.$(ARCH).o	\
$(SRC)/queueload.$(ARCH).o	\
$(SRC)/queuesize.$(ARCH).o	\
$(SRC)/queuepush.$(ARCH).o	\
$(SRC)/queuepop.$(ARCH).o	\
$(SRC)/queueprint.$(ARCH).o	\
$(SRC)/queuesave.$(ARCH).o	\
$(SRC)/queuesubstr.$(ARCH).o	\
$(SRC)/queueinit.$(ARCH).o	\
$(SRC)/queue2book.$(ARCH).o	\
$(SRC)/radial.$(ARCH).o	\
$(SRC)/rd.$(ARCH).o		\
$(SRC)/rdjpg.$(ARCH).o		\
$(SRC)/rdseg.$(ARCH).o		\
$(SRC)/read_vectors.$(ARCH).o	\
$(SRC)/rebin.$(ARCH).o		\
$(SRC)/reindex.$(ARCH).o	\
$(SRC)/resize.$(ARCH).o		\
$(SRC)/relocate.$(ARCH).o	\
$(SRC)/rndseed.$(ARCH).o		\
$(SRC)/roll.$(ARCH).o		\
$(SRC)/rotate.$(ARCH).o	\
$(SRC)/save.$(ARCH).o		\
$(SRC)/section.$(ARCH).o	\
$(SRC)/select.$(ARCH).o	\
$(SRC)/set.$(ARCH).o		\
$(SRC)/shift.$(ARCH).o		\
$(SRC)/opihi_size.$(ARCH).o        \
$(SRC)/sort.$(ARCH).o		\
$(SRC)/spline.$(ARCH).o		\
$(SRC)/spline_commands.$(ARCH).o \
$(SRC)/imspline_apply.$(ARCH).o	\
$(SRC)/imspline_construct.$(ARCH).o \
$(SRC)/squash3d.$(ARCH).o	   \
$(SRC)/imstats.$(ARCH).o	   \
$(SRC)/style.$(ARCH).o		   \
$(SRC)/subraster.$(ARCH).o	   \
$(SRC)/subset.$(ARCH).o	           \
$(SRC)/svd.$(ARCH).o		   \
$(SRC)/swapbytes.$(ARCH).o	   \
$(SRC)/textline.$(ARCH).o	   \
$(SRC)/threshold.$(ARCH).o		\
$(SRC)/triangle.$(ARCH).o		   \
$(SRC)/tv.$(ARCH).o		   \
$(SRC)/tvchannel.$(ARCH).o	   \
$(SRC)/tvcolors.$(ARCH).o	   \
$(SRC)/tvcontour.$(ARCH).o	   \
$(SRC)/tvgrid.$(ARCH).o	           \
$(SRC)/type.$(ARCH).o		   \
$(SRC)/uniq.$(ARCH).o		   \
$(SRC)/uniqpair.$(ARCH).o		   \
$(SRC)/unsign.$(ARCH).o	           \
$(SRC)/vbin.$(ARCH).o		   \
$(SRC)/vgroup.$(ARCH).o		   \
$(SRC)/vclip.$(ARCH).o		   \
$(SRC)/vgauss.$(ARCH).o            \
$(SRC)/vlorentz.$(ARCH).o          \
$(SRC)/vsigmoid.$(ARCH).o          \
$(SRC)/vellipse.$(ARCH).o          \
$(SRC)/vmaxwell.$(ARCH).o          \
$(SRC)/vgrid.$(ARCH).o		   \
$(SRC)/vlist.$(ARCH).o		   \
$(SRC)/vload.$(ARCH).o		   \
$(SRC)/vzload.$(ARCH).o		   \
$(SRC)/vpeaks.$(ARCH).o		   \
$(SRC)/vtransitions.$(ARCH).o     \
$(SRC)/vpop.$(ARCH).o		   \
$(SRC)/vroll.$(ARCH).o		   \
$(SRC)/vshift.$(ARCH).o		   \
$(SRC)/vmedfilt.$(ARCH).o	   \
$(SRC)/vsmooth.$(ARCH).o	   \
$(SRC)/vstats.$(ARCH).o		   \
$(SRC)/virls.$(ARCH).o		   \
$(SRC)/vwtmean.$(ARCH).o \
$(SRC)/xsection.$(ARCH).o          \
$(SRC)/vsh.$(ARCH).o	           \
$(SRC)/vshfit.$(ARCH).o	   \
$(SRC)/shterms.$(ARCH).o	   \
$(SRC)/shfit.$(ARCH).o	   \
$(SRC)/shdot.$(ARCH).o	   \
$(SRC)/shapply.$(ARCH).o	   \
$(SRC)/wd.$(ARCH).o		   \
$(SRC)/write_vectors.$(ARCH).o	   \
$(SRC)/zap.$(ARCH).o		   \
$(SRC)/zplot.$(ARCH).o

# dependancy rules for include files ########################
incs = \
$(INC)/opihi.h \
$(INC)/external.h \
$(INC)/shell.h \
$(INC)/data.h \
$(INC)/dvomath.h \
$(INC)/display.h 

$(srcs): $(incs)

$(LIB)/libdatacmd.$(ARCH).a: $(srcs)
$(LIB)/libdatacmd.$(ARCH).$(DLLTYPE): $(srcs)

$(DESTLIB)/libdatacmd.a: $(LIB)/libdatacmd.$(ARCH).a
$(DESTLIB)/libdatacmd.$(DLLTYPE): $(LIB)/libdatacmd.$(ARCH).$(DLLTYPE)

libdatacmd: $(DESTLIB)/libdatacmd.a  $(DESTLIB)/libdatacmd.$(DLLTYPE)

uninstall:
	rm -f $(DESTLIB)/libdatacmd.a
	rm -f $(DESTLIB)/libdatacmd.$(DLLTYPE)
