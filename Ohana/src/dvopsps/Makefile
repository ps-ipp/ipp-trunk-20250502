default: dvopsps dvopsps_client
help:
@echo "make options: dvopsps default help install (dvopsps)"

include ../../Makefile.System
HOME    =       $(ROOT)/src/dvopsps
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

dvopsps: $(BIN)/dvopsps.$(ARCH)
dvopsps_client: $(BIN)/dvopsps_client.$(ARCH)

install: $(DESTBIN)/dvopsps $(DESTBIN)/dvopsps_client

DVOPSPS = \
$(SRC)/dvopsps.$(ARCH).o	    \
$(SRC)/initialize_dvopsps.$(ARCH).o \
$(SRC)/mysql_dvopsps.$(ARCH).o \
$(SRC)/DetectionOps.$(ARCH).o \
$(SRC)/insert_skytable.$(ARCH).o \
$(SRC)/insert_objects_dvopsps.$(ARCH).o \
$(SRC)/insert_objects_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_FWobjects_dvopsps.$(ARCH).o \
$(SRC)/insert_FWobjects_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_FGshape_dvopsps.$(ARCH).o \
$(SRC)/insert_FGshape_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_diffobj_dvopsps.$(ARCH).o \
$(SRC)/insert_diffobj_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_detections_dvopsps.$(ARCH).o \
$(SRC)/insert_detections_dvopsps_catalog.$(ARCH).o

# $(SRC)/insert_detections_dvopsps_catalog.$(ARCH).o

$(DVOPSPS): $(INC)/dvopsps.h
$(BIN)/dvopsps.$(ARCH): $(DVOPSPS)

DVOPSPS_CLIENT = \
$(SRC)/dvopsps_client.$(ARCH).o	    \
$(SRC)/initialize_dvopsps.$(ARCH).o \
$(SRC)/mysql_dvopsps.$(ARCH).o \
$(SRC)/DetectionOps.$(ARCH).o \
$(SRC)/insert_skytable.$(ARCH).o \
$(SRC)/insert_objects_dvopsps.$(ARCH).o \
$(SRC)/insert_objects_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_FWobjects_dvopsps.$(ARCH).o \
$(SRC)/insert_FWobjects_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_FGshape_dvopsps.$(ARCH).o \
$(SRC)/insert_FGshape_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_diffobj_dvopsps.$(ARCH).o \
$(SRC)/insert_diffobj_dvopsps_catalog.$(ARCH).o \
$(SRC)/insert_detections_dvopsps.$(ARCH).o \
$(SRC)/insert_detections_dvopsps_catalog.$(ARCH).o

$(DVOPSPS_CLIENT): $(INC)/dvopsps.h
$(BIN)/dvopsps_client.$(ARCH): $(DVOPSPS_CLIENT)
