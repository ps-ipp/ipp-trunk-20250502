default: all
help:
	@echo "make options: getstar (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/getstar
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

getstar: $(BIN)/getstar.$(ARCH)
getstar.install: $(DESTBIN)/getstar

dvoImageOverlaps: $(BIN)/dvoImageOverlaps.$(ARCH)
dvoImageOverlaps.install: $(DESTBIN)/dvoImageOverlaps

dvoImagesAtCoords: $(BIN)/dvoImagesAtCoords.$(ARCH)
dvoImagesAtCoords.install: $(DESTBIN)/dvoImagesAtCoords

dvoImageExtract: $(BIN)/dvoImageExtract.$(ARCH)
dvoImageExtract.install: $(DESTBIN)/dvoImageExtract

all: getstar dvoImageOverlaps dvoImageExtract dvoImagesAtCoords
install: getstar.install dvoImageOverlaps.install dvoImageExtract.install dvoImagesAtCoords.install

GETSTAR = \
$(SRC)/getstar.$(ARCH).o 	  \
$(SRC)/args.$(ARCH).o		  \
$(SRC)/ConfigInit.$(ARCH).o 	  \
$(SRC)/Shutdown.$(ARCH).o	  \
$(SRC)/SetSignals.$(ARCH).o	  \
$(SRC)/select_by_region.$(ARCH).o \
$(SRC)/write_catalog.$(ARCH).o    \
$(SRC)/write_getstar_ps1_dev_0.$(ARCH).o \
$(SRC)/write_getstar_ps1_dev_1.$(ARCH).o \
$(SRC)/write_getstar_ps1_dev_2.$(ARCH).o 

$(GETSTAR): $(INC)/getstar.h $(DVO_INCS)
$(BIN)/getstar.$(ARCH): $(GETSTAR)

OVERLAPS = \
$(SRC)/dvoImageOverlaps.$(ARCH).o 	 \
$(SRC)/args_overlaps.$(ARCH).o		 \
$(SRC)/ConfigInit_overlaps.$(ARCH).o 	 \
$(SRC)/Shutdown.$(ARCH).o	         \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/parse_time.$(ARCH).o          \
$(SRC)/ReadImageFiles.$(ARCH).o          \
$(SRC)/ReadImageHeader.$(ARCH).o         \
$(SRC)/ListImageOverlaps.$(ARCH).o       \
$(SRC)/GetFileMode.$(ARCH).o             \
$(SRC)/edge_check.$(ARCH).o              \
$(SRC)/opening_angle.$(ARCH).o           \
$(SRC)/MatchImages.$(ARCH).o

$(OVERLAPS): $(INC)/dvoImageOverlaps.h $(DVO_INCS)
$(BIN)/dvoImageOverlaps.$(ARCH): $(OVERLAPS)

COVERLAPS = \
$(SRC)/dvoImagesAtCoords.$(ARCH).o 	 \
$(SRC)/args_coords.$(ARCH).o		 \
$(SRC)/ConfigInit_coords.$(ARCH).o 	 \
$(SRC)/Shutdown.$(ARCH).o	         \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/parse_time.$(ARCH).o          \
$(SRC)/ReadImageFiles.$(ARCH).o          \
$(SRC)/ReadImageHeader.$(ARCH).o         \
$(SRC)/GetFileMode.$(ARCH).o             \
$(SRC)/MatchCoords.$(ARCH).o

$(COVERLAPS): $(INC)/dvoImagesAtCoords.h $(DVO_INCS)
$(BIN)/dvoImagesAtCoords.$(ARCH): $(COVERLAPS)

EXTRACT = \
$(SRC)/dvoImageExtract.$(ARCH).o 	 \
$(SRC)/ConfigInit_extract.$(ARCH).o 	 \
$(SRC)/args_extract.$(ARCH).o		 \
$(SRC)/Shutdown.$(ARCH).o	         \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/WriteImages.$(ARCH).o             \
$(SRC)/WriteImageFITS.$(ARCH).o          \
$(SRC)/SelectImages.$(ARCH).o       

$(EXTRACT): $(INC)/dvoImageExtract.h $(DVO_INCS)
$(BIN)/dvoImageExtract.$(ARCH): $(EXTRACT)
