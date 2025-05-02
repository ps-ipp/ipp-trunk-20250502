default: all
help:
	@echo "make options: addstar (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/addstar
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

addstar      : $(BIN)/addstar.$(ARCH)
addstard     : $(BIN)/addstard.$(ARCH)
addstart     : $(BIN)/addstart.$(ARCH)
addstarc     : $(BIN)/addstarc.$(ARCH)
mkacc-2mass  : $(BIN)/mkacc-2mass.$(ARCH)
sedstar      : $(BIN)/sedstar.$(ARCH)
load2mass    : $(BIN)/load2mass.$(ARCH)
loadatlas    : $(BIN)/loadatlas.$(ARCH)
loadgalphot  : $(BIN)/loadgalphot.$(ARCH)
loadgaia     : $(BIN)/loadgaia.$(ARCH)
loadgaia_dr2 : $(BIN)/loadgaia_dr2.$(ARCH)
loadgaia_edr3 : $(BIN)/loadgaia_edr3.$(ARCH)
loadukirt_uhs : $(BIN)/loadukirt_uhs.$(ARCH)
loadstarpar  : $(BIN)/loadstarpar.$(ARCH)
loadstarpar_client : $(BIN)/loadstarpar_client.$(ARCH)
loadICRF  : $(BIN)/loadICRF.$(ARCH)
loadICRF_client : $(BIN)/loadICRF_client.$(ARCH)
loadwise     : $(BIN)/loadwise.$(ARCH)
loadtycho    : $(BIN)/loadtycho.$(ARCH)
loadbsc      : $(BIN)/loadbsc.$(ARCH)
setobjflags  : $(BIN)/setobjflags.$(ARCH)
setobjflags_client : $(BIN)/setobjflags_client.$(ARCH)
dumpskycells : $(BIN)/dumpskycells.$(ARCH)
findskycell  : $(BIN)/findskycell.$(ARCH)
loadsupercos : $(BIN)/loadsupercos.$(ARCH)
gztest       : $(BIN)/gztest.$(ARCH)
mkcmf        : $(BIN)/mkcmf.$(ARCH)

# programs in 'SERVER' use the client-server concept and are out of date

INSTALL = addstar addstar_client sedstar loadgalphot loadgaia loadgaia_dr2 loadgaia_edr3 loadukirt_uhs loadstarpar loadstarpar_client setobjflags setobjflags_client loadICRF loadICRF_client skycells mkcmf dumpskycells findskycell load2mass loadwise loadtycho loadbsc loadsupercos loadatlas
SERVER  = addstarc addstard addstart

all: $(INSTALL)

# I need to fix the client/server version of addstar now that I have dropped Stars
# as an autocode type and have converted to carrying average and measure

ADDSTAR = \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyListForStars.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/addstar.$(ARCH).o \
$(SRC)/addstar_create_ID.$(ARCH).o \
$(SRC)/airmass.$(ARCH).o \
$(SRC)/args.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/calibrate.$(ARCH).o \
$(SRC)/edge_check.$(ARCH).o \
$(SRC)/fakeimage.$(ARCH).o \
$(SRC)/find_matches.$(ARCH).o \
$(SRC)/find_matches_closest.$(ARCH).o \
$(SRC)/GetZeroPointExposure.$(ARCH).o \
$(SRC)/LoadFilenames.$(ARCH).o \
$(SRC)/LoadStars.$(ARCH).o \
$(SRC)/LoadHeaders.$(ARCH).o \
$(SRC)/MatchHeaders.$(ARCH).o \
$(SRC)/LoadData.$(ARCH).o \
$(SRC)/LoadDataSDSS.$(ARCH).o \
$(SRC)/LoadDataUKIRT.$(ARCH).o \
$(SRC)/LoadDataPMM.$(ARCH).o \
$(SRC)/in_image.$(ARCH).o \
$(SRC)/load_subpix.$(ARCH).o \
$(SRC)/opening_angle.$(ARCH).o \
$(SRC)/parse_time.$(ARCH).o \
$(SRC)/resort_catalogs.$(ARCH).o \
$(SRC)/resort_threaded.$(ARCH).o \
$(SRC)/resort_unthreaded.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/ReadStarsFITS.$(ARCH).o \
$(SRC)/ReadStarsSDSS.$(ARCH).o \
$(SRC)/ReadStarsUKIRT.$(ARCH).o \
$(SRC)/ReadXradFITS.$(ARCH).o \
$(SRC)/find_matches.$(ARCH).o \
$(SRC)/find_matches_closest.$(ARCH).o \
$(SRC)/find_matches_refstars.$(ARCH).o \
$(SRC)/find_matches_closest_refstars.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/getusnob.$(ARCH).o \
$(SRC)/greference.$(ARCH).o \
$(SRC)/grefstars.$(ARCH).o \
$(SRC)/greffits.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/FilterStars.$(ARCH).o \
$(SRC)/ImageOptions.$(ARCH).o \
$(SRC)/GetFileMode.$(ARCH).o \
$(SRC)/ReadImageHeader.$(ARCH).o \
$(SRC)/ImageIndex.$(ARCH).o \
$(SRC)/UpdateImageIDs.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

# this is the client for parallel operations
ADDSTAR_CLIENT = \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyListForStars.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/addstar_client.$(ARCH).o \
$(SRC)/args_parallel_client.$(ARCH).o \
$(SRC)/resort_catalogs.$(ARCH).o \
$(SRC)/resort_threaded.$(ARCH).o \
$(SRC)/resort_unthreaded.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o

ADDSTARD = \
$(SRC)/CheckPassword.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/NewImage.$(ARCH).o \
$(SRC)/NewRefcat.$(ARCH).o \
$(SRC)/NewReflist.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyListForStars.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/SocketOps.$(ARCH).o \
$(SRC)/UpdateDatabase_Image.$(ARCH).o \
$(SRC)/UpdateDatabase_Refcat.$(ARCH).o \
$(SRC)/UpdateDatabase_Reflist.$(ARCH).o \
$(SRC)/addstard.$(ARCH).o \
$(SRC)/airmass.$(ARCH).o \
$(SRC)/args_server.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/calibrate.$(ARCH).o \
$(SRC)/edge_check.$(ARCH).o \
$(SRC)/find_matches.$(ARCH).o \
$(SRC)/find_matches_closest.$(ARCH).o \
$(SRC)/find_matches_refstars.$(ARCH).o \
$(SRC)/find_subset.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/getusnob.$(ARCH).o \
$(SRC)/ImageOptions.$(ARCH).o \
$(SRC)/greference.$(ARCH).o \
$(SRC)/in_image.$(ARCH).o \
$(SRC)/load_subpix.$(ARCH).o \
$(SRC)/opening_angle.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

ADDSTART = \
$(SRC)/addstart.$(ARCH).o \
$(SRC)/ImageOptions.$(ARCH).o \
$(SRC)/DatasetOps.$(ARCH).o \
$(SRC)/SocketOps.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/args_server.$(ARCH).o \
$(SRC)/CheckPassword.$(ARCH).o \
$(SRC)/UpdateDatabase_Image.$(ARCH).o \
$(SRC)/UpdateDatabase_Reflist.$(ARCH).o \
$(SRC)/UpdateDatabase_Refcat.$(ARCH).o \
$(SRC)/airmass.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/calibrate.$(ARCH).o \
$(SRC)/find_matches.$(ARCH).o \
$(SRC)/find_matches_closest.$(ARCH).o \
$(SRC)/find_matches_refstars.$(ARCH).o \
$(SRC)/find_subset.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/getusnob.$(ARCH).o \
$(SRC)/greference.$(ARCH).o \
$(SRC)/in_image.$(ARCH).o \
$(SRC)/load_subpix.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/SkyListForStars.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/edge_check.$(ARCH).o \
$(SRC)/opening_angle.$(ARCH).o \
$(SRC)/ListenClients_Thread.$(ARCH).o \
$(SRC)/NewImage_Thread.$(ARCH).o \
$(SRC)/NewReflist_Thread.$(ARCH).o \
$(SRC)/NewRefcat_Thread.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

# this is the client for server / client addstar operations
ADDSTARC = \
$(SRC)/addstarc.$(ARCH).o \
$(SRC)/airmass.$(ARCH).o \
$(SRC)/SocketOps.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/LoadStars.$(ARCH).o \
$(SRC)/LoadHeaders.$(ARCH).o \
$(SRC)/MatchHeaders.$(ARCH).o \
$(SRC)/LoadData.$(ARCH).o \
$(SRC)/LoadDataSDSS.$(ARCH).o \
$(SRC)/LoadDataUKIRT.$(ARCH).o \
$(SRC)/LoadDataPMM.$(ARCH).o \
$(SRC)/grefstars.$(ARCH).o \
$(SRC)/load_subpix.$(ARCH).o \
$(SRC)/parse_time.$(ARCH).o \
$(SRC)/ReadStarsFITS.$(ARCH).o \
$(SRC)/GetFileMode.$(ARCH).o \
$(SRC)/ReadImageHeader.$(ARCH).o \
$(SRC)/ReadStarsTEXT.$(ARCH).o \
$(SRC)/ReadStarsSDSS.$(ARCH).o \
$(SRC)/ReadStarsUKIRT.$(ARCH).o \
$(SRC)/ImageIndex.$(ARCH).o \
$(SRC)/UpdateImageIDs.$(ARCH).o \
$(SRC)/FilterStars.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/args_client.$(ARCH).o

### this program may need to be repaired (EAM 2006.08)
MKACC-2MASS = \
$(SRC)/mkacc-2mass.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/get2mass_as.$(ARCH).o

LOAD-2MASS = \
$(SRC)/load2mass.$(ARCH).o \
$(SRC)/load2mass_as_rawdata.$(ARCH).o \
$(SRC)/get2mass_ops.$(ARCH).o \
$(SRC)/get2mass_full.$(ARCH).o \
$(SRC)/find_matches_refstars.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/args_load2mass.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

# $(SRC)/load2mass_catalog.$(ARCH).o 

LOAD-GALPHOT = \
$(SRC)/loadgalphot.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_loadgalphot.$(ARCH).o \
$(SRC)/find_matches_galphot.$(ARCH).o \
$(SRC)/loadgalphot_catalog.$(ARCH).o \
$(SRC)/loadgalphot_make_subset.$(ARCH).o \
$(SRC)/loadgalphot_readstars.$(ARCH).o \
$(SRC)/loadgalphot_fit2d.$(ARCH).o \
$(SRC)/loadgalphot_join.$(ARCH).o \
$(SRC)/loadgalphot_table.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/strhash.$(ARCH).o \
$(SRC)/sortIDs.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-GAIA = \
$(SRC)/loadgaia.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_loadgaia.$(ARCH).o \
$(SRC)/find_matches_gaia.$(ARCH).o \
$(SRC)/loadgaia_catalog.$(ARCH).o \
$(SRC)/loadgaia_make_subset.$(ARCH).o \
$(SRC)/loadgaia_readstars.$(ARCH).o \
$(SRC)/loadgaia_table.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/strhash.$(ARCH).o \
$(SRC)/sortIDs.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-GAIA_DR2 = \
$(SRC)/loadgaia_dr2.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_loadgaia_dr2.$(ARCH).o \
$(SRC)/find_matches_gaia_dr2.$(ARCH).o \
$(SRC)/loadgaia_dr2_catalog.$(ARCH).o \
$(SRC)/loadgaia_dr2_make_subset.$(ARCH).o \
$(SRC)/loadgaia_dr2_readstars.$(ARCH).o \
$(SRC)/loadgaia_dr2_table.$(ARCH).o \
$(SRC)/parse_csv.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/strhash.$(ARCH).o \
$(SRC)/sortIDs.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-GAIA_EDR3 = \
$(SRC)/loadgaia_edr3.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_loadgaia_edr3.$(ARCH).o \
$(SRC)/find_matches_gaia_edr3.$(ARCH).o \
$(SRC)/loadgaia_edr3_catalog.$(ARCH).o \
$(SRC)/loadgaia_edr3_make_subset.$(ARCH).o \
$(SRC)/loadgaia_edr3_readstars.$(ARCH).o \
$(SRC)/loadgaia_edr3_table.$(ARCH).o \
$(SRC)/parse_csv.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/strhash.$(ARCH).o \
$(SRC)/sortIDs.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-ATLAS = \
$(SRC)/loadatlas.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_loadatlas.$(ARCH).o \
$(SRC)/find_matches_atlas.$(ARCH).o \
$(SRC)/loadatlas_catalog.$(ARCH).o \
$(SRC)/loadatlas_make_subset.$(ARCH).o \
$(SRC)/loadatlas_readstars.$(ARCH).o \
$(SRC)/loadatlas_table.$(ARCH).o \
$(SRC)/parse_csv.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/strhash.$(ARCH).o \
$(SRC)/sortIDs.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-UKIRT_UHS = \
$(SRC)/loadukirt_uhs.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/args_loadukirt_uhs.$(ARCH).o \
$(SRC)/find_matches_ukirt_uhs.$(ARCH).o \
$(SRC)/loadukirt_uhs_catalog.$(ARCH).o \
$(SRC)/loadukirt_uhs_make_subset.$(ARCH).o \
$(SRC)/loadukirt_uhs_readstars.$(ARCH).o \
$(SRC)/loadukirt_uhs_readstars_uhs.$(ARCH).o \
$(SRC)/loadukirt_uhs_readstars_ugcs.$(ARCH).o \
$(SRC)/loadukirt_uhs_readstars_ugps.$(ARCH).o \
$(SRC)/loadukirt_uhs_readstars_ulas.$(ARCH).o \
$(SRC)/loadukirt_uhs_readstars_uhs2022.$(ARCH).o \
$(SRC)/loadukirt_uhs_table.$(ARCH).o \
$(SRC)/parse_csv.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/strhash.$(ARCH).o \
$(SRC)/sortIDs.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-STARPAR = \
$(SRC)/loadstarpar.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/args_loadstarpar.$(ARCH).o \
$(SRC)/find_matches_starpar.$(ARCH).o \
$(SRC)/loadstarpar_catalog.$(ARCH).o \
$(SRC)/loadstarpar_io.$(ARCH).o \
$(SRC)/loadstarpar_make_subset.$(ARCH).o \
$(SRC)/loadstarpar_readstars.$(ARCH).o \
$(SRC)/loadstarpar_remote_hosts.$(ARCH).o \
$(SRC)/loadstarpar_save_remote.$(ARCH).o \
$(SRC)/loadstarpar_table.$(ARCH).o \
$(SRC)/loadstarpar_extras.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-STARPAR-CLIENT = \
$(SRC)/loadstarpar_client.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/args_loadstarpar.$(ARCH).o \
$(SRC)/find_matches_starpar.$(ARCH).o \
$(SRC)/loadstarpar_catalog.$(ARCH).o \
$(SRC)/loadstarpar_io.$(ARCH).o \
$(SRC)/loadstarpar_make_subset.$(ARCH).o \
$(SRC)/loadstarpar_readstars.$(ARCH).o \
$(SRC)/loadstarpar_remote_hosts.$(ARCH).o \
$(SRC)/loadstarpar_save_remote.$(ARCH).o \
$(SRC)/loadstarpar_table.$(ARCH).o \
$(SRC)/loadstarpar_extras.$(ARCH).o \
$(SRC)/resort_catalog.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

SET-OBJFLAGS = \
$(SRC)/setobjflags.$(ARCH).o \
$(SRC)/ConfigInit_setobjflags.$(ARCH).o \
$(SRC)/args_setobjflags.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/find_matches_setobjflags.$(ARCH).o \
$(SRC)/setobjflags_catalog.$(ARCH).o \
$(SRC)/setobjflags_io.$(ARCH).o \
$(SRC)/setobjflags_make_subset.$(ARCH).o \
$(SRC)/setobjflags_loadfile.$(ARCH).o \
$(SRC)/setobjflags_remote_hosts.$(ARCH).o \
$(SRC)/setobjflags_save_remote.$(ARCH).o \
$(SRC)/setobjflags_table.$(ARCH).o \
$(SRC)/setobjflags_extras.$(ARCH).o

# $(SRC)/resort_catalog.$(ARCH).o \
# $(SRC)/psps_ids.$(ARCH).o

SET-OBJFLAGS-CLIENT = \
$(SRC)/setobjflags_client.$(ARCH).o \
$(SRC)/ConfigInit_setobjflags.$(ARCH).o \
$(SRC)/args_setobjflags.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/find_matches_setobjflags.$(ARCH).o \
$(SRC)/setobjflags_catalog.$(ARCH).o \
$(SRC)/setobjflags_io.$(ARCH).o \
$(SRC)/setobjflags_make_subset.$(ARCH).o \
$(SRC)/setobjflags_loadfile.$(ARCH).o \
$(SRC)/setobjflags_remote_hosts.$(ARCH).o \
$(SRC)/setobjflags_save_remote.$(ARCH).o \
$(SRC)/setobjflags_table.$(ARCH).o \
$(SRC)/setobjflags_extras.$(ARCH).o

# $(SRC)/resort_catalog.$(ARCH).o \
# $(SRC)/psps_ids.$(ARCH).o

# drop resort_catalog?

LOAD-ICRF = \
$(SRC)/loadICRF.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/args_loadICRF.$(ARCH).o \
$(SRC)/find_matches_ICRF.$(ARCH).o \
$(SRC)/loadICRF_catalog.$(ARCH).o \
$(SRC)/loadICRF_io.$(ARCH).o \
$(SRC)/loadICRF_make_subset.$(ARCH).o \
$(SRC)/loadICRF_readstars.$(ARCH).o \
$(SRC)/loadICRF_remote_hosts.$(ARCH).o \
$(SRC)/loadICRF_save_remote.$(ARCH).o \
$(SRC)/loadICRF_table.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-ICRF-CLIENT = \
$(SRC)/loadICRF_client.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/args_loadICRF.$(ARCH).o \
$(SRC)/find_matches_ICRF.$(ARCH).o \
$(SRC)/loadICRF_catalog.$(ARCH).o \
$(SRC)/loadICRF_io.$(ARCH).o \
$(SRC)/loadICRF_make_subset.$(ARCH).o \
$(SRC)/loadICRF_readstars.$(ARCH).o \
$(SRC)/loadICRF_remote_hosts.$(ARCH).o \
$(SRC)/loadICRF_save_remote.$(ARCH).o \
$(SRC)/loadICRF_table.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-WISE = \
$(SRC)/loadwise.$(ARCH).o \
$(SRC)/loadwise_rawdata.$(ARCH).o \
$(SRC)/loadwise_ops.$(ARCH).o \
$(SRC)/loadwise_star_full.$(ARCH).o \
$(SRC)/args_loadwise.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/find_matches_refstars.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-TYCHO = \
$(SRC)/loadtycho.$(ARCH).o \
$(SRC)/loadtycho_rawdata.$(ARCH).o \
$(SRC)/loadtycho_ops.$(ARCH).o \
$(SRC)/args_loadtycho.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/find_matches_closest_refstars.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-BSC = \
$(SRC)/loadbsc.$(ARCH).o \
$(SRC)/loadbsc_rawdata.$(ARCH).o \
$(SRC)/loadbsc_ops.$(ARCH).o \
$(SRC)/args_loadbsc.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/find_matches_closest_refstars.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

LOAD-SUPERCOS = \
$(SRC)/loadsupercos.$(ARCH).o \
$(SRC)/loadsupercos_ops.$(ARCH).o \
$(SRC)/loadsupercos_survey.$(ARCH).o \
$(SRC)/loadsupercos_plates.$(ARCH).o \
$(SRC)/loadsupercos_rawdata.$(ARCH).o \
$(SRC)/args_loadsupercos.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SkyListForStars.$(ARCH).o \
$(SRC)/find_matches_refstars.$(ARCH).o \
$(SRC)/build_links.$(ARCH).o \
$(SRC)/replace_match.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/update_coords.$(ARCH).o \
$(SRC)/SkyRegionUtils.$(ARCH).o \
$(SRC)/parse_time.$(ARCH).o \
$(SRC)/psps_ids.$(ARCH).o

SEDSTAR = \
$(SRC)/sedstar.$(ARCH).o \
$(SRC)/SEDtableLoad.$(ARCH).o \
$(SRC)/SEDfit.$(ARCH).o \
$(SRC)/SEDops.$(ARCH).o \
$(SRC)/args_sedstar.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o

MKCMF = \
$(SRC)/mkcmf.$(ARCH).o

SKYCELLS = \
$(SRC)/skycells.$(ARCH).o \
$(SRC)/args_skycells.$(ARCH).o \
$(SRC)/ConfigInit_skycells.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \
$(SRC)/sky_tessalation.$(ARCH).o

DUMPSKYCELLS = \
$(SRC)/dumpskycells.$(ARCH).o \
$(SRC)/skycells_to_mdc.$(ARCH).o \
$(SRC)/ConfigInit_skycells.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o \

FINDSKYCELL = \
$(SRC)/findskycell.$(ARCH).o \
$(SRC)/Shutdown.$(ARCH).o 

$(ADDSTARC)   	       : $(INC)/addstar.h
$(ADDSTARD)   	       : $(INC)/addstar.h
$(ADDSTART)   	       : $(INC)/addstar.h
$(ADDSTAR)    	       : $(INC)/addstar.h
$(ADDSTAR_CLIENT)      : $(INC)/addstar.h
$(SKYCELLS)    	       : $(INC)/addstar.h
$(DUMPSKYCELLS)	       : $(INC)/addstar.h
$(FINDSKYCELL)	       : $(INC)/addstar.h
$(LOAD-2MASS)  	       : $(INC)/addstar.h $(INC)/2mass.h
$(LOAD-ATLAS)          : $(INC)/addstar.h $(INC)/atlas.h
$(LOAD-GALPHOT)	       : $(INC)/addstar.h $(INC)/loadgalphot.h
$(LOAD-GAIA)	       : $(INC)/addstar.h $(INC)/gaia.h
$(LOAD-GAIA_DR2)       : $(INC)/addstar.h $(INC)/gaia_dr2.h
$(LOAD-GAIA_EDR3)      : $(INC)/addstar.h $(INC)/gaia_edr3.h
$(LOAD-UKIRT_UHS)      : $(INC)/addstar.h $(INC)/ukirt_uhs.h
$(LOAD-STARPAR)	       : $(INC)/addstar.h $(INC)/loadstarpar.h
$(LOAD-STARPAR-CLIENT) : $(INC)/addstar.h $(INC)/loadstarpar.h
$(LOAD-ICRF)	       : $(INC)/addstar.h $(INC)/loadICRF.h
$(LOAD-ICRF-CLIENT)    : $(INC)/addstar.h $(INC)/loadICRF.h
$(LOAD-WISE)   	       : $(INC)/addstar.h $(INC)/WISE.h
$(SET-OBJFLAGS)	       : $(INC)/addstar.h $(INC)/setobjflags.h
$(SET-OBJFLAGS-CLIENT) : $(INC)/addstar.h $(INC)/setobjflags.h
$(MKCMF)       	       : $(INC)/mkcmf.h

$(BIN)/addstar.$(ARCH) 	      : $(ADDSTAR)
$(BIN)/addstar_client.$(ARCH) : $(ADDSTAR_CLIENT)
$(BIN)/addstard.$(ARCH)	      : $(ADDSTARD)
$(BIN)/addstart.$(ARCH)	      : $(ADDSTART)
$(BIN)/addstarc.$(ARCH)	      : $(ADDSTARC)
$(BIN)/mkacc-2mass.$(ARCH)    : $(MKACC-2MASS)
$(BIN)/sedstar.$(ARCH)        : $(SEDSTAR)
$(BIN)/load2mass.$(ARCH)      : $(LOAD-2MASS)
$(BIN)/loadatlas.$(ARCH)      : $(LOAD-ATLAS)
$(BIN)/loadwise.$(ARCH)       : $(LOAD-WISE)
$(BIN)/loadtycho.$(ARCH)      : $(LOAD-TYCHO)
$(BIN)/loadbsc.$(ARCH)        : $(LOAD-BSC)
$(BIN)/loadgalphot.$(ARCH)    : $(LOAD-GALPHOT)
$(BIN)/loadgaia.$(ARCH)       : $(LOAD-GAIA)
$(BIN)/loadgaia_dr2.$(ARCH)   : $(LOAD-GAIA_DR2)
$(BIN)/loadgaia_edr3.$(ARCH)  : $(LOAD-GAIA_EDR3)
$(BIN)/loadukirt_uhs.$(ARCH)  : $(LOAD-UKIRT_UHS)
$(BIN)/loadstarpar.$(ARCH)    : $(LOAD-STARPAR)
$(BIN)/loadstarpar_client.$(ARCH) : $(LOAD-STARPAR-CLIENT)
$(BIN)/loadICRF.$(ARCH)       : $(LOAD-ICRF)
$(BIN)/loadICRF_client.$(ARCH): $(LOAD-ICRF-CLIENT)
$(BIN)/loadsupercos.$(ARCH)   : $(LOAD-SUPERCOS)
$(BIN)/setobjflags.$(ARCH)    : $(SET-OBJFLAGS)
$(BIN)/setobjflags_client.$(ARCH) : $(SET-OBJFLAGS-CLIENT)
$(BIN)/skycells.$(ARCH)       : $(SKYCELLS)
$(BIN)/dumpskycells.$(ARCH)   : $(DUMPSKYCELLS)
$(BIN)/findskycell.$(ARCH)    : $(FINDSKYCELL)
$(BIN)/mkcmf.$(ARCH)          : $(MKCMF)

# I need to fix the client/server version of addstar now that I have dropped Stars
# as an autocode type and have converted to carrying average and measure
# all: addstar addstarc addstard addstart sedstar load2mass skycells
# INSTALL = addstar addstarc addstard addstart sedstar load2mass skycells mkcmf

# dependancy rules for binary code #########################
$(INSTALL): % : $(BIN)/%.$(ARCH)

%.clean :
	rm -f $(BIN)/$*.$(ARCH)

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(INSTALL); do $(MAKE) $$i.install || exit; done
