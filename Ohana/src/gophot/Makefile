default: gophot
help:
	@echo "make options: gophot (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/gophot
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

gophot: $(BIN)/gophot.$(ARCH)
install: $(DESTBIN)/gophot

GOPHOT = \
$(SRC)/gophot.$(ARCH).o 	$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/message.$(ARCH).o	$(SRC)/dophot.$(ARCH).o \
$(SRC)/makenoise.$(ARCH).o      $(SRC)/findsky.$(ARCH).o \
$(SRC)/makemask.$(ARCH).o       $(SRC)/paravg.$(ARCH).o \
$(SRC)/objout.$(ARCH).o         $(SRC)/addstar.$(ARCH).o \
$(SRC)/ellipse.$(ARCH).o        $(SRC)/improve.$(ARCH).o \
$(SRC)/fillerup.$(ARCH).o       $(SRC)/parinterp.$(ARCH).o \
$(SRC)/guess.$(ARCH).o          $(SRC)/offpic.$(ARCH).o \
$(SRC)/addlims.$(ARCH).o        $(SRC)/parupd.$(ARCH).o \
$(SRC)/toofaint.$(ARCH).o       $(SRC)/isearch.$(ARCH).o \
$(SRC)/shape.$(ARCH).o          $(SRC)/galaxy.$(ARCH).o \
$(SRC)/transmask.$(ARCH).o      $(SRC)/cosmic.$(ARCH).o \
$(SRC)/impaper2.$(ARCH).o       \
$(SRC)/oblit.$(ARCH).o          $(SRC)/toobright.$(ARCH).o \
$(SRC)/Chisq.$(ARCH).o          $(SRC)/oblims.$(ARCH).o \
$(SRC)/ludcmp.$(ARCH).o         $(SRC)/lubksb.$(ARCH).o \
$(SRC)/pseud2d.$(ARCH).o        $(SRC)/twofit.$(ARCH).o \
$(SRC)/skyfun_plane.$(ARCH).o   $(SRC)/pseud4d.$(ARCH).o \
$(SRC)/large_features.$(ARCH).o $(SRC)/set_thresholds.$(ARCH).o \
$(SRC)/outline.$(ARCH).o        $(SRC)/delete_ellipse.$(ARCH).o \
$(SRC)/feature_fluxes.$(ARCH).o $(SRC)/mediansky.$(ARCH).o

$(GOPHOT): $(INC)/gophot.h $(INC)/prototypes.h
$(BIN)/gophot.$(ARCH): $(GOPHOT)
