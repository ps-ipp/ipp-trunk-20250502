/*
 * camtoolConfig.c
 *
 * Copyright (C) 2006  Joshua Hoblitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "pxcam.h"
#include "camtool.h"

pxConfig *camtoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    // -definebyquery
    // XXX need to allow multiple chip_ids
    // XXX need to allow multiple exp_ids
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    pxcamSetSearchArgs(definebyqueryArgs);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by chipRun label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-reduction",          0, "search by chipRun reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-data_group",         0, "search by chipRun data_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-obs_mode",           0, "search by rawExp obs_mode", NULL);

    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",        0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",          0, "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",      0, "define reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_expgroup",       0, "define exposure group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb",          0, "define DVO db", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_tess_id",        0, "define tess ID", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_end_stage",      0, "define end stage", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group",     0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dist_group",     0, "define dist group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note",           0, "define note", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",           0, "do not actual modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);

    // -updaterun
    // XXX need to allow multiple cam_ids
    // XXX need to allow multiple chip_ids
    // XXX need to allow multiple exp_ids
    psMetadata *updaterunArgs = psMetadataAlloc();
    pxcamSetSearchArgs(updaterunArgs);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-cam_id",             0, "search by cam_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",              0, "search by camRun label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group",         0, "search by camRun data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state",              0, "search by camRun state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-reduction",          0, "search by camRun reduction class", NULL);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-all",               0, "allow everything to be queued without search terms", false);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state",          0, "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label",          0, "set label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "define new note", NULL);
    pxmagicAddArguments(updaterunArgs);

    // -pendingexp
    psMetadata *pendingexpArgs = psMetadataAlloc();
    pxcamSetSearchArgs(pendingexpArgs);
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-cam_id",            0, "search by cam_id", 0);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by camRun label", NULL);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-reduction",         0, "search by camRun reduction class", NULL);
    psMetadataAddU64(pendingexpArgs, PS_LIST_TAIL, "-limit",             0, "limit result set to N items", 0);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-simple",           0, "use the simple output format", false);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-all",               0, "allow everything to be queued without search terms", false);

    // -pendingimfile
    psMetadata *pendingimfileArgs = psMetadataAlloc();
    pxcamSetSearchArgs(pendingimfileArgs);
    psMetadataAddS64(pendingimfileArgs, PS_LIST_TAIL, "-cam_id",   0,            "search by camtool ID", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by camRun label", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-reduction",0,            "search by camRun reduction class", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-class_id", 0,            "search by class ID", NULL);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-all",     0,            "allow everything to be queued without search terms", false);

    // XXX is this used? psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-class",    0,            "search by class", NULL);

    // -addprocessedexp
    psMetadata *addprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-cam_id", 0,            "define camtool ID (required)", 0);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-uri", 0,            "define URI (required)", NULL);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-bg", 0,            "define exposure background", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-bg_stdev", 0,            "define exposure background stdev", NAN);
    psMetadataAddF64(addprocessedexpArgs, PS_LIST_TAIL, "-bg_mean_stdev", 0,            "define exposure background mean stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-bias",  0,            "define bias", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-bias_stdev",  0,            "define bias stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fringe_0",  0,            "define fringe term 0", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fringe_1",  0,            "define fringe term 1", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fringe_2",  0,            "define fringe term 2", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-sigma_ra", 0,            "define exposure E ra", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-sigma_dec", 0,            "define exposure E dec", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ap_resid",  0,            "define aperture residual", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ap_resid_stdev",  0,            "define aperture residual stdev", NAN);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_obs", 0,   "define observed zero point", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_err", 0,   "define observed zero point error", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_uq", 0,   "define observed zero point upper quartile", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-zpt_lq", 0,   "define observed zero point lower quartile", 0);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fwhm_major",  0,            "define fwhm (major axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fwhm_major_lq",  0,            "define fwhm (major axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fwhm_major_uq",  0,            "define fwhm (major axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fwhm_minor",  0,            "define fwhm (minor axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fwhm_minor_lq",  0,            "define fwhm (minor axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-fwhm_minor_uq",  0,            "define fwhm (minor axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_fwhm_major",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_fwhm_major_err",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_fwhm_minor",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_fwhm_minor_err",     0,  "define moment value m2 mean", NAN);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2_err", 0,  "define moment value m2 stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2_lq",  0,  "define moment value m2 lower quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2_uq",  0,  "define moment value m2 upper quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2c",     0,  "define moment value m2c mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2c_err", 0,  "define moment value m2c stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2c_lq",  0,  "define moment value m2c lower quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2c_uq",  0,  "define moment value m2c upper quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2s",     0,  "define moment value m2s mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2s_err", 0,  "define moment value m2s stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2s_lq",  0,  "define moment value m2s lower quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m2s_uq",  0,  "define moment value m2s upper quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m3",     0,  "define moment value m3 mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m3_err", 0,  "define moment value m3 stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m3_lq",  0,  "define moment value m3 lower quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m3_uq",  0,  "define moment value m3 upper quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m4",     0,  "define moment value m4 mean", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m4_err", 0,  "define moment value m4 stdev", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m4_lq",  0,  "define moment value m4 lower quartile", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-iq_m4_uq",  0,  "define moment value m4 upper quartile", NAN);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-dtime_astrom", 0, "define elapsed time for astrometry (seconds)", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-dtime_addstar", 0, "define elapsed time for DVO insertion (seconds)", NAN);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-hostname", 0,            "define hostname", NULL);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_stars", 0,            "define number of stars", 0);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_psfstars", 0,            "define number of PSF stars", 0);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_iqstars", 0,            "define number of IQ stars", 0);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_extended", 0,            "define number of extended objects", 0);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_cr", 0,            "define number of cosmic rays", 0);
    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-n_astrom", 0,            "define number of astrometry reference objects", 0);

    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", NULL);
    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);
    psMetadataAddBool(addprocessedexpArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-magicked",   0,         "set magicked", 0);

    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_psastro", 0, "define psastro version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_ppimage", 0, "define ppImage version", NULL);
    psMetadataAddStr(addprocessedexpArgs, PS_LIST_TAIL, "-ver_streaks", 0, "define streaksremove version", NULL);

    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_ref_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_ref_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_ref_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_ref_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_ref_advisory", 0, "define advisory mask fraction", NAN);

    psMetadataAddS32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_max_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_max_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_max_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_max_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-maskfrac_max_advisory", 0, "define advisory mask fraction", NAN);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst", 0, "define deteff", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst_err", 0, "define deteff_err", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst_lq", 0, "define deteff_lq", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-deteff_inst_uq", 0, "define deteff_uq", NAN);
    psMetadataAddS16(addprocessedexpArgs, PS_LIST_TAIL, "-background_model", 0, "set background_model value", 0);
    psMetadataAddS64(addprocessedexpArgs, PS_LIST_TAIL, "-astrom_chips", 0, "chips with successful astrom", 0);

    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ast_r0", 0,   "define boresite offset in RA", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ast_d0", 0,   "define boresite offset in DEC", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ast_t0", 0,   "define boresite angle", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ast_s0", 0,   "define bosite scale", NAN); 
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ast_rs", 0,   "define bosite scatter in RA", NAN);
    psMetadataAddF32(addprocessedexpArgs, PS_LIST_TAIL, "-ast_ds", 0,   "define bosite scatter in DEC", NAN);

    // -processedexp
    psMetadata *processedexpArgs = psMetadataAlloc();
    pxcamSetSearchArgs(processedexpArgs);
    psMetadataAddS64(processedexpArgs, PS_LIST_TAIL, "-cam_id",   0,            "search by cam_id", 0);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by camRun label", NULL);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by camRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(processedexpArgs, PS_LIST_TAIL, "-reduction",0,            "search by camRun reduction class", NULL);
    pxspaceAddArguments(processedexpArgs);
    pxmagicAddArguments(processedexpArgs);

    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-pstamp_order",  0,      "order the results for the postage stamp server", false);

    psMetadataAddU64(processedexpArgs, PS_LIST_TAIL, "-limit",    0,            "limit result set to N items", 0);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-all",     0,            "list everything without restriction", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(processedexpArgs, PS_LIST_TAIL, "-faulted", 0,            "only return imfiles with a fault status set", false);
    psMetadataAddS16(processedexpArgs, PS_LIST_TAIL, "-background_model", 0, "search by background_model value", 0);

    // -revertprocessedexp
    // XXX need to allow multiple cam_ids
    // XXX need to allow multiple chip_ids
    // XXX need to allow multiple exp_ids
    psMetadata *revertprocessedexpArgs = psMetadataAlloc();
    pxcamSetSearchArgs(revertprocessedexpArgs);
    psMetadataAddS64(revertprocessedexpArgs, PS_LIST_TAIL, "-cam_id",   0,            "search by cam_id", 0);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "search by camRun label", NULL);
    psMetadataAddStr(revertprocessedexpArgs, PS_LIST_TAIL, "-reduction",0,            "search by camRun reduction class", NULL);
    psMetadataAddS16(revertprocessedexpArgs, PS_LIST_TAIL, "-code",     0,            "search by fault code", 0);

    psMetadataAddBool(revertprocessedexpArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);
    psMetadataAddS16(revertprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -updateprocessedexp
    // XXX allow full search options?
    psMetadata *updateprocessedexpArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-cam_id", 0,            "search by camtool ID", 0);
    psMetadataAddS64(updateprocessedexpArgs, PS_LIST_TAIL, "-chip_id",  0,            "search by chiptool ID", 0);
    psMetadataAddStr(updateprocessedexpArgs, PS_LIST_TAIL, "-class",  0,            "search by class", NULL);
    psMetadataAddStr(updateprocessedexpArgs, PS_LIST_TAIL, "-class_id",  0,            "search by class ID", NULL);
    psMetadataAddS16(updateprocessedexpArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code (required)", INT16_MAX);
    psMetadataAddS16(updateprocessedexpArgs, PS_LIST_TAIL, "-set_quality",  0,            "set quality", 0);
    psMetadataAddS16(updateprocessedexpArgs, PS_LIST_TAIL, "-set_background_model", 0, "set background_model value", 0);

    // -block
    psMetadata *blockArgs = psMetadataAlloc();
    psMetadataAddStr(blockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to mask out (required)", NULL);

    // -masked
    psMetadata *maskedArgs = psMetadataAlloc();
    psMetadataAddBool(maskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -unblock
    psMetadata *unblockArgs = psMetadataAlloc();
    psMetadataAddStr(unblockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to unmask (required)", NULL);

    // -pendingcleanuprun
    // XXX allow full search options?
    psMetadata *pendingcleanuprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanuprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanuprunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -pendingcleanupexp
    // XXX allow full search options?
    psMetadata *pendingcleanupexpArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupexpArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupexpArgs, PS_LIST_TAIL, "-cam_id", 0,            "search by camera ID", 0);
    psMetadataAddStr(pendingcleanupexpArgs, PS_LIST_TAIL, "-exp_id",                 0,            "search by exp_id", NULL);
    psMetadataAddBool(pendingcleanupexpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanupexpArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-cam_id", 0,          "export this camera ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "export tables as cleaned", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);


    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from chip stage",          CAMTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-updaterun",            "change cam run properties",            CAMTOOL_MODE_UPDATERUN,      updaterunArgs);
    PXOPT_ADD_MODE("-pendingexp",           "show pending exposures",               CAMTOOL_MODE_PENDINGEXP,    pendingexpArgs);
    PXOPT_ADD_MODE("-pendingimfile",        "show pending imfiles",                 CAMTOOL_MODE_PENDINGIMFILE, pendingimfileArgs);
    PXOPT_ADD_MODE("-addprocessedexp",      "add a processed exposure",             CAMTOOL_MODE_ADDPROCESSEDEXP, addprocessedexpArgs);
    PXOPT_ADD_MODE("-processedexp",         "show processed exposures",             CAMTOOL_MODE_PROCESSEDEXP,  processedexpArgs);
    PXOPT_ADD_MODE("-revertprocessedexp",   "undo a processed exposure",       CAMTOOL_MODE_REVERTPROCESSEDEXP,  revertprocessedexpArgs);
    PXOPT_ADD_MODE("-updateprocessedexp",   "changed processed exp properties",            CAMTOOL_MODE_UPDATEPROCESSEDEXP,updateprocessedexpArgs);
    PXOPT_ADD_MODE("-block",                "set a label block",                    CAMTOOL_MODE_BLOCK,         blockArgs);
    PXOPT_ADD_MODE("-masked",               "show blocked labels",                  CAMTOOL_MODE_MASKED,        maskedArgs);
    PXOPT_ADD_MODE("-unblock",              "remove a label block",                 CAMTOOL_MODE_UNBLOCK,       unblockArgs);
    PXOPT_ADD_MODE("-pendingcleanuprun",    "show runs that need to be cleaned up", CAMTOOL_MODE_PENDINGCLEANUPRUN, pendingcleanuprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupexp",    "show exposures for cleanup runs",      CAMTOOL_MODE_PENDINGCLEANUPEXP, pendingcleanupexpArgs);
    PXOPT_ADD_MODE("-donecleanup",          "show runs that have been cleaned",     CAMTOOL_MODE_DONECLEANUP,       donecleanupArgs);
   PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", CAMTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           CAMTOOL_MODE_IMPORTRUN, importrunArgs);

    if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
        psError(PS_ERR_UNKNOWN, false, "option parsing failed");
        psFree(argSets);
        psFree(modes);
        psFree(config);
        return NULL;
    }

    psFree(argSets);
    psFree(modes);

    // define Database handle, if used
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}
