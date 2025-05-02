default: dvolens dvolens_client
help:
	@echo "make options: dvolens (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/dvolens
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

dvolens: $(BIN)/dvolens.$(ARCH)
dvolens_client: $(BIN)/dvolens_client.$(ARCH)

install: $(DESTBIN)/dvolens $(DESTBIN)/dvolens_client

DVOLENS = \
$(SRC)/load_images.$(ARCH).o 	 \
$(SRC)/select_images.$(ARCH).o 	 \
$(SRC)/WarpImageMaps.$(ARCH).o 	 \
$(SRC)/ConfigInit.$(ARCH).o	 \
$(SRC)/SetSignals.$(ARCH).o 	 \
$(SRC)/Shutdown.$(ARCH).o 	 \
$(SRC)/args.$(ARCH).o		 \
$(SRC)/help.$(ARCH).o		 \
$(SRC)/initialize.$(ARCH).o	 \
$(SRC)/myIndex.$(ARCH).o	 \
$(SRC)/mySequence.$(ARCH).o	 \
$(SRC)/update_objects.$(ARCH).o	 \
$(SRC)/update_objects_catalog.$(ARCH).o	 \
$(SRC)/client_logger.$(ARCH).o	 \
$(SRC)/dvolens.$(ARCH).o

$(DVOLENS): $(INC)/dvolens.h
$(BIN)/dvolens.$(ARCH): $(DVOLENS)

DVOLENS_CLIENT = \
$(SRC)/load_images.$(ARCH).o 	 \
$(SRC)/select_images.$(ARCH).o 	 \
$(SRC)/WarpImageMaps.$(ARCH).o 	 \
$(SRC)/ConfigInit.$(ARCH).o	 \
$(SRC)/SetSignals.$(ARCH).o 	 \
$(SRC)/Shutdown.$(ARCH).o 	 \
$(SRC)/args.$(ARCH).o		 \
$(SRC)/help.$(ARCH).o		 \
$(SRC)/initialize.$(ARCH).o	 \
$(SRC)/myIndex.$(ARCH).o	 \
$(SRC)/mySequence.$(ARCH).o	 \
$(SRC)/update_objects.$(ARCH).o	 \
$(SRC)/update_objects_catalog.$(ARCH).o	 \
$(SRC)/client_logger.$(ARCH).o	 \
$(SRC)/dvolens_client.$(ARCH).o

$(DVOLENS_CLIENT): $(INC)/dvolens.h
$(BIN)/dvolens_client.$(ARCH): $(DVOLENS_CLIENT)
