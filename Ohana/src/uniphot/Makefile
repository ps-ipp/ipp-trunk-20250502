default: uniphot setphot setphot_client setfwhm setposangle setposangle_client setastrom setastrom_client setgalmodel setgalmodel_client fixhsc fixhsc_client fiximids fiximids_client fixstkids fixstkids_client ckids ckids_client

help:
@echo "make options: uniphot setphot setfwhm default help install default (uniphot setphot setfwhm)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/uniphot
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
setfwhm: $(BIN)/setfwhm.$(ARCH)
setphot: $(BIN)/setphot.$(ARCH)
setphot_client: $(BIN)/setphot_client.$(ARCH)
setposangle: $(BIN)/setposangle.$(ARCH)
setposangle_client: $(BIN)/setposangle_client.$(ARCH)
setastrom: $(BIN)/setastrom.$(ARCH)
setastrom_client: $(BIN)/setastrom_client.$(ARCH)
setgalmodel: $(BIN)/setgalmodel.$(ARCH)
setgalmodel_client: $(BIN)/setgalmodel_client.$(ARCH)
fixhsc: $(BIN)/fixhsc.$(ARCH)
fixhsc_client: $(BIN)/fixhsc_client.$(ARCH)
fiximids: $(BIN)/fiximids.$(ARCH)
fiximids_client: $(BIN)/fiximids_client.$(ARCH)
fixstkids: $(BIN)/fixstkids.$(ARCH)
fixstkids_client: $(BIN)/fixstkids_client.$(ARCH)
ckids: $(BIN)/ckids.$(ARCH)
ckids_client: $(BIN)/ckids_client.$(ARCH)

install: $(DESTBIN)/uniphot $(DESTBIN)/setfwhm $(DESTBIN)/setphot $(DESTBIN)/setphot_client $(DESTBIN)/setposangle $(DESTBIN)/setposangle_client $(DESTBIN)/setastrom $(DESTBIN)/setastrom_client $(DESTBIN)/setgalmodel $(DESTBIN)/setgalmodel_client $(DESTBIN)/fixhsc $(DESTBIN)/fixhsc_client $(DESTBIN)/fiximids $(DESTBIN)/fiximids_client $(DESTBIN)/fixstkids $(DESTBIN)/fixstkids_client $(DESTBIN)/ckids $(DESTBIN)/ckids_client

UNIPHOT = \
$(SRC)/uniphot.$(ARCH).o	    \
$(SRC)/initialize.$(ARCH).o	    \
$(SRC)/ConfigInit.$(ARCH).o	    \
$(SRC)/args.$(ARCH).o               \
$(SRC)/liststats.$(ARCH).o	    \
$(SRC)/load_images.$(ARCH).o	    \
$(SRC)/subset_images.$(ARCH).o	    \
$(SRC)/find_image_tgroups.$(ARCH).o \
$(SRC)/find_image_sgroups.$(ARCH).o \
$(SRC)/fit_groups.$(ARCH).o	    \
$(SRC)/update_dvo_uniphot.$(ARCH).o \
$(SRC)/update_catalog_uniphot.$(ARCH).o \
$(SRC)/convert.$(ARCH).o	    \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/sort.$(ARCH).o

$(UNIPHOT): $(INC)/uniphot.h
$(BIN)/uniphot.$(ARCH): $(UNIPHOT)

SETPHOT = \
$(SRC)/setphot.$(ARCH).o	    \
$(SRC)/initialize_setphot.$(ARCH).o \
$(SRC)/load_zpt_table.$(ARCH).o	    \
$(SRC)/load_images_setphot.$(ARCH).o \
$(SRC)/ImageSubset.$(ARCH).o	    \
$(SRC)/match_zpts_to_images.$(ARCH).o	    \
$(SRC)/match_flatcorr_to_images.$(ARCH).o	    \
$(SRC)/match_camcorr_to_images.$(ARCH).o	    \
$(SRC)/cam_zpt_correction.$(ARCH).o	    \
$(SRC)/cam_correction.$(ARCH).o \
$(SRC)/dcr_correction.$(ARCH).o \
$(SRC)/kh_correction.$(ARCH).o \
$(SRC)/astrom_correction.$(ARCH).o \
$(SRC)/update_dvo_setphot.$(ARCH).o \
$(SRC)/update_catalog_setphot.$(ARCH).o \
$(SRC)/update_catalog_setastrom.$(ARCH).o \
$(SRC)/update_catalog_setgalmodel.$(ARCH).o \
$(SRC)/repair_catalog_by_objID.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    

$(SETPHOT): $(INC)/setphot.h
$(BIN)/setphot.$(ARCH): $(SETPHOT)

SETPHOT_CLIENT = \
$(SRC)/setphot_client.$(ARCH).o	    \
$(SRC)/update_dvo_setphot.$(ARCH).o \
$(SRC)/update_catalog_setphot.$(ARCH).o \
$(SRC)/update_catalog_setastrom.$(ARCH).o \
$(SRC)/update_catalog_setgalmodel.$(ARCH).o \
$(SRC)/repair_catalog_by_objID.$(ARCH).o \
$(SRC)/initialize_setphot_client.$(ARCH).o \
$(SRC)/cam_zpt_correction.$(ARCH).o	    \
$(SRC)/cam_correction.$(ARCH).o \
$(SRC)/dcr_correction.$(ARCH).o \
$(SRC)/kh_correction.$(ARCH).o \
$(SRC)/astrom_correction.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/ImageSubset.$(ARCH).o

$(SETPHOT_CLIENT): $(INC)/setphot.h
$(BIN)/setphot_client.$(ARCH): $(SETPHOT_CLIENT)

SETPOSANGLE = \
$(SRC)/setposangle.$(ARCH).o	    \
$(SRC)/initialize_setposangle.$(ARCH).o \
$(SRC)/load_images_setposangle.$(ARCH).o \
$(SRC)/ImageSubsetSetPosangle.$(ARCH).o	    \
$(SRC)/update_dvo_setposangle.$(ARCH).o \
$(SRC)/update_catalog_setposangle.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    

$(SETPOSANGLE): $(INC)/setposangle.h
$(BIN)/setposangle.$(ARCH): $(SETPOSANGLE)

SETPOSANGLE_CLIENT = \
$(SRC)/setposangle_client.$(ARCH).o	    \
$(SRC)/update_dvo_setposangle.$(ARCH).o \
$(SRC)/update_catalog_setposangle.$(ARCH).o \
$(SRC)/initialize_setposangle.$(ARCH).o \
$(SRC)/load_images_setposangle.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/ImageSubsetSetPosangle.$(ARCH).o

$(SETPOSANGLE_CLIENT): $(INC)/setposangle.h
$(BIN)/setposangle_client.$(ARCH): $(SETPOSANGLE_CLIENT)

SETASTROM = \
$(SRC)/setastrom.$(ARCH).o	    \
$(SRC)/initialize_setastrom.$(ARCH).o \
$(SRC)/tyc_correction.$(ARCH).o \
$(SRC)/cam_correction.$(ARCH).o \
$(SRC)/dcr_correction.$(ARCH).o \
$(SRC)/kh_correction.$(ARCH).o \
$(SRC)/astrom_correction.$(ARCH).o \
$(SRC)/repair_tycho_setastrom.$(ARCH).o \
$(SRC)/update_dvo_setastrom.$(ARCH).o \
$(SRC)/update_catalog_setastrom.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    

$(SETASTROM): $(INC)/setastrom.h
$(BIN)/setastrom.$(ARCH): $(SETASTROM)

SETASTROM_CLIENT = \
$(SRC)/setastrom_client.$(ARCH).o	    \
$(SRC)/update_dvo_setastrom.$(ARCH).o \
$(SRC)/update_catalog_setastrom.$(ARCH).o \
$(SRC)/tyc_correction.$(ARCH).o \
$(SRC)/cam_correction.$(ARCH).o \
$(SRC)/dcr_correction.$(ARCH).o \
$(SRC)/kh_correction.$(ARCH).o \
$(SRC)/repair_tycho_setastrom.$(ARCH).o \
$(SRC)/astrom_correction.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/initialize_setastrom.$(ARCH).o

$(SETASTROM_CLIENT): $(INC)/setastrom.h
$(BIN)/setastrom_client.$(ARCH): $(SETASTROM_CLIENT)

SETGALMODEL = \
$(SRC)/setgalmodel.$(ARCH).o	    \
$(SRC)/initialize_setgalmodel.$(ARCH).o \
$(SRC)/galmodel_correction.$(ARCH).o \
$(SRC)/update_dvo_setgalmodel.$(ARCH).o \
$(SRC)/update_catalog_setgalmodel.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    

$(SETGALMODEL): $(INC)/setgalmodel.h
$(BIN)/setgalmodel.$(ARCH): $(SETGALMODEL)

SETGALMODEL_CLIENT = \
$(SRC)/setgalmodel_client.$(ARCH).o	    \
$(SRC)/update_dvo_setgalmodel.$(ARCH).o \
$(SRC)/update_catalog_setgalmodel.$(ARCH).o \
$(SRC)/galmodel_correction.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/initialize_setgalmodel.$(ARCH).o

$(SETGALMODEL_CLIENT): $(INC)/setgalmodel.h
$(BIN)/setgalmodel_client.$(ARCH): $(SETGALMODEL_CLIENT)

SETFWHM =                           \
$(SRC)/setfwhm.$(ARCH).o	    \
$(SRC)/initialize.$(ARCH).o	    \
$(SRC)/ConfigInit.$(ARCH).o	    \
$(SRC)/args.$(ARCH).o               \
$(SRC)/liststats.$(ARCH).o	    \
$(SRC)/load_fwhm_table.$(ARCH).o    \
$(SRC)/load_images.$(ARCH).o	    \
$(SRC)/match_fwhm_to_images.$(ARCH).o	    \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    

$(SETFWHM): $(INC)/uniphot.h
$(BIN)/setfwhm.$(ARCH): $(SETFWHM)

FIXHSC = \
$(SRC)/fixhsc.$(ARCH).o	         \
$(SRC)/initialize_fixhsc.$(ARCH).o     \
$(SRC)/load_images_fixhsc.$(ARCH).o    \
$(SRC)/load_rules_fixhsc.$(ARCH).o    \
$(SRC)/update_dvo_fixhsc.$(ARCH).o     \
$(SRC)/update_catalog_fixhsc.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/Shutdown.$(ARCH).o	    

$(FIXHSC): $(INC)/fixhsc.h
$(BIN)/fixhsc.$(ARCH): $(FIXHSC)

FIXHSC_CLIENT = \
$(SRC)/fixhsc_client.$(ARCH).o	 \
$(SRC)/initialize_fixhsc.$(ARCH).o     \
$(SRC)/load_images_fixhsc.$(ARCH).o    \
$(SRC)/load_rules_fixhsc.$(ARCH).o    \
$(SRC)/update_dvo_fixhsc.$(ARCH).o     \
$(SRC)/update_catalog_fixhsc.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/Shutdown.$(ARCH).o

$(FIXHSC_CLIENT): $(INC)/fixhsc.h
$(BIN)/fixhsc_client.$(ARCH): $(FIXHSC_CLIENT)

FIXIMIDS = \
$(SRC)/fiximids.$(ARCH).o	         \
$(SRC)/initialize_fiximids.$(ARCH).o     \
$(SRC)/load_images_fiximids.$(ARCH).o    \
$(SRC)/ImageSubsetFixImageIDs.$(ARCH).o  \
$(SRC)/update_dvo_fiximids.$(ARCH).o     \
$(SRC)/update_catalog_fiximids.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/Shutdown.$(ARCH).o	    

$(FIXIMIDS): $(INC)/fiximids.h
$(BIN)/fiximids.$(ARCH): $(FIXIMIDS)

FIXIMIDS_CLIENT = \
$(SRC)/fiximids_client.$(ARCH).o	 \
$(SRC)/initialize_fiximids.$(ARCH).o     \
$(SRC)/load_images_fiximids.$(ARCH).o    \
$(SRC)/ImageSubsetFixImageIDs.$(ARCH).o  \
$(SRC)/update_dvo_fiximids.$(ARCH).o     \
$(SRC)/update_catalog_fiximids.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/Shutdown.$(ARCH).o

$(FIXIMIDS_CLIENT): $(INC)/fiximids.h
$(BIN)/fiximids_client.$(ARCH): $(FIXIMIDS_CLIENT)

FIXSTKIDS = \
$(SRC)/fixstkids.$(ARCH).o	         \
$(SRC)/initialize_fixstkids.$(ARCH).o     \
$(SRC)/load_images_fixstkids.$(ARCH).o    \
$(SRC)/ImageSubsetFixStackIDs.$(ARCH).o  \
$(SRC)/update_dvo_fixstkids.$(ARCH).o     \
$(SRC)/update_catalog_fixstkids.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/Shutdown.$(ARCH).o	    

$(FIXSTKIDS): $(INC)/fixstkids.h
$(BIN)/fixstkids.$(ARCH): $(FIXSTKIDS)

FIXSTKIDS_CLIENT = \
$(SRC)/fixstkids_client.$(ARCH).o	 \
$(SRC)/initialize_fixstkids.$(ARCH).o     \
$(SRC)/load_images_fixstkids.$(ARCH).o    \
$(SRC)/ImageSubsetFixStackIDs.$(ARCH).o  \
$(SRC)/update_dvo_fixstkids.$(ARCH).o     \
$(SRC)/update_catalog_fixstkids.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o	         \
$(SRC)/Shutdown.$(ARCH).o

$(FIXSTKIDS_CLIENT): $(INC)/fixstkids.h
$(BIN)/fixstkids_client.$(ARCH): $(FIXSTKIDS_CLIENT)

CKIDS = \
$(SRC)/ckids.$(ARCH).o	         \
$(SRC)/initialize_ckids.$(ARCH).o     \
$(SRC)/update_dvo_ckids.$(ARCH).o     \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/update_catalog_ckids.$(ARCH).o

$(CKIDS): $(INC)/ckids.h
$(BIN)/ckids.$(ARCH): $(CKIDS)

CKIDS_CLIENT = \
$(SRC)/ckids_client.$(ARCH).o	 \
$(SRC)/initialize_ckids.$(ARCH).o     \
$(SRC)/update_dvo_ckids.$(ARCH).o     \
$(SRC)/SetSignals.$(ARCH).o	    \
$(SRC)/Shutdown.$(ARCH).o	    \
$(SRC)/update_catalog_ckids.$(ARCH).o

$(CKIDS_CLIENT): $(INC)/ckids.h
$(BIN)/ckids_client.$(ARCH): $(CKIDS_CLIENT)

