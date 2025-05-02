default: all
help:
	@echo "make options: all imregister etc (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/imregister
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

BASEDIR =	$(HOME)/base
DETREND =	$(HOME)/detrend
PHOTREG =	$(HOME)/photreg
IMREG   =	$(HOME)/imreg
IMPHOT  =	$(HOME)/imphot
SPREG   =	$(HOME)/spreg

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

# programs which should be installed
INSTALL = \
imregister   imstatreg    \
imsort       imsearch     \
showiminfo   convertimreg \
detregister  detsearch    \
photsearch   photreg      \
imregtable   imphotsearch \
cameraconfig filtnames    \
photcode     photcode-table

# program under development, not installed
DEVEL = spregister spsearch imphotmerge

IMPHOTOBJ = \
$(IMPHOT)/args.$(ARCH).o \
$(IMPHOT)/db_load.$(ARCH).o \
$(IMPHOT)/dumpfits.$(ARCH).o \
$(IMPHOT)/modify.$(ARCH).o \
$(IMPHOT)/output.$(ARCH).o \
$(IMPHOT)/rfits.$(ARCH).o \
$(IMPHOT)/rtext.$(ARCH).o \
$(IMPHOT)/subset.$(ARCH).o

$(SRC)/imphotsearch.$(ARCH).o \
$(SRC)/imphotmerge.$(ARCH).o \
$(IMPHOTOBJ): $(INC)/imphot.h $(INC)/imregister.h

IMOBJ  = \
$(IMREG)/delete.$(ARCH).o \
$(IMREG)/iminfo.$(ARCH).o \
$(IMREG)/load_probes.$(ARCH).o  \
$(IMREG)/match.$(ARCH).o  \
$(IMREG)/modify.$(ARCH).o \
$(IMREG)/newimages.$(ARCH).o \
$(IMREG)/output.$(ARCH).o \
$(IMREG)/unique.$(ARCH).o \
$(IMREG)/cadc.$(ARCH).o   \
$(IMREG)/mosaics.$(ARCH).o   \
$(IMREG)/ConfigPID.$(ARCH).o   \
$(IMREG)/SetSignals.$(ARCH).o   \
$(IMREG)/FifoOps.$(ARCH).o   \
$(IMREG)/SockScan.$(ARCH).o   \
$(IMREG)/submit.$(ARCH).o \
$(IMREG)/imregclient.$(ARCH).o

$(SRC)/imsearch.$(ARCH).o \
$(SRC)/imregister.$(ARCH).o \
$(SRC)/imstatreg.$(ARCH).o \
$(SRC)/showiminfo.$(ARCH).o \
$(SRC)/imregtable.$(ARCH).o \
$(SRC)/convertimreg.$(ARCH).o \
$(IMREG)/args.imsearch.$(ARCH).o \
$(IMREG)/args.imregister.$(ARCH).o \
$(IMOBJ): $(INC)/imreg.h $(INC)/imregister.h

SPOBJ  = \
$(SPREG)/delete.$(ARCH).o   \
$(SPREG)/match.$(ARCH).o    \
$(SPREG)/modify.$(ARCH).o   \
$(SPREG)/output.$(ARCH).o   \
$(SPREG)/unique.$(ARCH).o   \
$(SPREG)/spinfo.$(ARCH).o   \
$(SPREG)/showinfo.$(ARCH).o \
$(SPREG)/ConfigInit.$(ARCH).o

$(SRC)/spregister.$(ARCH).o \
$(SPREG)/args.spregister.$(ARCH).o \
$(SRC)/spsearch.$(ARCH).o \
$(SPREG)/args.spsearch.$(ARCH).o \
$(SPOBJ): $(INC)/spreg.h $(INC)/imregister.h

PHOTOBJ  = \
$(PHOTREG)/delete.$(ARCH).o  \
$(PHOTREG)/match.$(ARCH).o   \
$(PHOTREG)/convert.$(ARCH).o \
$(PHOTREG)/output.$(ARCH).o  \
$(PHOTREG)/getImageData.$(ARCH).o

$(SRC)/photreg.$(ARCH).o \
$(SRC)/photsearch.$(ARCH).o \
$(PHOTREG)/args.photreg.$(ARCH).o \
$(PHOTREG)/args.photsearch.$(ARCH).o \
$(PHOTOBJ): $(INC)/photreg.h $(INC)/imregister.h

DETOBJ  = \
$(DETREND)/criteria.$(ARCH).o  \
$(DETREND)/delete.$(ARCH).o    \
$(DETREND)/entry.$(ARCH).o     \
$(DETREND)/image.$(ARCH).o     \
$(DETREND)/imdef.$(ARCH).o     \
$(DETREND)/match.$(ARCH).o     \
$(DETREND)/modify.$(ARCH).o    \
$(DETREND)/mosaic.$(ARCH).o    \
$(DETREND)/output.$(ARCH).o    \
$(DETREND)/recipe.$(ARCH).o    \
$(DETREND)/select.$(ARCH).o    \
$(DETREND)/unique.$(ARCH).o    \
$(DETREND)/altpath.$(ARCH).o   \
$(DETREND)/db_names.$(ARCH).o   \
$(DETREND)/usage.$(ARCH).o

$(SRC)/detregister.$(ARCH).o  \
$(SRC)/detsearch.$(ARCH).o \
$(DETREND)/args.detregister.$(ARCH).o \
$(DETREND)/args.detsearch.$(ARCH).o \
$(DETOBJ): $(INC)/detrend.h $(INC)/imregister.h

BASEOBJ = \
$(BASEDIR)/ConfigCamera.$(ARCH).o \
$(BASEDIR)/ConfigFilter.$(ARCH).o \
$(BASEDIR)/ConfigInit.$(ARCH).o   \
$(BASEDIR)/WriteFIFO.$(ARCH).o    \
$(BASEDIR)/get_fwhm.$(ARCH).o     \
$(BASEDIR)/misc.$(ARCH).o         \
$(BASEDIR)/version.$(ARCH).o      \
$(BASEDIR)/parse_time.$(ARCH).o   \
$(BASEDIR)/fits_scan_nchar.$(ARCH).o

$(SRC)/photcode-table.$(ARCH).o \
$(SRC)/photcode.$(ARCH).o \
$(SRC)/cameraconfig.$(ARCH).o \
$(SRC)/filtnames.$(ARCH).o \
$(BASEOBJ): $(INC)/imregister.h

# specific programs:
$(BIN)/imsearch.$(ARCH)     : $(SRC)/imsearch.$(ARCH).o     $(IMREG)/args.imsearch.$(ARCH).o      $(IMOBJ)   $(BASEOBJ)
$(BIN)/imregister.$(ARCH)   : $(SRC)/imregister.$(ARCH).o   $(IMREG)/args.imregister.$(ARCH).o    $(IMOBJ)   $(BASEOBJ)
$(BIN)/imstatreg.$(ARCH)    : $(SRC)/imstatreg.$(ARCH).o    $(IMREG)/args.imregister.$(ARCH).o    $(IMOBJ)   $(BASEOBJ)
$(BIN)/showiminfo.$(ARCH)   : $(SRC)/showiminfo.$(ARCH).o   $(IMREG)/args.imregister.$(ARCH).o    $(IMOBJ)   $(BASEOBJ)
$(BIN)/imregtable.$(ARCH)   : $(SRC)/imregtable.$(ARCH).o   $(IMREG)/args.imregister.$(ARCH).o    $(IMOBJ)   $(BASEOBJ)
$(BIN)/convertimreg.$(ARCH) : $(SRC)/convertimreg.$(ARCH).o $(IMREG)/args.imregister.$(ARCH).o    $(IMOBJ)   $(BASEOBJ)
$(BIN)/detregister.$(ARCH)  : $(SRC)/detregister.$(ARCH).o  $(DETREND)/args.detregister.$(ARCH).o $(DETOBJ)  $(BASEOBJ)
$(BIN)/detsearch.$(ARCH)    : $(SRC)/detsearch.$(ARCH).o    $(DETREND)/args.detsearch.$(ARCH).o   $(DETOBJ)  $(BASEOBJ)
$(BIN)/photreg.$(ARCH)      : $(SRC)/photreg.$(ARCH).o      $(PHOTREG)/args.photreg.$(ARCH).o     $(PHOTOBJ) $(BASEOBJ)
$(BIN)/photsearch.$(ARCH)   : $(SRC)/photsearch.$(ARCH).o   $(PHOTREG)/args.photsearch.$(ARCH).o  $(PHOTOBJ) $(BASEOBJ)
$(BIN)/spregister.$(ARCH)   : $(SRC)/spregister.$(ARCH).o   $(SPREG)/args.spregister.$(ARCH).o    $(SPOBJ)   $(BASEOBJ)
$(BIN)/spsearch.$(ARCH)     : $(SRC)/spsearch.$(ARCH).o     $(SPREG)/args.spsearch.$(ARCH).o      $(SPOBJ)   $(BASEOBJ)
$(BIN)/imphotsearch.$(ARCH) : $(SRC)/imphotsearch.$(ARCH).o $(IMPHOTOBJ) $(BASEOBJ)
$(BIN)/imphotmerge.$(ARCH)  : $(SRC)/imphotmerge.$(ARCH).o  $(IMPHOTOBJ) $(BASEOBJ)
$(BIN)/cameraconfig.$(ARCH) : $(SRC)/cameraconfig.$(ARCH).o $(BASEOBJ)
$(BIN)/filtnames.$(ARCH)    : $(SRC)/filtnames.$(ARCH).o    $(BASEOBJ)
$(BIN)/photcode.$(ARCH)     : $(SRC)/photcode.$(ARCH).o     $(BASEOBJ)
$(BIN)/photcode-table.$(ARCH) : $(SRC)/photcode-table.$(ARCH).o

$(BIN)/imsort.$(ARCH): $(BIN)/imregister.$(ARCH)
	rm -f  $(BIN)/imsort.$(ARCH)
	cp $(BIN)/imregister.$(ARCH) $(BIN)/imsort.$(ARCH)

all: $(INSTALL) $(DEVEL)

$(INSTALL) $(DEVEL) : % : $(BIN)/%.$(ARCH)

# utilities #################################################

%.clean :
	rm -f $(BIN)/$*.$(ARCH)

%.rebuild :
	rm -f $(BIN)/$*.$(ARCH)
	$(MAKE) $(DESTBIN)/$*

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(INSTALL); do $(MAKE) $$i.install || exit; done
