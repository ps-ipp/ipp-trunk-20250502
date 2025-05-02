default: gastro
help:
	@echo "make options: gastro (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/gastro
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -lkapa -ldvo -lFITS -lohana $(BASE_LDFLAGS)

gastro: $(BIN)/gastro.$(ARCH)
install: $(DESTBIN)/gastro

GASTRO	= \
$(SRC)/gastro.$(ARCH).o \
$(SRC)/gstars.$(ARCH).o \
$(SRC)/gargs.$(ARCH).o \
$(SRC)/gcenter.$(ARCH).o \
$(SRC)/granges.$(ARCH).o \
$(SRC)/gfit.$(ARCH).o \
$(SRC)/rotate.$(ARCH).o \
$(SRC)/gproject.$(ARCH).o \
$(SRC)/gheader.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/gfitpoly.$(ARCH).o \
$(SRC)/get_region_coords.$(ARCH).o \
$(SRC)/greference.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/getptolemy.$(ARCH).o \
$(SRC)/plotstuff.$(ARCH).o \
$(SRC)/misc.$(ARCH).o

$(GASTRO): $(INC)/gastro.h
$(BIN)/gastro.$(ARCH): $(GASTRO)

