default: nightd
help:
	@echo "make options: nightd (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/nightd
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

nightd: $(BIN)/nightd.$(ARCH)
install: $(DESTBIN)/nightd

NIGHTD = \
$(SRC)/nightd.$(ARCH).o 	$(SRC)/ConfigInit.$(ARCH).o 	\
$(SRC)/DoCommand.$(ARCH).o 	$(SRC)/GetStatus.$(ARCH).o 	\
$(SRC)/SetSignals.$(ARCH).o 	$(SRC)/StartUp.$(ARCH).o 	\
$(SRC)/misc.$(ARCH).o

$(NIGHTD): $(INC)/nightd.h
$(BIN)/nightd.$(ARCH): $(NIGHTD)
