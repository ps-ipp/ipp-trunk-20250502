default: relastro relastro_client fitobj fitobj2 fitobj3 fitexample
help:
	@echo "make options: relastro (default)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/relastro
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
TESTDIR	=	$(HOME)/test
TESTBIN	=	$(HOME)/test
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -lkapa -ldvo -lFITS -lohana $(BASE_LDFLAGS)

relastro: $(BIN)/relastro.$(ARCH)
relastro_client: $(BIN)/relastro_client.$(ARCH)

install: $(DESTBIN)/relastro $(DESTBIN)/relastro_client 

RELASTRO = \
$(SRC)/CheckMeasureToImage.$(ARCH).o \
$(SRC)/RepairObjectIDs.$(ARCH).o \
$(SRC)/RepairStacks.$(ARCH).o \
$(SRC)/RepairStackMeasures.$(ARCH).o \
$(SRC)/StackImageMaps.$(ARCH).o \
$(SRC)/RepairWarps.$(ARCH).o \
$(SRC)/RepairWarpMeasures.$(ARCH).o \
$(SRC)/WarpImageMaps.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o \
$(SRC)/myIndex.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o	     \
$(SRC)/FitChip.$(ARCH).o             \
$(SRC)/FitMosaic.$(ARCH).o           \
$(SRC)/FitSimple.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o         \
$(SRC)/FitPosPMfixed.$(ARCH).o       \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/ImageOps.$(ARCH).o	     \
$(SRC)/MosaicOps.$(ARCH).o	     \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/SetSignals.$(ARCH).o 	     \
$(SRC)/Shutdown.$(ARCH).o 	     \
$(SRC)/UpdateChips.$(ARCH).o         \
$(SRC)/UpdateStacks.$(ARCH).o         \
$(SRC)/UpdateMosaic.$(ARCH).o        \
$(SRC)/UpdateObjects.$(ARCH).o       \
$(SRC)/UpdateObjectOffsets.$(ARCH).o \
$(SRC)/UpdateSimple.$(ARCH).o        \
$(SRC)/UpdateMeasures.$(ARCH).o      \
$(SRC)/GetAstromError.$(ARCH).o      \
$(SRC)/MeasFilterTest.$(ARCH).o            \
$(SRC)/args.$(ARCH).o		     \
$(SRC)/bcatalog.$(ARCH).o	     \
$(SRC)/dvo_astrom_ops.$(ARCH).o      \
$(SRC)/fitpoly.$(ARCH).o             \
$(SRC)/initialize.$(ARCH).o	     \
$(SRC)/liststats.$(ARCH).o	     \
$(SRC)/load_catalogs.$(ARCH).o	     \
$(SRC)/load_images.$(ARCH).o	     \
$(SRC)/mkpolyterm.$(ARCH).o          \
$(SRC)/plot_scatter.$(ARCH).o	     \
$(SRC)/plotstuff.$(ARCH).o	     \
$(SRC)/select_images.$(ARCH).o	     \
$(SRC)/relastro.$(ARCH).o	     \
$(SRC)/relastro_images.$(ARCH).o    \
$(SRC)/relastro_objects.$(ARCH).o    \
$(SRC)/save_catalogs.$(ARCH).o       \
$(SRC)/CoordOps.$(ARCH).o            \
$(SRC)/extra.$(ARCH).o            \
$(SRC)/StarMaps.$(ARCH).o    \
$(SRC)/high_speed_catalogs.$(ARCH).o  \
$(SRC)/high_speed_objects.$(ARCH).o  \
$(SRC)/high_speed_utils.$(ARCH).o  \
$(SRC)/hpm_catalogs.$(ARCH).o  \
$(SRC)/hpm_objects.$(ARCH).o  \
$(SRC)/relastro_merge_source.$(ARCH).o  \
$(SRC)/resort_catalog.$(ARCH).o  \
$(SRC)/BrightCatalog.$(ARCH).o  \
$(SRC)/assign_images.$(ARCH).o \
$(SRC)/launch_region_hosts.$(ARCH).o \
$(SRC)/relastro_parallel_images.$(ARCH).o \
$(SRC)/relastro_parallel_regions.$(ARCH).o \
$(SRC)/MeanPosIO.$(ARCH).o \
$(SRC)/MeasPosIO.$(ARCH).o \
$(SRC)/share_mean_pos.$(ARCH).o \
$(SRC)/share_meas_pos.$(ARCH).o \
$(SRC)/share_images_pos.$(ARCH).o \
$(SRC)/share_icrf_obj.$(ARCH).o \
$(SRC)/ICRF.$(ARCH).o \
$(SRC)/ICRFobjIO.$(ARCH).o \
$(SRC)/FrameCorrection.$(ARCH).o \
$(SRC)/FrameCorrectionIO.$(ARCH).o \
$(SRC)/FrameCorrectionUtils.$(ARCH).o \
$(SRC)/ImagePosIO.$(ARCH).o \
$(SRC)/ImageTable.$(ARCH).o \
$(SRC)/markObjects.$(ARCH).o \
$(SRC)/indexCatalogs.$(ARCH).o \
$(SRC)/syncfile.$(ARCH).o \
$(SRC)/client_logger.$(ARCH).o \
$(SRC)/relastroVisual.$(ARCH).o

#$(SRC)/write_coords.$(ARCH).o        
#$(SRC)/UpdateStacksWithFit.$(ARCH).o 

$(RELASTRO): $(INC)/relastro.h
$(BIN)/relastro.$(ARCH): $(RELASTRO)

RELASTRO_CLIENT = \
$(SRC)/CheckMeasureToImage.$(ARCH).o \
$(SRC)/RepairObjectIDs.$(ARCH).o \
$(SRC)/RepairStacks.$(ARCH).o \
$(SRC)/RepairStackMeasures.$(ARCH).o \
$(SRC)/StackImageMaps.$(ARCH).o \
$(SRC)/RepairWarps.$(ARCH).o \
$(SRC)/RepairWarpMeasures.$(ARCH).o \
$(SRC)/WarpImageMaps.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o \
$(SRC)/myIndex.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o	     \
$(SRC)/FitSimple.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o         \
$(SRC)/FitPosPMfixed.$(ARCH).o       \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/ImageOps.$(ARCH).o	     \
$(SRC)/MosaicOps.$(ARCH).o	     \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/SetSignals.$(ARCH).o 	     \
$(SRC)/Shutdown.$(ARCH).o 	     \
$(SRC)/UpdateObjects.$(ARCH).o       \
$(SRC)/UpdateObjectOffsets.$(ARCH).o \
$(SRC)/UpdateMeasures.$(ARCH).o      \
$(SRC)/GetAstromError.$(ARCH).o      \
$(SRC)/MeasFilterTest.$(ARCH).o            \
$(SRC)/args.$(ARCH).o		     \
$(SRC)/bcatalog.$(ARCH).o	     \
$(SRC)/dvo_astrom_ops.$(ARCH).o      \
$(SRC)/fitpoly.$(ARCH).o             \
$(SRC)/initialize.$(ARCH).o	     \
$(SRC)/liststats.$(ARCH).o	     \
$(SRC)/load_catalogs.$(ARCH).o	     \
$(SRC)/load_images.$(ARCH).o	     \
$(SRC)/mkpolyterm.$(ARCH).o          \
$(SRC)/select_images.$(ARCH).o	     \
$(SRC)/relastro_client.$(ARCH).o	\
$(SRC)/relastro_objects.$(ARCH).o    \
$(SRC)/save_catalogs.$(ARCH).o       \
$(SRC)/extra.$(ARCH).o            \
$(SRC)/high_speed_catalogs.$(ARCH).o  \
$(SRC)/high_speed_objects.$(ARCH).o  \
$(SRC)/high_speed_utils.$(ARCH).o  \
$(SRC)/hpm_catalogs.$(ARCH).o  \
$(SRC)/hpm_objects.$(ARCH).o  \
$(SRC)/relastro_merge_source.$(ARCH).o  \
$(SRC)/resort_catalog.$(ARCH).o  \
$(SRC)/plot_scatter.$(ARCH).o	     \
$(SRC)/plotstuff.$(ARCH).o	     \
$(SRC)/ICRF.$(ARCH).o \
$(SRC)/relastroVisual.$(ARCH).o \
$(SRC)/syncfile.$(ARCH).o \
$(SRC)/client_logger.$(ARCH).o \
$(SRC)/BrightCatalog.$(ARCH).o 

$(RELASTRO_CLIENT): $(INC)/relastro.h
$(BIN)/relastro_client.$(ARCH): $(RELASTRO_CLIENT)

testparallax: $(BIN)/testparallax.$(ARCH)

TESTPAR = \
$(SRC)/FitPMandPar.$(ARCH).o         \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/fitpoly.$(ARCH).o             \
$(SRC)/mkpolyterm.$(ARCH).o            \
$(SRC)/testparallax.$(ARCH).o

$(TESTPAR): $(INC)/relastro.h
$(BIN)/testparallax.$(ARCH): $(TESTPAR)

fitexample: $(BIN)/fitexample.$(ARCH)

FITEXAMPLE = \
$(SRC)/fitexample.$(ARCH).o 	      \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPosPMfixed.$(ARCH).o               \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o               \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/mkpolyterm.$(ARCH).o            \
$(SRC)/fitpoly.$(ARCH).o

$(FITEXAMPLE): $(INC)/relastro.h
$(BIN)/fitexample.$(ARCH): $(FITEXAMPLE)

fitobj: $(BIN)/fitobj.$(ARCH)

FITOBJ = \
$(SRC)/fitobj.$(ARCH).o 	      \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPosPMfixed.$(ARCH).o               \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o               \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/UpdateObjects.$(ARCH).o               \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/mkpolyterm.$(ARCH).o            \
$(SRC)/GetAstromError.$(ARCH).o            \
$(SRC)/MeasFilterTest.$(ARCH).o            \
$(SRC)/extra.$(ARCH).o            \
$(SRC)/plotstuff.$(ARCH).o            \
$(SRC)/fitpoly.$(ARCH).o

$(FITOBJ): $(INC)/relastro.h
$(BIN)/fitobj.$(ARCH): $(FITOBJ)

fitobj2: $(BIN)/fitobj2.$(ARCH)

FITOBJ2 = \
$(SRC)/fitobj2.$(ARCH).o 	      \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPosPMfixed.$(ARCH).o               \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o               \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/UpdateObjects.$(ARCH).o               \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/mkpolyterm.$(ARCH).o            \
$(SRC)/GetAstromError.$(ARCH).o            \
$(SRC)/MeasFilterTest.$(ARCH).o            \
$(SRC)/extra.$(ARCH).o            \
$(SRC)/plotstuff.$(ARCH).o            \
$(SRC)/fitpoly.$(ARCH).o

$(FITOBJ2): $(INC)/relastro.h
$(BIN)/fitobj2.$(ARCH): $(FITOBJ2)

fitobj3: $(BIN)/fitobj3.$(ARCH)

FITOBJ3 = \
$(SRC)/fitobj3.$(ARCH).o 	      \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPosPMfixed.$(ARCH).o               \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o               \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/UpdateObjects.$(ARCH).o               \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/mkpolyterm.$(ARCH).o            \
$(SRC)/GetAstromError.$(ARCH).o            \
$(SRC)/MeasFilterTest.$(ARCH).o            \
$(SRC)/extra.$(ARCH).o            \
$(SRC)/plotstuff.$(ARCH).o            \
$(SRC)/fitpoly.$(ARCH).o

$(FITOBJ3): $(INC)/relastro.h
$(BIN)/fitobj3.$(ARCH): $(FITOBJ3)

fitstk: $(BIN)/fitstk.$(ARCH)

FITSTK = \
$(SRC)/fitstk.$(ARCH).o 	      \
$(SRC)/ParFactor.$(ARCH).o           \
$(SRC)/FitAstromOps.$(ARCH).o        \
$(SRC)/FitPosPMfixed.$(ARCH).o               \
$(SRC)/FitPM.$(ARCH).o               \
$(SRC)/FitPMandPar.$(ARCH).o               \
$(SRC)/FitPosStack.$(ARCH).o       \
$(SRC)/UpdateObjects.$(ARCH).o               \
$(SRC)/BootstrapOps.$(ARCH).o        \
$(SRC)/mkpolyterm.$(ARCH).o            \
$(SRC)/GetAstromError.$(ARCH).o            \
$(SRC)/MeasFilterTest.$(ARCH).o            \
$(SRC)/extra.$(ARCH).o            \
$(SRC)/plotstuff.$(ARCH).o            \
$(SRC)/fitpoly.$(ARCH).o

# stupid dependencies: 
# UpdateObjects.c:UpdateObjects_Chips needs
# ImageOps.c:MeasFilterTiny forces
# load_images.c:free_astrom_table needs
# select_images.c
# MosaicOps.c

$(FITSTK): $(INC)/relastro.h
$(BIN)/fitstk.$(ARCH): $(FITSTK)

# $(SRC)/FitPM_IRLS.$(ARCH).o          \
# $(SRC)/FitPMandPar_IRLS.$(ARCH).o               \
# $(SRC)/FitPM_IRLS.$(ARCH).o          \
# $(SRC)/FitPMandPar_IRLS.$(ARCH).o               \
# $(SRC)/FitPosPMfixed_IRLS.$(ARCH).o               \
# $(SRC)/FitPM_IRLS.$(ARCH).o               \
# $(SRC)/FitPMandPar_IRLS.$(ARCH).o               \
