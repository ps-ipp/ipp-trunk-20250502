default: all
help:
	@echo "make options: dvomerge dvomerge_client dvoconvert (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/dvomerge
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

dvomerge        : $(BIN)/dvomerge.$(ARCH)
dvomerge_client : $(BIN)/dvomerge_client.$(ARCH)
dvoconvert      : $(BIN)/dvoconvert.$(ARCH)
dvoverify       : $(BIN)/dvoverify.$(ARCH)
dvoverify_client : $(BIN)/dvoverify_client.$(ARCH)

dvoutils        : $(BIN)/dvoutils.$(ARCH)

all: dvomerge dvomerge_client dvoconvert dvosecfilt dvosecfilt_client dvorepair dvorepair_client dvoverify dvoverify_client dvoutils

DVOMERGE = \
$(SRC)/dvomerge.$(ARCH).o \
$(SRC)/dvomergeUpdate.$(ARCH).o \
$(SRC)/dvomergeCreate.$(ARCH).o \
$(SRC)/dvomergeUpdate_catalogs.$(ARCH).o \
$(SRC)/dvomergeUpdate_threaded.$(ARCH).o \
$(SRC)/dvomergeHistory.$(ARCH).o \
$(SRC)/dvomergeImageIDs.$(ARCH).o \
$(SRC)/dvo_image_merge_dbs.$(ARCH).o \
$(SRC)/IDmapIO.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/help.$(ARCH).o \
$(SRC)/args.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o \
$(SRC)/cptlist.$(ARCH).o \
$(SRC)/SkyTablePopulatedRange.$(ARCH).o \
$(SRC)/LoadCatalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/replace_tycho.$(ARCH).o \
$(SRC)/repair_catalog_by_objID.$(ARCH).o \
$(SRC)/merge_catalogs_new.$(ARCH).o \
$(SRC)/merge_catalogs_old.$(ARCH).o

$(DVOMERGE)  : $(INC)/dvomerge.h
$(BIN)/dvomerge.$(ARCH) : $(DVOMERGE)

DVOMERGE_CLIENT = \
$(SRC)/dvomerge_client.$(ARCH).o \
$(SRC)/dvomergeUpdate_catalogs.$(ARCH).o \
$(SRC)/dvomergeHistory.$(ARCH).o \
$(SRC)/dvomergeImageIDs.$(ARCH).o \
$(SRC)/dvo_image_merge_dbs.$(ARCH).o \
$(SRC)/IDmapIO.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/help.$(ARCH).o \
$(SRC)/args.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o \
$(SRC)/cptlist.$(ARCH).o \
$(SRC)/SkyTablePopulatedRange.$(ARCH).o \
$(SRC)/LoadCatalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/replace_tycho.$(ARCH).o \
$(SRC)/repair_catalog_by_objID.$(ARCH).o \
$(SRC)/merge_catalogs_new.$(ARCH).o \
$(SRC)/merge_catalogs_old.$(ARCH).o

$(DVOMERGE_CLIENT) : $(INC)/dvomerge.h
$(BIN)/dvomerge_client.$(ARCH) : $(DVOMERGE_CLIENT)

DVOCONVERT = \
$(SRC)/dvoconvert.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/help.$(ARCH).o \
$(SRC)/args.$(ARCH).o \
$(SRC)/cptlist.$(ARCH).o \
$(SRC)/SkyTablePopulatedRange.$(ARCH).o \
$(SRC)/LoadCatalog.$(ARCH).o \
$(SRC)/merge_catalogs_new.$(ARCH).o

$(DVOCONVERT)  : $(INC)/dvomerge.h
$(BIN)/dvoconvert.$(ARCH) : $(DVOCONVERT)

DVOSECFILT = \
$(SRC)/dvosecfilt.$(ARCH).o \
$(SRC)/dvosecfilt_catalogs.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/help.$(ARCH).o \
$(SRC)/cptlist.$(ARCH).o \
$(SRC)/args.$(ARCH).o

$(DVOSECFILT)  : $(INC)/dvomerge.h
$(BIN)/dvosecfilt.$(ARCH) : $(DVOSECFILT)

DVOSECFILT_CLIENT = \
$(SRC)/dvosecfilt_client.$(ARCH).o \
$(SRC)/dvosecfilt_catalogs.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/help.$(ARCH).o \
$(SRC)/cptlist.$(ARCH).o \
$(SRC)/args.$(ARCH).o

$(DVOSECFILT_CLIENT)  : $(INC)/dvomerge.h
$(BIN)/dvosecfilt_client.$(ARCH) : $(DVOSECFILT_CLIENT)

DVOREPAIR = \
$(SRC)/dvorepair.$(ARCH).o \
$(SRC)/dvorepair_args.$(ARCH).o \
$(SRC)/dvorepairFixCPT.$(ARCH).o \
$(SRC)/dvorepairFixWarpIDs.$(ARCH).o \
$(SRC)/dvorepairFixStackIDs.$(ARCH).o \
$(SRC)/dvorepairImagesVsMeasures.$(ARCH).o \
$(SRC)/dvorepairDeleteImageList.$(ARCH).o \
$(SRC)/dvorepairDeleteImagesByExternID.$(ARCH).o \
$(SRC)/dvorepairDeleteImagesByExternID_v2.$(ARCH).o \
$(SRC)/dvorepairFixImages.$(ARCH).o \
$(SRC)/dvorepair_by_objID.$(ARCH).o \
$(SRC)/repair_catalog_by_objID.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o \
$(SRC)/LoadImages.$(ARCH).o \
$(SRC)/DeleteImagesOps.$(ARCH).o \
$(SRC)/ReadDeleteList.$(ARCH).o \
$(SRC)/ReadDeleteListExternID.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/myIndex.$(ARCH).o \
$(SRC)/match_image.$(ARCH).o \
$(SRC)/help.$(ARCH).o \
$(SRC)/ImageOps.$(ARCH).o

$(DVOREPAIR)  : $(INC)/dvorepair.h
$(BIN)/dvorepair.$(ARCH) : $(DVOREPAIR)

DVOREPAIR_CLIENT = \
$(SRC)/dvorepair_client.$(ARCH).o \
$(SRC)/dvorepair_args.$(ARCH).o \
$(SRC)/dvorepairDeleteImagesByExternID.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o \
$(SRC)/LoadImages.$(ARCH).o \
$(SRC)/DeleteImagesOps.$(ARCH).o \
$(SRC)/ReadDeleteListExternID.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/myIndex.$(ARCH).o \
$(SRC)/help.$(ARCH).o

$(DVOREPAIR_CLIENT)  : $(INC)/dvorepair.h
$(BIN)/dvorepair_client.$(ARCH) : $(DVOREPAIR_CLIENT)

DVOVERIFY = \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/dvoverify.$(ARCH).o \
$(SRC)/dvoverify_args.$(ARCH).o \
$(SRC)/dvoverify_catalogs.$(ARCH).o \
$(SRC)/dvoverify_utils.$(ARCH).o

$(DVOVERIFY)  : $(INC)/dvoverify.h
$(BIN)/dvoverify.$(ARCH) : $(DVOVERIFY)

DVOVERIFY_CLIENT = \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/dvoverify_client.$(ARCH).o \
$(SRC)/dvoverify_args.$(ARCH).o \
$(SRC)/dvoverify_catalogs.$(ARCH).o \
$(SRC)/dvoverify_utils.$(ARCH).o

$(DVOVERIFY_CLIENT) : $(INC)/dvoverify.h
$(BIN)/dvoverify_client.$(ARCH) : $(DVOVERIFY_CLIENT)

DVOUTILS = \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/LoadImages.$(ARCH).o \
$(SRC)/dvoutils.$(ARCH).o \
$(SRC)/dvoutils_args.$(ARCH).o \
$(SRC)/dvoutils_uniq_images.$(ARCH).o \
$(SRC)/dvoutils_check_images.$(ARCH).o \
$(SRC)/dvoutils_load_image_index.$(ARCH).o

$(DVOUTILS)  : $(INC)/dvoutils.h
$(BIN)/dvoutils.$(ARCH) : $(DVOUTILS)

INSTALL = dvomerge dvomerge_client dvoconvert dvosecfilt dvosecfilt_client dvorepair dvorepair_client dvoverify dvoverify_client dvoverify dvoutils

# dependancy rules for binary code #########################
$(INSTALL): % : $(BIN)/%.$(ARCH)

%.clean :
	rm -f $(BIN)/$*.$(ARCH)

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(INSTALL); do $(MAKE) $$i.install || exit; done
