default: relphot relphot_client
help:
	@echo "make options: relphot (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/relphot
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

relphot: $(BIN)/relphot.$(ARCH)
relphot_client: $(BIN)/relphot_client.$(ARCH)

test_liststats: $(BIN)/test_liststats.$(ARCH)

test_fitirls: $(BIN)/test_fitirls.$(ARCH)

install: $(DESTBIN)/relphot $(DESTBIN)/relphot_client

RELPHOT = \
$(SRC)/StatDataSetOps.$(ARCH).o	 \
$(SRC)/ConfigInit.$(ARCH).o	 \
$(SRC)/GridOps.$(ARCH).o	 \
$(SRC)/GridIO.$(ARCH).o	 \
$(SRC)/ResetOps.$(ARCH).o	 \
$(SRC)/ImageOps.$(ARCH).o	 \
$(SRC)/ImageSubset.$(ARCH).o	 \
$(SRC)/BrightCatalog.$(ARCH).o	 \
$(SRC)/MosaicOps.$(ARCH).o	 \
$(SRC)/TGroupOps.$(ARCH).o	 \
$(SRC)/ReadTGroupFile.$(ARCH).o	 \
$(SRC)/SetSignals.$(ARCH).o 	 \
$(SRC)/Shutdown.$(ARCH).o 	 \
$(SRC)/StarOps.$(ARCH).o	 \
$(SRC)/psps_ids.$(ARCH).o	 \
$(SRC)/args.$(ARCH).o		 \
$(SRC)/help.$(ARCH).o		 \
$(SRC)/extra.$(ARCH).o		 \
$(SRC)/bcatalog.$(ARCH).o	 \
$(SRC)/global_stats.$(ARCH).o	 \
$(SRC)/initialize.$(ARCH).o	 \
$(SRC)/fit1d_irls.$(ARCH).o	 \
$(SRC)/liststats.$(ARCH).o	 \
$(SRC)/load_catalogs.$(ARCH).o	 \
$(SRC)/load_images.$(ARCH).o	 \
$(SRC)/save_images.$(ARCH).o	 \
$(SRC)/MagResidSave.$(ARCH).o	 \
$(SRC)/setXradAverages.$(ARCH).o	 \
$(SRC)/lensing.$(ARCH).o	 \
$(SRC)/synthetic_mags.$(ARCH).o	 \
$(SRC)/plot_scatter.$(ARCH).o	 \
$(SRC)/plotstuff.$(ARCH).o	 \
$(SRC)/reload_catalogs.$(ARCH).o \
$(SRC)/relphot.$(ARCH).o	 \
$(SRC)/relphot_images.$(ARCH).o	 \
$(SRC)/relphot_objects.$(ARCH).o	 \
$(SRC)/relphot_parallel_regions.$(ARCH).o \
$(SRC)/relphot_parallel_images.$(ARCH).o \
$(SRC)/relphot_synthphot.$(ARCH).o	 \
$(SRC)/relphot_synthphot_catalog.$(ARCH).o	 \
$(SRC)/synthetic_zpts.$(ARCH).o	 \
$(SRC)/select_images.$(ARCH).o	 \
$(SRC)/assign_images.$(ARCH).o	 \
$(SRC)/launch_region_hosts.$(ARCH).o	 \
$(SRC)/ImageTable.$(ARCH).o	 \
$(SRC)/ImageMagIO.$(ARCH).o	 \
$(SRC)/syncfile.$(ARCH).o	 \
$(SRC)/share_image_mags.$(ARCH).o	 \
$(SRC)/share_mean_mags.$(ARCH).o	 \
$(SRC)/MeanMagIO.$(ARCH).o	 \
$(SRC)/indexCatalog.$(ARCH).o	 \
$(SRC)/client_logger.$(ARCH).o	 \
$(SRC)/ZeroPointModes.$(ARCH).o	 \
$(SRC)/setExclusions.$(ARCH).o 	 \
$(SRC)/setMrelFinal.$(ARCH).o 	 \
$(SRC)/setMrelCatalog.$(ARCH).o   \
$(SRC)/BoundaryTreeOps.$(ARCH).o  \
$(SRC)/DumpAllMags.$(ARCH).o      \
$(SRC)/write_coords.$(ARCH).o

$(RELPHOT): $(INC)/relphot.h
$(BIN)/relphot.$(ARCH): $(RELPHOT)

RELPHOT_CLIENT = \
$(SRC)/StatDataSetOps.$(ARCH).o	 \
$(SRC)/ConfigInit.$(ARCH).o	 \
$(SRC)/GridOps.$(ARCH).o	 \
$(SRC)/GridIO.$(ARCH).o	 \
$(SRC)/ResetOps.$(ARCH).o	 \
$(SRC)/ImageOps.$(ARCH).o	 \
$(SRC)/ImageSubset.$(ARCH).o	 \
$(SRC)/BrightCatalog.$(ARCH).o	 \
$(SRC)/MosaicOps.$(ARCH).o	 \
$(SRC)/TGroupOps.$(ARCH).o	 \
$(SRC)/ReadTGroupFile.$(ARCH).o	 \
$(SRC)/SetSignals.$(ARCH).o 	 \
$(SRC)/Shutdown.$(ARCH).o 	 \
$(SRC)/StarOps.$(ARCH).o	 \
$(SRC)/psps_ids.$(ARCH).o	 \
$(SRC)/bcatalog.$(ARCH).o	 \
$(SRC)/args.$(ARCH).o		 \
$(SRC)/help.$(ARCH).o		 \
$(SRC)/extra.$(ARCH).o		 \
$(SRC)/synthetic_mags.$(ARCH).o	 \
$(SRC)/plotstuff.$(ARCH).o	 \
$(SRC)/fit1d_irls.$(ARCH).o	 \
$(SRC)/liststats.$(ARCH).o	 \
$(SRC)/initialize.$(ARCH).o	 \
$(SRC)/syncfile.$(ARCH).o	 \
$(SRC)/select_images.$(ARCH).o	 \
$(SRC)/load_images.$(ARCH).o	 \
$(SRC)/save_images.$(ARCH).o	 \
$(SRC)/MagResidSave.$(ARCH).o	 \
$(SRC)/setXradAverages.$(ARCH).o	 \
$(SRC)/lensing.$(ARCH).o	 \
$(SRC)/load_catalogs.$(ARCH).o	 \
$(SRC)/reload_catalogs.$(ARCH).o \
$(SRC)/relphot_objects.$(ARCH).o	 \
$(SRC)/relphot_client.$(ARCH).o	 \
$(SRC)/relphot_synthphot.$(ARCH).o	 \
$(SRC)/relphot_synthphot_catalog.$(ARCH).o	 \
$(SRC)/synthetic_zpts.$(ARCH).o	 \
$(SRC)/client_logger.$(ARCH).o	 \
$(SRC)/ZeroPointModes.$(ARCH).o	 \
$(SRC)/setExclusions.$(ARCH).o 	 \
$(SRC)/setMrelFinal.$(ARCH).o    \
$(SRC)/setMrelCatalog.$(ARCH).o    \
$(SRC)/BoundaryTreeOps.$(ARCH).o 	 \
$(SRC)/write_coords.$(ARCH).o

$(RELPHOT_CLIENT): $(INC)/relphot.h
$(BIN)/relphot_client.$(ARCH): $(RELPHOT_CLIENT)

TEST_LISTSTATS = \
$(SRC)/StatDataSetOps.$(ARCH).o	 \
$(SRC)/liststats.$(ARCH).o	 \
$(SRC)/test_liststats.$(ARCH).o

$(TEST_LISTSTATS): $(INC)/relphot.h
$(BIN)/test_liststats.$(ARCH): $(TEST_LISTSTATS)

TEST_FITIRLS = \
$(SRC)/fit1d_irls.$(ARCH).o	 \
$(SRC)/extra.$(ARCH).o	\
$(SRC)/test_fitirls.$(ARCH).o

$(TEST_FITIRLS): $(INC)/relphot.h
$(BIN)/test_fitirls.$(ARCH): $(TEST_FITIRLS)

