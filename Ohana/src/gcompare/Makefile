default: gcompare
help:
	@echo "make options: gcompare (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/gcompare
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
MAN	=	$(HOME)/doc
SRC	=	$(HOME)/src
INC	=	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

gcompare: $(BIN)/gcompare.$(ARCH)
install: $(DESTBIN)/gcompare

GCOMPARE 	= \
$(SRC)/input.$(ARCH).o 				$(SRC)/output.$(ARCH).o    		\
$(SRC)/compare.$(ARCH).o 			$(SRC)/sort.$(ARCH).o      		\
$(SRC)/count_neighbors.$(ARCH).o                $(SRC)/nextline.$(ARCH).o		\
$(SRC)/nextword.$(ARCH).o			$(SRC)/gcompare.$(ARCH).o               \
$(SRC)/gc_compare.$(ARCH).o

$(GCOMPARE): $(INC)/gcompare.h
$(BIN)/gcompare.$(ARCH): $(GCOMPARE)
