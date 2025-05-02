default: all
help:
	@echo "make options: dvosplit (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/dvosplit
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

dvosplit     : $(BIN)/dvosplit.$(ARCH)
dvosplitsky  : $(BIN)/dvosplitsky.$(ARCH)

all: dvosplit dvosplitsky

DVOSPLIT = \
$(SRC)/dvosplit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args.$(ARCH).o \
$(SRC)/split_averages.$(ARCH).o \
$(SRC)/split_measures.$(ARCH).o \
$(SRC)/open_output_catalogs.$(ARCH).o

$(DVOSPLIT)  : $(INC)/dvosplit.h

$(BIN)/dvosplit.$(ARCH) : $(DVOSPLIT)

DVOSPLITSKY = \
$(SRC)/dvosplitsky.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_sky.$(ARCH).o

$(DVOSPLITSKY)  : $(INC)/dvosplit.h

$(BIN)/dvosplitsky.$(ARCH) : $(DVOSPLITSKY)

INSTALL = dvosplit dvosplitsky

# dependancy rules for binary code #########################
$(INSTALL): % : $(BIN)/%.$(ARCH)

%.clean :
	rm -f $(BIN)/$*.$(ARCH)

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(INSTALL); do $(MAKE) $$i.install || exit; done
