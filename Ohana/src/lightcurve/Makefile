default: lightcurve
help:
	@echo "make options: lightcurve (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/lightcurve
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

lightcurve: $(BIN)/lightcurve.$(ARCH)
install: $(DESTBIN)/lightcurve

LIGHTCURVE = \
$(SRC)/args.$(ARCH).o 		$(SRC)/get_names.$(ARCH).o \
$(SRC)/get_stars.$(ARCH).o 	$(SRC)/sort_stars.$(ARCH).o \
$(SRC)/get_unique.$(ARCH).o 	$(SRC)/get_sources.$(ARCH).o \
$(SRC)/get_info.$(ARCH).o 	$(SRC)/make_table.$(ARCH).o \
$(SRC)/lightcurve.$(ARCH).o	\
$(SRC)/help.$(ARCH).o 

$(LIGHTCURVE): $(INC)/lightcurve.h
$(BIN)/lightcurve.$(ARCH): $(LIGHTCURVE)
