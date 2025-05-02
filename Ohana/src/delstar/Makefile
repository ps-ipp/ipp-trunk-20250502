default: delstar delstar_client
help:
	@echo "make options: addstar (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/delstar
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
INC	= 	$(HOME)/include
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

delstar: $(BIN)/delstar.$(ARCH)
delstar_client: $(BIN)/delstar_client.$(ARCH)

install: $(DESTBIN)/delstar $(DESTBIN)/delstar_client

DELSTAR = \
$(SRC)/delstar.$(ARCH).o          \
$(SRC)/SetSignals.$(ARCH).o       \
$(SRC)/ConfigInit.$(ARCH).o 	  \
$(SRC)/args.$(ARCH).o	          \
$(SRC)/delete_imagename.$(ARCH).o \
$(SRC)/delete_imagefile.$(ARCH).o \
$(SRC)/delete_duplicate_images.$(ARCH).o \
$(SRC)/delete_duplicate_measures.$(ARCH).o \
$(SRC)/delete_measures_by_match.$(ARCH).o \
$(SRC)/delete_measures_by_detID.$(ARCH).o \
$(SRC)/delete_fix_LAP.$(ARCH).o \
$(SRC)/delete_fix_LAP_edges.$(ARCH).o \
$(SRC)/delete_fix_LAP_edges_delete.$(ARCH).o \
$(SRC)/delete_image_photcodes.$(ARCH).o \
$(SRC)/delete_photcodes.$(ARCH).o \
$(SRC)/delete_photcodes_single.$(ARCH).o \
$(SRC)/delete_times.$(ARCH).o 	  \
$(SRC)/gimages.$(ARCH).o   	  \
$(SRC)/find_image_db.$(ARCH).o    \
$(SRC)/find_matches.$(ARCH).o 	  \
$(SRC)/parse_time.$(ARCH).o       \
$(SRC)/ImageIDsOps.$(ARCH).o       \
$(SRC)/ImageOpsFixLAP.$(ARCH).o       \
$(SRC)/ImageSubsetFixLAP.$(ARCH).o       \
$(SRC)/MeasureEdgeOps.$(ARCH).o       \
$(SRC)/check_permissions.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o          

DELSTAR_CLIENT = \
$(SRC)/delstar_client.$(ARCH).o   \
$(SRC)/SetSignals.$(ARCH).o       \
$(SRC)/ConfigInit.$(ARCH).o 	  \
$(SRC)/args.$(ARCH).o	          \
$(SRC)/delete_duplicate_images.$(ARCH).o \
$(SRC)/delete_duplicate_measures.$(ARCH).o \
$(SRC)/delete_measures_by_match.$(ARCH).o \
$(SRC)/delete_measures_by_detID.$(ARCH).o \
$(SRC)/delete_fix_LAP.$(ARCH).o \
$(SRC)/delete_fix_LAP_edges.$(ARCH).o \
$(SRC)/delete_fix_LAP_edges_delete.$(ARCH).o \
$(SRC)/delete_photcodes.$(ARCH).o \
$(SRC)/delete_photcodes_single.$(ARCH).o \
$(SRC)/ImageIDsOps.$(ARCH).o       \
$(SRC)/ImageOpsFixLAP.$(ARCH).o       \
$(SRC)/ImageSubsetFixLAP.$(ARCH).o       \
$(SRC)/MeasureEdgeOps.$(ARCH).o       \
$(SRC)/Shutdown.$(ARCH).o          

OLD = \
$(SRC)/gregion_image.$(ARCH).o    \
$(SRC)/gregion_patch.$(ARCH).o    \
$(SRC)/RegionOps.$(ARCH).o   	  

$(DELSTAR): $(INC)/delstar.h
$(BIN)/delstar.$(ARCH) : $(DELSTAR)

$(DELSTAR_CLIENT): $(INC)/delstar.h
$(BIN)/delstar_client.$(ARCH) : $(DELSTAR_CLIENT)
