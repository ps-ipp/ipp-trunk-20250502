default: mosastro
help:
	@echo "make options: mosastro (default) mkstandards mkobs warptest"

include ../../Makefile.System
HOME    =       $(ROOT)/src/mosastro
LIB	=	$(HOME)/lib
BIN	=	$(HOME)/bin
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

mosastro: $(BIN)/mosastro.$(ARCH)
mkstandards: $(BIN)/mkstandards.$(ARCH)
mkobs: $(BIN)/mkobs.$(ARCH)
warptest: $(BIN)/warptest.(ARCH)
install: $(DESTBIN)/mosastro $(DESTBIN)/mosastro $(DESTBIN)/mosastro

MOS = \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/ConvertMatch.$(ARCH).o \
$(SRC)/FitChips.$(ARCH).o \
$(SRC)/FitGradients.$(ARCH).o \
$(SRC)/GetGradients.$(ARCH).o \
$(SRC)/GetScatter.$(ARCH).o \
$(SRC)/LoadStars.$(ARCH).o \
$(SRC)/SaveResiduals.$(ARCH).o \
$(SRC)/args.$(ARCH).o \
$(SRC)/chips.$(ARCH).o \
$(SRC)/clip.$(ARCH).o \
$(SRC)/dump.$(ARCH).o \
$(SRC)/field.$(ARCH).o \
$(SRC)/fitpoly.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/getptolemy.$(ARCH).o \
$(SRC)/getstone.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/getusnob.$(ARCH).o \
$(SRC)/greference.$(ARCH).o \
$(SRC)/match.$(ARCH).o \
$(SRC)/mkheader.$(ARCH).o \
$(SRC)/mkmosaic.$(ARCH).o \
$(SRC)/mkpolyterm.$(ARCH).o \
$(SRC)/mosastro.$(ARCH).o \
$(SRC)/output.$(ARCH).o \
$(SRC)/parse_time.$(ARCH).o \
$(SRC)/project.$(ARCH).o \
$(SRC)/rfits.$(ARCH).o \
$(SRC)/rtext.$(ARCH).o \
$(SRC)/testcoords.$(ARCH).o \
$(SRC)/transforms.$(ARCH).o \
$(SRC)/wfits.$(ARCH).o \
$(SRC)/wstars.$(ARCH).o

STD = \
$(SRC)/mkstandards.$(ARCH).o \
$(SRC)/mkheader.$(ARCH).o \
$(SRC)/random.$(ARCH).o \
$(SRC)/wstars.$(ARCH).o \
$(SRC)/wfits.$(ARCH).o

OBS = \
$(SRC)/args_obs.$(ARCH).o \
$(SRC)/mkobs.$(ARCH).o \
$(SRC)/mkheader.$(ARCH).o \
$(SRC)/fakefield.$(ARCH).o \
$(SRC)/greference.$(ARCH).o \
$(SRC)/project.$(ARCH).o \
$(SRC)/transforms.$(ARCH).o \
$(SRC)/dump.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/get2mass.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/getptolemy.$(ARCH).o \
$(SRC)/getstone.$(ARCH).o \
$(SRC)/gptolemy.$(ARCH).o \
$(SRC)/gregions.$(ARCH).o \
$(SRC)/gcatalog.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/mkpolyterm.$(ARCH).o \
$(SRC)/random.$(ARCH).o \
$(SRC)/wstars.$(ARCH).o

$(MOS) : $(INC)/mosastro.h
$(OBS) : $(INC)/mosastro.h
$(STD) : $(INC)/mosastro.h

$(BIN)/mosastro.$(ARCH) : $(MOS)
$(BIN)/mkstandards.$(ARCH) : $(STD)
$(BIN)/mkobs.$(ARCH) : $(OBS)
$(BIN)/warptest.(ARCH) : $(SRC)/warptest.$(ARCH).o
