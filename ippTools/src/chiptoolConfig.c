/*
 * chiptoolConfig.c
 *
 * Copyright (C) 2006-2008  Joshua Hoblitt
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

#include <stdint.h>

#include <psmodules.h>

#include "pxtools.h"
#include "chiptool.h"

pxConfig *chiptoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    config->modules = pmConfigRead(&argc, argv, NULL);
    if (! config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    pxchipSetSearchArgs (definebyqueryArgs);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by rawExp label (LIKE comparison)", NULL);

    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-exp_type",          PS_META_REPLACE, "search by exp_type", "object");

    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",  0,            "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",  0,            "define label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",  0,            "define reduction class", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_expgroup",  0,            "define exposure group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dvodb",  0,            "define DVO db", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_tess_id",  0,            "define tessellation identifier", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_end_stage",  0,            "define end stage", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group",  0,      "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_dist_group",  0,      "define distribution group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note",  0,           "define note", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-unique",   0,           "only queue exposures that have no previous chipRun", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0,            "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(definebyqueryArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-random",  0,            "randomly sort the output", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -definecopy
    psMetadata *definecopyArgs = psMetadataAlloc();
    pxchipSetSearchArgs (definecopyArgs);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by chipRun label (LIKE comparison)", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by chipRun label (LIKE comparison)", NULL);

    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-exp_type", PS_META_REPLACE, "search by exp_type", "object");
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-state", 0, "search by chipRun.state", NULL);

    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_label", 0, "define label (required)", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction class", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_expgroup", 0, "define exposure group", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_dvodb", 0, "define DVO db", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_tess_id", 0, "define tessellation identifier", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_end_stage", 0, "define end stage", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define distribution group", NULL);
    psMetadataAddStr(definecopyArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-pretend", 0, "do not actually modify the database", false);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definecopyArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    pxchipSetSearchArgs (updaterunArgs);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-chip_id",              0,            "search by chip ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0,            "search by state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",  0,           "search by label (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group",  0,      "search by data_group (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-dist_group",  0,      "search by dist_group (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0,        "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0,        "set label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "set data group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0,   "set dist group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "set note", NULL);
    pxmagicAddArguments(updaterunArgs);

    // -setimfiletoupdate
    psMetadata *setimfiletoupdateArgs = psMetadataAlloc();
    psMetadataAddS64(setimfiletoupdateArgs, PS_LIST_TAIL, "-chip_id", 0,   "search by chip ID (required)", 0);
    psMetadataAddStr(setimfiletoupdateArgs, PS_LIST_TAIL, "-class_id", 0,  "search by class_id", NULL);
    psMetadataAddStr(setimfiletoupdateArgs, PS_LIST_TAIL, "-set_label",  0,"new value for label", NULL);
    psMetadataAddS16(setimfiletoupdateArgs, PS_LIST_TAIL, "-set_update_mode",  0,"new value for update_mode", 0);

    // -pendingimfile
    psMetadata *pendingimfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingimfileArgs, PS_LIST_TAIL, "-chip_id",  0,            "search by chip ID", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-class_id",  0,          "search by class_id", 0);
    pxchipSetSearchArgs(pendingimfileArgs);
    psMetadataAddU64(pendingimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-all",  0,            "list everything without search terms", false);

    // -addprocessedimfile
    psMetadata *addprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-chip_id",  0,            "define chip ID (required)", 0);
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "define exposure ID (required)", 0);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-class_id",  0,            "define class ID (required)", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-uri",  0,            "define URL", NULL);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-bg",  0,            "define exposure background", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-bg_stdev",  0,            "define exposure background stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-bg_mean_stdev",  0,            "define exposure background mean stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-bias",  0,            "define bias", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-bias_stdev",  0,            "define bias stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fringe_0",  0,            "define fringe term 0", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fringe_1",  0,            "define fringe term 1", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fringe_2",  0,            "define fringe term 2", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-ap_resid",  0,            "define aperture residual", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-ap_resid_stdev",  0,            "define aperture residual stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fwhm_major",  0,            "define fwhm (major axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fwhm_major_lq",  0,            "define fwhm (major axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fwhm_major_uq",  0,            "define fwhm (major axis; arcsec)", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fwhm_minor",  0,            "define fwhm (minor axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fwhm_minor_lq",  0,            "define fwhm (minor axis; arcsec)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-fwhm_minor_uq",  0,            "define fwhm (minor axis; arcsec)", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_fwhm_major",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_fwhm_major_err",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_fwhm_minor",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_fwhm_minor_err",     0,  "define moment value m2 mean", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2",     0,  "define moment value m2 mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2_err", 0,  "define moment value m2 stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2_lq",  0,  "define moment value m2 lower quartile", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2_uq",  0,  "define moment value m2 upper quartile", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2c",     0,  "define moment value m2c mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2c_err", 0,  "define moment value m2c stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2c_lq",  0,  "define moment value m2c lower quartile", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2c_uq",  0,  "define moment value m2c upper quartile", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2s",     0,  "define moment value m2s mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2s_err", 0,  "define moment value m2s stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2s_lq",  0,  "define moment value m2s lower quartile", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m2s_uq",  0,  "define moment value m2s upper quartile", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m3",     0,  "define moment value m3 mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m3_err", 0,  "define moment value m3 stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m3_lq",  0,  "define moment value m3 lower quartile", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m3_uq",  0,  "define moment value m3 upper quartile", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m4",     0,  "define moment value m4 mean", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m4_err", 0,  "define moment value m4 stdev", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m4_lq",  0,  "define moment value m4 lower quartile", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-iq_m4_uq",  0,  "define moment value m4 upper quartile", NAN);

    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-dtime_detrend",  0,            "define elapsed time for detrend (seconds)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-dtime_photom",  0,            "define elapsed time for photometry (seconds)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-dtime_total",  0,            "define total elapsed time (seconds)", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-dtime_script",  0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-hostname",  0,            "define hostname", NULL);
    psMetadataAddS32(addprocessedimfileArgs, PS_LIST_TAIL, "-n_stars",  0,            "define number of stars in image", 0);
    psMetadataAddS32(addprocessedimfileArgs, PS_LIST_TAIL, "-n_psfstars",  0,          "define number of stars used for psf", 0);
    psMetadataAddS32(addprocessedimfileArgs, PS_LIST_TAIL, "-n_iqstars",  0,          "define number of stars used for IQ stats", 0);
    psMetadataAddS32(addprocessedimfileArgs, PS_LIST_TAIL, "-n_extended",  0,            "define number of extended objects in image", 0);
    psMetadataAddS32(addprocessedimfileArgs, PS_LIST_TAIL, "-n_cr",  0,            "define number of cosmic rays in image", 0);

    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-path_base",  0,            "define base output location", NULL);
    psMetadataAddS16(addprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(addprocessedimfileArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);
    psMetadataAddS64(addprocessedimfileArgs, PS_LIST_TAIL, "-magicked",  0,        "define magicked status", 0);

    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_psastro", 0, "define psastro version", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_ppimage", 0, "define ppImage version", NULL);
    psMetadataAddStr(addprocessedimfileArgs, PS_LIST_TAIL, "-ver_streaks", 0, "define streaksremove version", NULL);
    
    psMetadataAddS32(addprocessedimfileArgs, PS_LIST_TAIL, "-maskfrac_npix", 0, "define number of pixels used for maskstats", 0);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-maskfrac_static", 0, "define static mask fraction", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-maskfrac_dynamic", 0, "define dynamic mask fraction", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-maskfrac_magic", 0, "define magic mask fraction", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-maskfrac_advisory", 0, "define advisory mask fraction", NAN);
    psMetadataAddF32(addprocessedimfileArgs, PS_LIST_TAIL, "-deteff_magref", 0, "define deteff_magref", NAN);

    // -processedimfile
    psMetadata *processedimfileArgs = psMetadataAlloc();
    pxchipSetSearchArgs(processedimfileArgs);
    psMetadataAddS64(processedimfileArgs, PS_LIST_TAIL, "-chip_id",  0,         "search by  chip ID", 0);
    psMetadataAddS64(processedimfileArgs, PS_LIST_TAIL, "-chip_imfile_id",  0,  "search by chip_file_id", 0);
    psMetadataAddStr(processedimfileArgs,  PS_LIST_TAIL, "-class_id",           0, "search by class ID", NULL);
    psMetadataAddStr(processedimfileArgs,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);
    psMetadataAddStr(processedimfileArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by chipRun label (LIKE comparison)", NULL);
    psMetadataAddStr(processedimfileArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by chipRun data_group (LIKE comparison)", NULL);
    pxmagicAddArguments(processedimfileArgs);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);

    psMetadataAddU64(processedimfileArgs,  PS_LIST_TAIL, "-limit",  0,           "limit result set to N items", 0);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-all",  0,            "list everything without search terms", false);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-faulted",  0,        "only return imfiles with a fault status set", false);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-allfiles",  0,       "only all matching imfiles regardless of fault status", false);
    psMetadataAddBool(processedimfileArgs, PS_LIST_TAIL, "-simple",  0,         "use the simple output format", false);

    pxspaceAddArguments(processedimfileArgs);

    // -revertprocessedimfile
    psMetadata *revertprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(revertprocessedimfileArgs, PS_LIST_TAIL, "-chip_id", 0,            "search by chip ID", 0);
    psMetadataAddStr(revertprocessedimfileArgs,  PS_LIST_TAIL, "-class_id",           0, "search by class ID", NULL);
    psMetadataAddStr(revertprocessedimfileArgs,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);
    psMetadataAddStr(revertprocessedimfileArgs,  PS_LIST_TAIL, "-label",              PS_META_DUPLICATE_OK, "search by chipRun label (LIKE comparison)", NULL);
    pxchipSetSearchArgs(revertprocessedimfileArgs);
    psMetadataAddBool(revertprocessedimfileArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);
    psMetadataAddS16(revertprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -updateprocessedimfile
    psMetadata *updateprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedimfileArgs, PS_LIST_TAIL, "-chip_id",  0,            "search by chip ID", 0);
    psMetadataAddStr(updateprocessedimfileArgs,  PS_LIST_TAIL, "-class_id",           0, "search by class ID", NULL);
    psMetadataAddS16(updateprocessedimfileArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code (required)", 0);
    psMetadataAddStr(updateprocessedimfileArgs, PS_LIST_TAIL, "-set_state", 0,         "set state", NULL);
    psMetadataAddS16(updateprocessedimfileArgs, PS_LIST_TAIL, "-set_quality",  0,            "set quality", 0);

    // -dropprocessedimfile
    psMetadata *dropprocessedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(dropprocessedimfileArgs, PS_LIST_TAIL, "-chip_id",  0,            "search by chip ID", 0);
    psMetadataAddS64(dropprocessedimfileArgs, PS_LIST_TAIL, "-cam_id",  0,            "search by cam ID", 0);
    psMetadataAddStr(dropprocessedimfileArgs,  PS_LIST_TAIL, "-class_id",           0, "search by class ID (required)", NULL);
    psMetadataAddS16(dropprocessedimfileArgs, PS_LIST_TAIL, "-set_quality",  0,        "set quality (required)", 0);

    // -listrun
    psMetadata *listrunArgs = psMetadataAlloc();
    pxchipSetSearchArgs(listrunArgs);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-chip_id",  0,         "search by  chip ID", 0);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by chipRun label (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-state",              0, "search by chipRun state", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-tess_id",            0, "search by chipRun tess_id (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by chipRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-dist_group",  PS_META_DUPLICATE_OK, "search by chipRun dist_group (LIKE comparison)", NULL);
    pxmagicAddArguments(listrunArgs);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);
    psMetadataAddU64(listrunArgs,  PS_LIST_TAIL, "-limit",  0,           "limit result set to N items", 0);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-all",  0,            "list everything without search terms", false);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-simple",  0,         "use the simple output format", false);
    pxspaceAddArguments(listrunArgs);

    // -block
    psMetadata *blockArgs = psMetadataAlloc();
    psMetadataAddStr(blockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to mask out (required)", NULL);

    // -masked
    psMetadata *maskedArgs = psMetadataAlloc();
    psMetadataAddStr(maskedArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(maskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(maskedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -unmasked
    psMetadata *unmaskedArgs = psMetadataAlloc();
    psMetadataAddStr(unmaskedArgs, PS_LIST_TAIL, "-label",  0,            "restrict to specified label", NULL);
    psMetadataAddBool(unmaskedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(unmaskedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -unblock
    psMetadata *unblockArgs = psMetadataAlloc();
    psMetadataAddStr(unblockArgs, PS_LIST_TAIL, "-label",  0,            "name of a label to unmask (required)", NULL);

    // -pendingcleanuprun
    psMetadata *pendingcleanuprunArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanuprunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanuprunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingcleanuprunArgs, PS_LIST_TAIL, "-all",  0,            "list all components regardless of data_state", false);

    // -pendingcleanupimfile
    psMetadata *pendingcleanupimfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupimfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupimfileArgs, PS_LIST_TAIL, "-chip_id", 0,          "search by chip ID", 0);
    psMetadataAddStr(pendingcleanupimfileArgs, PS_LIST_TAIL, "-exp_id",                 0,            "search by exp_id", NULL);
    psMetadataAddBool(pendingcleanupimfileArgs, PS_LIST_TAIL, "-all",  0,            "list all components regardless of data_state", false);
    psMetadataAddBool(pendingcleanupimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -revertcleanup
    psMetadata *revertcleanupArgs = psMetadataAlloc();
    psMetadataAddS64(revertcleanupArgs, PS_LIST_TAIL, "-chip_id", 0,            "search by chip ID", 0);
    psMetadataAddStr(revertcleanupArgs,  PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by chipRun label (LIKE comparison)", NULL);
    psMetadataAddStr(revertcleanupArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by chipRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-state", 0,             "search by current state", NULL);
    psMetadataAddS16(revertcleanupArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -run
    psMetadata *runArgs = psMetadataAlloc();
    psMetadataAddStr(runArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddBool(runArgs, PS_LIST_TAIL, "-simple",  0,     "use the simple output format", false);
    psMetadataAddU64(runArgs, PS_LIST_TAIL, "-limit",  0,       "limit result set to N items", 0);
    psMetadataAddStr(runArgs, PS_LIST_TAIL, "-state", 0,        "search by state (required)", NULL);
    pxchipSetSearchArgs(runArgs);
    psMetadataAddS64(runArgs, PS_LIST_TAIL, "-chip_id",  0,         "search by  chip ID", 0);
    psMetadataAddStr(runArgs,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);

    // -advanceexp
    psMetadata *advanceexpArgs = psMetadataAlloc();
    psMetadataAddS64(advanceexpArgs, PS_LIST_TAIL, "-chip_id",  0,          "search by chip ID", 0);
    psMetadataAddStr(advanceexpArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "advance exposures for specified label", NULL);
    psMetadataAddU64(advanceexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -tocleanedimfile
    psMetadata *tocleanedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedimfileArgs, PS_LIST_TAIL, "-chip_id", 0,          "chip ID to update", 0);
    psMetadataAddStr(tocleanedimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -tofullimfile
    psMetadata *tofullimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tofullimfileArgs, PS_LIST_TAIL, "-chip_id", 0,          "chip ID to update", 0);
    psMetadataAddStr(tofullimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -topurgedimfile
    psMetadata *topurgedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(topurgedimfileArgs, PS_LIST_TAIL, "-chip_id", 0,          "chip ID to update", 0);
    psMetadataAddStr(topurgedimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -toscrubbedimfile
    psMetadata *toscrubbedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(toscrubbedimfileArgs, PS_LIST_TAIL, "-chip_id", 0,        "chip ID to update", 0);
    psMetadataAddStr(toscrubbedimfileArgs, PS_LIST_TAIL, "-class_id", 0,       "class ID to update", NULL);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    // pxchipSetSearchArgs (exportrunArgs); XXX include search terms?
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-chip_id", 0,          "export this chip ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "mark tables as cleaned", false);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);

    // -runstate
    psMetadata *runstateArgs = psMetadataAlloc();
    psMetadataAddS64(runstateArgs, PS_LIST_TAIL, "-chip_id", 0,           "search by chip ID", 0);
    psMetadataAddS64(runstateArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure tag", 0);
    psMetadataAddStr(runstateArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure tag", 0);
    psMetadataAddStr(runstateArgs, PS_LIST_TAIL,  "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-no_magic",  0,        "magic is not necessary for result", false);

    psMetadataAddU64(runstateArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from raw exposures",       CHIPTOOL_MODE_DEFINEBYQUERY,        definebyqueryArgs);
    PXOPT_ADD_MODE("-definecopy",           "create copy runs",                     CHIPTOOL_MODE_DEFINECOPY,        definecopyArgs);
    PXOPT_ADD_MODE("-updaterun",            "change chip run properties",           CHIPTOOL_MODE_UPDATERUN,            updaterunArgs);
    PXOPT_ADD_MODE("-setimfiletoupdate",    "set imfile state to update",           CHIPTOOL_MODE_SETIMFILETOUPDATE,    setimfiletoupdateArgs);
    PXOPT_ADD_MODE("-pendingimfile",        "show pending imfiles",                 CHIPTOOL_MODE_PENDINGIMFILE,        pendingimfileArgs);
    PXOPT_ADD_MODE("-addprocessedimfile",   "add a processed imfile",               CHIPTOOL_MODE_ADDPROCESSEDIMFILE,   addprocessedimfileArgs);
    PXOPT_ADD_MODE("-processedimfile",      "show processed imfiles",               CHIPTOOL_MODE_PROCESSEDIMFILE,      processedimfileArgs);
    PXOPT_ADD_MODE("-updateprocessedimfile","change procesed imfile properties",    CHIPTOOL_MODE_UPDATEPROCESSEDIMFILE,updateprocessedimfileArgs);
    PXOPT_ADD_MODE("-dropprocessedimfile","drop previously procesed imfile",        CHIPTOOL_MODE_DROPPROCESSEDIMFILE,  dropprocessedimfileArgs);
    PXOPT_ADD_MODE("-revertprocessedimfile","undo a processed imfile",              CHIPTOOL_MODE_REVERTPROCESSEDIMFILE,revertprocessedimfileArgs);
    PXOPT_ADD_MODE("-runstate",             "list the states of chip run",          CHIPTOOL_MODE_RUNSTATE,             runstateArgs);
    PXOPT_ADD_MODE("-listrun",              "list chipRuns",                        CHIPTOOL_MODE_LISTRUN,              listrunArgs);
    PXOPT_ADD_MODE("-advanceexp",           "advance completed exposures",          CHIPTOOL_MODE_ADVANCEEXP,           advanceexpArgs);
    PXOPT_ADD_MODE("-block",                "set a label block",                    CHIPTOOL_MODE_BLOCK,                blockArgs);
    PXOPT_ADD_MODE("-masked",               "show blocked labels",                  CHIPTOOL_MODE_MASKED,               maskedArgs);
    PXOPT_ADD_MODE("-unmasked",             "",                                     CHIPTOOL_MODE_UNMASKED,             unmaskedArgs);
    PXOPT_ADD_MODE("-unblock",              "remove a label block",                 CHIPTOOL_MODE_UNBLOCK,              unblockArgs);
    PXOPT_ADD_MODE("-pendingcleanuprun",    "show runs that need to be cleaned up", CHIPTOOL_MODE_PENDINGCLEANUPRUN,    pendingcleanuprunArgs);
    PXOPT_ADD_MODE("-pendingcleanupimfile", "show runs that need to be cleaned up", CHIPTOOL_MODE_PENDINGCLEANUPIMFILE, pendingcleanupimfileArgs);
    PXOPT_ADD_MODE("-donecleanup",          "show runs that have been cleaned",     CHIPTOOL_MODE_DONECLEANUP,          donecleanupArgs);
    PXOPT_ADD_MODE("-revertcleanup",        "show runs that have been cleaned",     CHIPTOOL_MODE_REVERTCLEANUP,        revertcleanupArgs);
    PXOPT_ADD_MODE("-run",                  "show runs",                            CHIPTOOL_MODE_RUN,                  runArgs);
    PXOPT_ADD_MODE("-tocleanedimfile",      "set imfile state to cleaned",          CHIPTOOL_MODE_TOCLEANEDIMFILE,      tocleanedimfileArgs);
    PXOPT_ADD_MODE("-tofullimfile",         "set imfile state to full",              CHIPTOOL_MODE_TOFULLIMFILE,         tofullimfileArgs);
    PXOPT_ADD_MODE("-topurgedimfile",       "set imfile state to purged",            CHIPTOOL_MODE_TOPURGEDIMFILE,       topurgedimfileArgs);
    PXOPT_ADD_MODE("-toscrubbedimfile",     "set imfile state to scrubbed",          CHIPTOOL_MODE_TOSCRUBBEDIMFILE,     toscrubbedimfileArgs);

    PXOPT_ADD_MODE("-exportrun",            "export run for import on other database", CHIPTOOL_MODE_EXPORTRUN, exportrunArgs);
    PXOPT_ADD_MODE("-importrun",            "import run from metadata file",           CHIPTOOL_MODE_IMPORTRUN, importrunArgs);

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
