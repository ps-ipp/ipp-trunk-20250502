default: install
help:
	@echo "make options: install libdvo clean dist"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/libdvo
AUTO	=	$(ROOT)/src/libautocode
BIN	=	$(HOME)/bin
LIB	= 	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	=	$(HOME)/include
TESTDIR	=	$(HOME)/test
TESTBIN =       $(HOME)/test
ASRC	=	$(AUTO)/src
AINC	=	$(AUTO)/include
ADEF	=	$(AUTO)/def
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS) -lFITS -lohana

TEST_CFLAGS   =	$(BASE_CFLAGS)
TEST_CPPFLAGS =	$(BASE_CPPFLAGS)
TEST_LDFLAGS  = $(BASE_LDFLAGS) -ldvo -lFITS -lohana -ltap_ohana

TESTPROG = tap_AstromOffsetMapIO
$(TESTPROG) : % : $(TESTBIN)/%
test: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 | $(TESTHARNESS); done
test.verbose: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 |& $(TESTHARNESS) -v; done

install: $(DESTLIB)/libdvo.a $(DESTLIB)/libdvo.$(DLLTYPE)
libdvo: $(LIB)/libdvo.$(ARCH).a $(LIB)/libdvo.$(ARCH).$(DLLTYPE)

DEFS = \
$(DESTINC)/loneos_defs.h \
$(DESTINC)/elixir_defs.h \
$(DESTINC)/panstarrs_dev_0_defs.h \
$(DESTINC)/panstarrs_dev_1_defs.h \
$(DESTINC)/ps1_dev_1_defs.h \
$(DESTINC)/ps1_dev_2_defs.h \
$(DESTINC)/ps1_dev_3_defs.h \
$(DESTINC)/ps1_v1_defs.h \
$(DESTINC)/ps1_v2_defs.h \
$(DESTINC)/ps1_v3_defs.h \
$(DESTINC)/ps1_v4_defs.h \
$(DESTINC)/ps1_v5_defs.h \
$(DESTINC)/ps1_v6_defs.h \
$(DESTINC)/ps1_v5_ld_defs.h \
$(DESTINC)/ps1_ref_defs.h \
$(DESTINC)/ps1_ref_v2_defs.h \
$(DESTINC)/ps1_ref_v3_defs.h \
$(DESTINC)/ps1_sim_defs.h \
$(DESTINC)/cmf-ps1-dv3.h \
$(DESTINC)/cmf-ps1-v5.h \
$(DESTINC)/cmf-ps1-v5-r0.h \
$(DESTINC)/cmf-ps1-sv3.h \
$(DESTINC)/cmf-ps1-sv4.h \
$(DESTINC)/cmf-ps1-v5-r0-lensing.h \
$(DESTINC)/cmf-ps1-v5-r1-lensing.h \
$(DESTINC)/cmf-ps1-v5-r2-lensing.h

INCS = $(DEFS) $(DESTINC)/dvo.h $(DESTINC)/autocode.h $(DESTINC)/dvo_util.h $(DESTINC)/dvodb.h $(DESTINC)/libdvo_astro.h $(DESTINC)/convert.h $(DESTINC)/get_graphdata.h

OBJS = \
$(SRC)/version.$(ARCH).o	 \
$(SRC)/coordops.$(ARCH).o	 \
$(SRC)/galaxy_model.$(ARCH).o	 \
$(SRC)/dvosorts.$(ARCH).o	 \
$(SRC)/dvo_photcode_ops.$(ARCH).o \
$(SRC)/dvo_photcode_utils.$(ARCH).o \
$(SRC)/LoadPhotcodes.$(ARCH).o   \
$(SRC)/LoadPhotcodesText.$(ARCH).o   \
$(SRC)/LoadPhotcodesFITS.$(ARCH).o   \
$(SRC)/SavePhotcodesText.$(ARCH).o   \
$(SRC)/SavePhotcodesFITS.$(ARCH).o   \
$(SRC)/imreg_datatypes.$(ARCH).o \
$(SRC)/mosaic_astrom.$(ARCH).o   \
$(SRC)/fits_db.$(ARCH).o	 \
$(SRC)/dvo_tiny_values.$(ARCH).o       \
$(SRC)/dvo_image.$(ARCH).o       \
$(SRC)/dvo_image_raw.$(ARCH).o   \
$(SRC)/dvo_catalog.$(ARCH).o     \
$(SRC)/dvo_catalog_raw.$(ARCH).o       \
$(SRC)/dvo_catalog_mef.$(ARCH).o       \
$(SRC)/dvo_catalog_split.$(ARCH).o     \
$(SRC)/dvo_catalog_create.$(ARCH).o    \
$(SRC)/dvo_catalog_chipcoords.$(ARCH).o \
$(SRC)/dvo_convert.$(ARCH).o           \
$(SRC)/dvo_convert_elixir.$(ARCH).o    \
$(SRC)/dvo_convert_loneos.$(ARCH).o    \
$(SRC)/dvo_convert_panstarrs_DEV_0.$(ARCH).o \
$(SRC)/dvo_convert_panstarrs_DEV_1.$(ARCH).o \
$(SRC)/dvo_convert_PS1_DEV_1.$(ARCH).o \
$(SRC)/dvo_convert_PS1_DEV_2.$(ARCH).o \
$(SRC)/dvo_convert_PS1_DEV_3.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V1.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V2.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V3.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V4.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V5.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V6.$(ARCH).o \
$(SRC)/dvo_convert_PS1_V5_LOAD.$(ARCH).o \
$(SRC)/dvo_convert_PS1_REF.$(ARCH).o \
$(SRC)/dvo_convert_PS1_REF_V2.$(ARCH).o \
$(SRC)/dvo_convert_PS1_REF_V3.$(ARCH).o \
$(SRC)/dvo_convert_PS1_SIM.$(ARCH).o \
$(SRC)/flatcorr_io.$(ARCH).o    \
$(SRC)/skyregion_io.$(ARCH).o    \
$(SRC)/skyregion_gsc.$(ARCH).o    \
$(SRC)/skyregion_ops.$(ARCH).o \
$(SRC)/AstromOffsetMapIO.$(ARCH).o    \
$(SRC)/AstromOffsetMapOps.$(ARCH).o \
$(SRC)/AstromOffsetMapUtils.$(ARCH).o \
$(SRC)/dvo_set_skyregion.$(ARCH).o \
$(SRC)/dvo_set_catdir.$(ARCH).o \
$(SRC)/cmf-ps1-v1-alt.$(ARCH).o \
$(SRC)/cmf-ps1-sv1-alt.$(ARCH).o \
$(SRC)/cmf-ps1-dv3.$(ARCH).o \
$(SRC)/cmf-ps1-sv3.$(ARCH).o \
$(SRC)/cmf-ps1-sv4.$(ARCH).o \
$(SRC)/cmf-ps1-v5.$(ARCH).o \
$(SRC)/cmf-ps1-v5-r0.$(ARCH).o \
$(SRC)/cmf-ps1-v5-r0-lensing.$(ARCH).o \
$(SRC)/cmf-ps1-v5-r1-lensing.$(ARCH).o \
$(SRC)/cmf-ps1-v5-r2-lensing.$(ARCH).o \
$(SRC)/dvo_util.$(ARCH).o \
$(SRC)/dbBooleanCond.$(ARCH).o		\
$(SRC)/dbCheckStack.$(ARCH).o		\
$(SRC)/dbCmdlineFields.$(ARCH).o	\
$(SRC)/dbExtractAverages.$(ARCH).o	\
$(SRC)/dbExtractMeasures.$(ARCH).o	\
$(SRC)/dbExtractImages.$(ARCH).o	\
$(SRC)/dbFields.$(ARCH).o	\
$(SRC)/dbRPN.$(ARCH).o			\
$(SRC)/dbStackMath.$(ARCH).o		\
$(SRC)/dbStackOps.$(ARCH).o 		\
$(SRC)/coord_systems.$(ARCH).o		\
$(SRC)/LoadImages.$(ARCH).o		\
$(SRC)/ImageSelection.$(ARCH).o		\
$(SRC)/ImageMetadataSelection.$(ARCH).o \
$(SRC)/ImageMetadata.$(ARCH).o \
$(SRC)/ImageOps.$(ARCH).o		\
$(SRC)/RegionHostTable.$(ARCH).o		\
$(SRC)/match_image.$(ARCH).o		\
$(SRC)/db_utils.$(ARCH).o		\
$(SRC)/convert.$(ARCH).o                \
$(SRC)/HostTable.$(ARCH).o              \
$(SRC)/BoundaryTree.$(ARCH).o           \
$(SRC)/TessellationTable.$(ARCH).o

# $(SRC)/dvo_convert_panstarrs.$(ARCH).o 
# $(SRC)/dvo_convert_pmtest.$(ARCH).o 
# $(SRC)/photfits.$(ARCH).o        

# libautocode/Makefile.Targets defines $(AOBJS) & $(AINCS)
include ../libautocode/Makefile.Targets

$(OBJS): $(INCS)

$(LIB)/libdvo.$(ARCH).a:  $(AOBJS) $(OBJS)
$(LIB)/libdvo.$(ARCH).$(DLLTYPE): $(AOBJS) $(OBJS)

$(DESTLIB)/libdvo.a:  $(LIB)/libdvo.$(ARCH).a
$(DESTLIB)/libdvo.$(DLLTYPE): $(LIB)/libdvo.$(ARCH).$(DLLTYPE)

# dependency rules for the autocode files

$(AOBJS): $(ADEF)/autocode.c $(DESTINC)/autocode.h

$(DESTINC)/autocode.h: $(AINC)/autocode.h
	@if [ ! -d $(DESTINC) ]; then mkdir -p $(DESTINC); fi
	/bin/rm -f $@
	/bin/cp -f $< $@
	echo "done making autocode.h"

$(AINC)/autocode.h: $(AINCS) $(ADEF)/autocode.h $(ADEF)/common.h
#	@echo "$(MAKE) $@ from $^"
	cd $(AUTO) && $(MAKE) $@

# generic rule for AINCS
$(AINC)/%.h : $(ADEF)/%.d $(ADEF)/autocode.h $(ADEF)/common.h
#	@echo "*** h-build $(MAKE) $@ from $^"
	cd $(AUTO) && $(MAKE) $@

# generic rule for ASRC
$(ASRC)/%.c : $(ADEF)/%.d $(ADEF)/autocode.c
#	@echo "*** c-build: $(MAKE) $@ from $^"
	cd $(AUTO) && $(MAKE) $@

# generic rule for AOBJS
$(ASRC)/%.$(ARCH).o : $(ASRC)/%.c
#	@echo "*** o-build: $(MAKE) $@ from $^"
	cd $(AUTO) && $(MAKE) $@

tabletest: install
	gcc -L$(LIBDIR) -I$(INCDIR) -o tabletest $(SRC)/tabletest.c -ldvo -lFITS -lohana -lm

clean: cleandef
cleandef:
	cd $(AUTO) && $(MAKE) clean

# .PRECIOUS: $(ASRC)/%.c
