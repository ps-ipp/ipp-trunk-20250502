default: imclean
help:
	@echo "make options: imclean (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/imclean
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

imclean: $(BIN)/imclean.$(ARCH)
install: $(DESTBIN)/imclean

IMCLEAN = \
$(SRC)/imclean.$(ARCH).o	$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/args.$(ARCH).o		$(SRC)/sort_stars.$(ARCH).o \
$(SRC)/AdjustHeader.$(ARCH).o	$(SRC)/find_group.$(ARCH).o \
$(SRC)/find_trails.$(ARCH).o	$(SRC)/find_line.$(ARCH).o \
$(SRC)/wstars.$(ARCH).o	        $(SRC)/LoadStarsDophot.$(ARCH).o \
$(SRC)/LoadStarsSex.$(ARCH).o	$(SRC)/LoadStarsChad.$(ARCH).o	\
$(SRC)/wfits.$(ARCH).o

$(IMCLEAN): $(INC)/imclean.h
$(BIN)/imclean.$(ARCH): $(IMCLEAN)
