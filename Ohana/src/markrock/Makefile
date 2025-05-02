default: markrock
help:
	@echo "make options: markrock (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/markrock
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

markrock: $(BIN)/markrock.$(ARCH)
install: $(DESTBIN)/markrock

MARKROCK = \
$(SRC)/markrock.$(ARCH).o 	$(SRC)/gcatalog.$(ARCH).o   	\
$(SRC)/coordops.$(ARCH).o	$(SRC)/sorts.$(ARCH).o 		\
$(SRC)/ConfigInit.$(ARCH).o 	$(SRC)/photometry.$(ARCH).o \
$(SRC)/check_lockfile.$(ARCH).o \
$(SRC)/find_rocks.$(ARCH).o 	$(SRC)/wcatalog.$(ARCH).o	\
$(SRC)/gcatstats.$(ARCH).o	$(SRC)/wrocks.$(ARCH).o 	\
$(SRC)/check_permissions.$(ARCH).o \
$(SRC)/gregions.$(ARCH).o \
$(SRC)/aregion.$(ARCH).o \
$(SRC)/find_bright_stars.$(ARCH).o \
$(SRC)/load_gsc_data.$(ARCH).o \
$(SRC)/count_neighbors.$(ARCH).o

$(MARKROCK): $(INC)/markrock.h
$(BIN)/markrock.$(ARCH): $(MARKROCK)
