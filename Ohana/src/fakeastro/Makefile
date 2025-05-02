default: fakeastro fakeastro_client
help:
	@echo "make options: fakeastro (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/fakeastro
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

fakeastro: $(BIN)/fakeastro.$(ARCH)
fakeastro_client: $(BIN)/fakeastro_client.$(ARCH)

install: $(DESTBIN)/fakeastro $(DESTBIN)/fakeastro_client

FAKEASTRO = \
$(SRC)/ConfigInit.$(ARCH).o	     \
$(SRC)/SetSignals.$(ARCH).o 	     \
$(SRC)/Shutdown.$(ARCH).o 	     \
$(SRC)/args.$(ARCH).o		     \
$(SRC)/initialize.$(ARCH).o	     \
$(SRC)/fakeastro.$(ARCH).o	     \
$(SRC)/fakeastro_galaxy.$(ARCH).o    \
$(SRC)/StarOps.$(ARCH).o    \
$(SRC)/make_fakestars.$(ARCH).o \
$(SRC)/make_fakeqsos.$(ARCH).o \
$(SRC)/make_subset.$(ARCH).o \
$(SRC)/save_fakestars.$(ARCH).o \
$(SRC)/fakestar_catalog.$(ARCH).o \
$(SRC)/fakestar_io.$(ARCH).o \
$(SRC)/insert_fakestar.$(ARCH).o \
$(SRC)/fakeastro_images.$(ARCH).o \
$(SRC)/fakeastro_images_region.$(ARCH).o \
$(SRC)/load_template_images.$(ARCH).o \
$(SRC)/make_fake_images.$(ARCH).o \
$(SRC)/fit_fake_stars.$(ARCH).o \
$(SRC)/fitpoly.$(ARCH).o \
$(SRC)/AstromOffsetMapOps.$(ARCH).o \
$(SRC)/airmass.$(ARCH).o \
$(SRC)/get_image_patch.$(ARCH).o \
$(SRC)/load_fake_stars.$(ARCH).o \
$(SRC)/make_fake_stars.$(ARCH).o \
$(SRC)/make_fake_stars_catalog.$(ARCH).o \
$(SRC)/save_fake_stars.$(ARCH).o \
$(SRC)/match_fake_stars.$(ARCH).o \
$(SRC)/fakeastro_2mass.$(ARCH).o \
$(SRC)/fakeastro_gaia.$(ARCH).o \
$(SRC)/make_2mass_measures.$(ARCH).o \
$(SRC)/make_gaia_measures.$(ARCH).o \
$(SRC)/remote_hosts.$(ARCH).o 

#$(SRC)/fakeastro_images.$(ARCH).o    \

$(FAKEASTRO): $(INC)/fakeastro.h
$(BIN)/fakeastro.$(ARCH): $(FAKEASTRO)

FAKEASTRO_CLIENT = \
$(SRC)/ConfigInit.$(ARCH).o	     \
$(SRC)/SetSignals.$(ARCH).o 	     \
$(SRC)/Shutdown.$(ARCH).o 	     \
$(SRC)/args.$(ARCH).o		     \
$(SRC)/initialize.$(ARCH).o	     \
$(SRC)/fakeastro_client.$(ARCH).o	\
$(SRC)/fakestar_io.$(ARCH).o \
$(SRC)/fakestar_catalog.$(ARCH).o \
$(SRC)/insert_fakestar.$(ARCH).o \

$(FAKEASTRO_CLIENT): $(INC)/fakeastro.h
$(BIN)/fakeastro_client.$(ARCH): $(FAKEASTRO_CLIENT)

