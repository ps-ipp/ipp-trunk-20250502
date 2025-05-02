default: markstar
help:
	@echo "make options: markstar (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/markstar
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

markstar: $(BIN)/markstar.$(ARCH)
install: $(DESTBIN)/markstar

MARKSTAR = \
$(SRC)/markstar.$(ARCH).o 	$(SRC)/gcatalog.$(ARCH).o   \
$(SRC)/coordops.$(ARCH).o	$(SRC)/sorts.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o 	\
$(SRC)/check_lockfile.$(ARCH).o \
$(SRC)/find_trails.$(ARCH).o	$(SRC)/find_group.$(ARCH).o \
$(SRC)/find_line.$(ARCH).o	$(SRC)/mark_trail.$(ARCH).o \
$(SRC)/load_gsc_data.$(ARCH).o	$(SRC)/find_bright_stars.$(ARCH).o \
$(SRC)/wcatalog.$(ARCH).o	$(SRC)/find_images.$(ARCH).o	\
$(SRC)/load_gsc_data_ghost.$(ARCH).o	$(SRC)/find_ghosts.$(ARCH).o \
$(SRC)/gcatstats.$(ARCH).o	$(SRC)/match_images.$(ARCH).o	\
$(SRC)/gregions.$(ARCH).o	$(SRC)/aregion.$(ARCH).o	\
$(SRC)/find_matches.$(ARCH).o	$(SRC)/check_permissions.$(ARCH).o

$(MARKSTAR): $(INC)/markstar.h
$(BIN)/markstar.$(ARCH): $(MARKSTAR)
