default: uniphot
help:
	@echo "make options: uniphot (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/fixcat
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

uniphot: $(BIN)/uniphot.$(ARCH)
install: $(DESTBIN)/uniphot

FIXCAT = \
$(SRC)/fixcat.$(ARCH).o 	$(SRC)/gcatalog.$(ARCH).o   \
$(SRC)/coordops.$(ARCH).o	$(SRC)/sorts.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o 	$(SRC)/config.$(ARCH).o     \
$(SRC)/Fread.$(ARCH).o		$(SRC)/check_lockfile.$(ARCH).o \
$(SRC)/find_images.$(ARCH).o	$(SRC)/match_images.$(ARCH).o	\
$(SRC)/gregions.$(ARCH).o	$(SRC)/aregion.$(ARCH).o	\
$(SRC)/find_funnymags.$(ARCH).o	$(SRC)/gcatstats.$(ARCH).o\
$(SRC)/find_matches.$(ARCH).o	$(SRC)/check_permissions.$(ARCH).o

$(UNIPHOT): $(INC)/uniphot.h
$(BIN)/uniphot.$(ARCH): $(UNIPHOT)
