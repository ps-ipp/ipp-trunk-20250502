default: checkastro checkastro_client
help:
	@echo "make options: checkastro (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/checkastro
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

checkastro: $(BIN)/checkastro.$(ARCH)
checkastro_client: $(BIN)/checkastro_client.$(ARCH)

install: $(DESTBIN)/checkastro $(DESTBIN)/checkastro_client $(DESTBIN)/testparallax

CHECKASTRO = \
$(SRC)/ConfigInit.$(ARCH).o	     \
$(SRC)/ImageOps.$(ARCH).o	     \
$(SRC)/SetSignals.$(ARCH).o 	     \
$(SRC)/Shutdown.$(ARCH).o 	     \
$(SRC)/args.$(ARCH).o		     \
$(SRC)/bcatalog.$(ARCH).o	     \
$(SRC)/initialize.$(ARCH).o	     \
$(SRC)/load_catalogs.$(ARCH).o	     \
$(SRC)/load_images.$(ARCH).o	     \
$(SRC)/select_images.$(ARCH).o	     \
$(SRC)/checkastro.$(ARCH).o	     \
$(SRC)/checkastro_images.$(ARCH).o   \
$(SRC)/MeasFilterTest.$(ARCH).o      \
$(SRC)/BrightCatalog.$(ARCH).o

$(CHECKASTRO): $(INC)/checkastro.h
$(BIN)/checkastro.$(ARCH): $(CHECKASTRO)

CHECKASTRO_CLIENT = \
$(SRC)/ConfigInit.$(ARCH).o	     \
$(SRC)/SetSignals.$(ARCH).o 	     \
$(SRC)/Shutdown.$(ARCH).o 	     \
$(SRC)/args.$(ARCH).o		     \
$(SRC)/bcatalog.$(ARCH).o	     \
$(SRC)/initialize.$(ARCH).o	     \
$(SRC)/load_catalogs.$(ARCH).o	     \
$(SRC)/checkastro_client.$(ARCH).o   \
$(SRC)/MeasFilterTest.$(ARCH).o      \
$(SRC)/BrightCatalog.$(ARCH).o 

$(CHECKASTRO_CLIENT): $(INC)/checkastro.h
$(BIN)/checkastro_client.$(ARCH): $(CHECKASTRO_CLIENT)

