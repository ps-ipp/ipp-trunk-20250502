/*
 * staticskytoolConfig.c
 *
 * Copyright (C) 2007-2010  Joshua Hoblitt, Paul Price
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

#include <psmodules.h>

#include "pxtools.h"
#include "staticskytool.h"

pxConfig *staticskytoolConfig(pxConfig *config, int argc, char **argv)
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

    psTime *now = psTimeGetNow(PS_TIME_TAI);

    // -definebyquery
    // XXX need a 'ra_min,max', 'dec_min,max' : to do this, we need a table of skycell boundaries
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddS64(definebyqueryArgs,  PS_LIST_TAIL, "-select_stack_id", 0, "search for stack_id", 0);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_skycell_id", 0, "search for skycell_id", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_tess_id", 0, "search for tess_id", NULL);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_good_frac_min", 0, "define min good_frac", NAN);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_label", PS_META_DUPLICATE_OK, "search by stackRun label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_data_group", PS_META_DUPLICATE_OK, "search by stackRun data_group (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_filter", PS_META_DUPLICATE_OK, "search by filter (LIKE comparison, multiple OK)", NULL);
    pxskycellAddArguments(definebyqueryArgs);
#ifdef notdef
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_glat_min", 0, "define min galactic latitude", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_glat_max", 0, "define max galactic latitude", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_rahours_min", 0, "define min RA in hours", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_rahours_max", 0, "define max RA in hours", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_radeg_min", 0, "define min RA in degrees", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_radeg_max", 0, "define max RA in degrees", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_decdeg_min", 0, "define min DEC in degrees", NAN);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_decdeg_max", 0, "define max DEC in degrees", NAN);
#endif
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_label", 0, "define label (required)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_reduction", 0, "define reduction", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-group_by_data_group",  0, "queue unique run for each data_group", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-rerun",  0, "queue new run even if one already exists for inputs", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-check_inputs",  0, "list inputs, do not modify database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-sky_id", 0, "search by stack ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0, "search by label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess_id", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", NULL);
    pxskycellAddArguments(updaterunArgs);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0, "define new value for label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0, "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0, "define new note", NULL);

    // -inputs
    psMetadata *inputsArgs = psMetadataAlloc();
    psMetadataAddS64(inputsArgs, PS_LIST_TAIL, "-sky_id", 0, "search by staticsky ID", 0);
    psMetadataAddU64(inputsArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(inputsArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -todo
    psMetadata *todoArgs = psMetadataAlloc();
    psMetadataAddS64(todoArgs, PS_LIST_TAIL, "-sky_id", 0, "search by staticsky ID", 0);
    psMetadataAddStr(todoArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    pxskycellAddArguments(todoArgs);
    psMetadataAddU64(todoArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(todoArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -addresult
    psMetadata *addresultArgs = psMetadataAlloc();
    psMetadataAddS64(addresultArgs, PS_LIST_TAIL, "-sky_id", 0, "define sky ID (required)", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location", 0);
    psMetadataAddF32(addresultArgs, PS_LIST_TAIL, "-dtime_phot", 0, "define photometry time", NAN);
    psMetadataAddF32(addresultArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddS32(addresultArgs, PS_LIST_TAIL, "-sources", 0, "number of sources", 0);
    psMetadataAddS32(addresultArgs, PS_LIST_TAIL, "-num_inputs", 0, "number of inputs", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", 0);
    psMetadataAddF32(addresultArgs, PS_LIST_TAIL, "-good_frac", 0, "define fraction of good pixels", NAN);
    psMetadataAddS16(addresultArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddS16(addresultArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -result
    psMetadata *resultArgs= psMetadataAlloc();
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-sky_id", 0, "search by staticsky ID", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID (LIKE comparison)", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID (LIKE comparison)", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-label", 0, "search by label", NULL);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", NULL);
    psMetadataAddS16(resultArgs, PS_LIST_TAIL, "-num_filters", 0, "search by number of filters in the inputs", 0);
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-stack_id", 0, "search by input stack ID (if used num_filters will be wrong)", 0);
    pxskycellAddArguments(resultArgs);
    psMetadataAddS16(resultArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddU64(resultArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(resultArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(resultArgs, PS_LIST_TAIL, "-all", 0, "allow no search terms", false);

    // -revert
    psMetadata *revertArgs= psMetadataAlloc();
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-sky_id", 0, "search by staticsky ID", 0);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddS16(revertArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    pxskycellAddArguments(revertArgs);
    psMetadataAddBool(revertArgs, PS_LIST_TAIL, "-all", 0, "allow no search terms", false);

    // -updateresult
    psMetadata *updateresultArgs = psMetadataAlloc();
    psMetadataAddS64(updateresultArgs, PS_LIST_TAIL, "-sky_id", 0, "define staticksky ID (required)", 0);
    psMetadataAddS16(updateresultArgs, PS_LIST_TAIL, "-fault", 0, "set fault code (required)", 0);
    psMetadataAddS16(updateresultArgs, PS_LIST_TAIL, "-set_quality",  0, "set quality", 0);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-sky_id", 0, "export this staticsky ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0, "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean", 0, "mark tables as cleaned", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile", 0, "import from this file (required)", NULL);
  
    // -defineskycalrun
    psMetadata *defineskycalrunArgs = psMetadataAlloc();
    psMetadataAddS64(defineskycalrunArgs,  PS_LIST_TAIL, "-select_sky_id", 0, "search for sky_id", 0);
    psMetadataAddS64(defineskycalrunArgs,  PS_LIST_TAIL, "-select_stack_id", 0, "search for stack_id", 0);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-select_skycell_id", 0, "search for skycell_id", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-select_tess_id", 0, "search for tess_id", NULL);
    psMetadataAddF32(defineskycalrunArgs,  PS_LIST_TAIL, "-select_good_frac_min", 0, "define min good_frac", NAN);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-select_label", PS_META_DUPLICATE_OK, "search by stackRun label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-select_data_group", PS_META_DUPLICATE_OK, "search by stackRun data_group (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-select_filter", PS_META_DUPLICATE_OK, "search by filter (LIKE comparison, multiple OK)", NULL);
    pxskycellAddArguments(defineskycalrunArgs);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-set_workdir", 0, "define workdir", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(defineskycalrunArgs,  PS_LIST_TAIL, "-set_reduction", 0, "define reduction", NULL);
    psMetadataAddTime(defineskycalrunArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddBool(defineskycalrunArgs, PS_LIST_TAIL, "-rerun",  0, "queue new run even if one already exists for inputs", false);
    psMetadataAddBool(defineskycalrunArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(defineskycalrunArgs, PS_LIST_TAIL, "-check_inputs",  0, "list inputs, do not modify database", false);
    psMetadataAddBool(defineskycalrunArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddU64(defineskycalrunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);

    // -updateskycalrun
    psMetadata *updateskycalrunArgs = psMetadataAlloc();
    psMetadataAddS64(updateskycalrunArgs, PS_LIST_TAIL, "-skycal_id", 0, "search by skycal ID", 0);
    psMetadataAddS64(updateskycalrunArgs, PS_LIST_TAIL, "-sky_id", 0,    "search by sky ID", 0);
    psMetadataAddS64(updateskycalrunArgs, PS_LIST_TAIL, "-stack_id", 0,  "search by stack ID", 0);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", 0);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-label", 0, "search by label", 0);
    pxskycellAddArguments(updateskycalrunArgs);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-set_label", 0, "define new value for label", NULL);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-set_data_group", 0, "define new data_group", NULL);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define new dist_group", NULL);
    psMetadataAddStr(updateskycalrunArgs, PS_LIST_TAIL, "-set_note", 0, "define new note", NULL);

    // -pendingskycalrun
    psMetadata *pendingskycalrunArgs = psMetadataAlloc();
    psMetadataAddS64(pendingskycalrunArgs, PS_LIST_TAIL, "-skycal_id", 0, "search by skycal ID", 0);
    psMetadataAddS64(pendingskycalrunArgs, PS_LIST_TAIL, "-sky_id", 0, "search by staticsky ID", 0);
    psMetadataAddStr(pendingskycalrunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddStr(pendingskycalrunArgs, PS_LIST_TAIL, "-filter", 0, "search by filter", NULL);
    psMetadataAddU64(pendingskycalrunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(pendingskycalrunArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);


    // -addskycalresult
    psMetadata *addskycalresultArgs = psMetadataAlloc();
    psMetadataAddS64(addskycalresultArgs, PS_LIST_TAIL, "-skycal_id", 0,            "define camtool ID (required)", 0);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-sigma_ra", 0,            "define exposure E ra", NAN);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-sigma_dec", 0,            "define exposure E dec", NAN);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-zpt_obs", 0,   "define observed zero point", NAN);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-zpt_stdev", 0,   "define observed zero point stdandard deviation", NAN);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-fwhm_major", 0,   "define fwhm (major axis; pixels)", NAN);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-fwhm_minor", 0,   "define fwhm (minor axis; pixels)", NAN);

    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddF32(addskycalresultArgs, PS_LIST_TAIL, "-dtime_astrom", 0, "define elapsed time for astrometry (seconds)", NAN);

    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-hostname", 0,            "define hostname", NULL);
    // psMetadataAddS32(addskycalresultArgs, PS_LIST_TAIL, "-n_stars", 0,            "define number of stars", 0);
    psMetadataAddS32(addskycalresultArgs, PS_LIST_TAIL, "-n_astrom", 0,            "define number of astrometry reference objects", 0);
    psMetadataAddS32(addskycalresultArgs, PS_LIST_TAIL, "-n_detections", 0,        "define number of detections", 0);
    psMetadataAddS32(addskycalresultArgs, PS_LIST_TAIL, "-n_extended", 0,          "define number of extended detections", 0);
    psMetadataAddS32(addskycalresultArgs, PS_LIST_TAIL, "-n_forced", 0,            "define number of forced detections", 0);

    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-path_base", 0,            "define base output location", NULL);
    psMetadataAddS16(addskycalresultArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddS16(addskycalresultArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);

    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-ver_psastro", 0, "define psastro version", NULL);
    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(addskycalresultArgs, PS_LIST_TAIL, "-ver_streaks", 0, "define streaksremove version", NULL);

    // -revertskycal
    psMetadata *revertskycalArgs= psMetadataAlloc();
    psMetadataAddS64(revertskycalArgs, PS_LIST_TAIL, "-skycal_id", 0, "search by skycal ID", 0);
    psMetadataAddStr(revertskycalArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddStr(revertskycalArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by data_group", 0);
    psMetadataAddStr(revertskycalArgs, PS_LIST_TAIL, "-filter", PS_META_DUPLICATE_OK, "search by filter", 0);
    pxskycellAddArguments(revertskycalArgs);
    psMetadataAddS16(revertskycalArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddBool(revertskycalArgs, PS_LIST_TAIL, "-all", 0, "allow no search terms", false);

    // -updateskycalresult
    psMetadata *updateskycalresultArgs = psMetadataAlloc();
    psMetadataAddS64(updateskycalresultArgs, PS_LIST_TAIL, "-skycal_id", 0, "define staticksky ID (required)", 0);
    psMetadataAddS16(updateskycalresultArgs, PS_LIST_TAIL, "-set_fault", 0, "set fault code (required)", 0);
    psMetadataAddS16(updateskycalresultArgs, PS_LIST_TAIL, "-set_quality", 0, "set quality code", 0);

    // -skycalresult
    psMetadata *skycalresultArgs= psMetadataAlloc();
    psMetadataAddS64(skycalresultArgs, PS_LIST_TAIL, "-skycal_id", 0, "search by skycal ID", 0);
    psMetadataAddS64(skycalresultArgs, PS_LIST_TAIL, "-sky_id", 0, "search by staticsky ID", 0);
    psMetadataAddS64(skycalresultArgs, PS_LIST_TAIL, "-stack_id", 0, "search by stack ID", 0);
    psMetadataAddStr(skycalresultArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID (LIKE comparison)", 0);
    psMetadataAddStr(skycalresultArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID (LIKE comparison)", 0);
    psMetadataAddStr(skycalresultArgs, PS_LIST_TAIL, "-filter", 0, "search by filter (LIKE comparison)", 0);
    psMetadataAddStr(skycalresultArgs, PS_LIST_TAIL, "-label", 0, "search by label (LIKE comparison)", NULL);
    psMetadataAddStr(skycalresultArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", NULL);
    psMetadataAddS16(skycalresultArgs, PS_LIST_TAIL, "-quality",  0, "search by quality", 0);
    psMetadataAddS16(skycalresultArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    pxskycellAddArguments(skycalresultArgs);
    psMetadataAddU64(skycalresultArgs, PS_LIST_TAIL, "-limit", 0, "limit skycalresult set to N items", 0);
    psMetadataAddBool(skycalresultArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -exportskycalrun
    psMetadata *exportskycalrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportskycalrunArgs, PS_LIST_TAIL, "-skycal_id", 0, "export this skycal ID (required)", 0);
    psMetadataAddStr(exportskycalrunArgs, PS_LIST_TAIL, "-outfile", 0, "export to this file (required)", NULL);
    psMetadataAddU64(exportskycalrunArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(exportskycalrunArgs, PS_LIST_TAIL, "-clean", 0, "mark tables as cleaned", false);

    // -importrun
    psMetadata *importskycalrunArgs = psMetadataAlloc();
    psMetadataAddStr(importskycalrunArgs, PS_LIST_TAIL, "-infile", 0, "import from this file (required)", NULL);
  

    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery", "Define a new run",     STATICSKYTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-updaterun",     "Update a run",         STATICSKYTOOL_MODE_UPDATERUN,     updaterunArgs);
    PXOPT_ADD_MODE("-inputs",        "Get inputs",           STATICSKYTOOL_MODE_INPUTS,        inputsArgs);
    PXOPT_ADD_MODE("-todo",          "Get runs to do",       STATICSKYTOOL_MODE_TODO,          todoArgs);
    PXOPT_ADD_MODE("-addresult",     "Add result of run",    STATICSKYTOOL_MODE_ADDRESULT,     addresultArgs);
    PXOPT_ADD_MODE("-result",        "Get result of run",    STATICSKYTOOL_MODE_RESULT,        resultArgs);
    PXOPT_ADD_MODE("-revert",        "Revert failed run",    STATICSKYTOOL_MODE_REVERT,        revertArgs);
    PXOPT_ADD_MODE("-updateresult",  "Update fault for run", STATICSKYTOOL_MODE_UPDATERESULT,  updateresultArgs);
    PXOPT_ADD_MODE("-exportrun",     "Export run",           STATICSKYTOOL_MODE_EXPORTRUN,     exportrunArgs);
    PXOPT_ADD_MODE("-importrun",     "Import run",           STATICSKYTOOL_MODE_IMPORTRUN,     importrunArgs);
    PXOPT_ADD_MODE("-defineskycalrun", "Define a new skycalrun", STATICSKYTOOL_MODE_DEFINESKYCALRUN, defineskycalrunArgs);
    PXOPT_ADD_MODE("-updateskycalrun", "Update a skycalrun", STATICSKYTOOL_MODE_UPDATESKYCALRUN,     updateskycalrunArgs);
    PXOPT_ADD_MODE("-pendingskycalrun", "Get skcal runs to do",       STATICSKYTOOL_MODE_PENDINGSKYCALRUN,          pendingskycalrunArgs);
    PXOPT_ADD_MODE("-addskycalresult", "add skycal result",       STATICSKYTOOL_MODE_ADDSKYCALRESULT, addskycalresultArgs);
    PXOPT_ADD_MODE("-revertskycal",  "revert faulted skycal run", STATICSKYTOOL_MODE_REVERTSKYCALRESULT, revertskycalArgs);
    PXOPT_ADD_MODE("-updateskycal",  "revert faulted skycal run", STATICSKYTOOL_MODE_UPDATESKYCALRESULT, updateskycalresultArgs);
    PXOPT_ADD_MODE("-skycalresult",  "Get result of skycal run",  STATICSKYTOOL_MODE_SKYCALRESULT, skycalresultArgs);
    PXOPT_ADD_MODE("-exportskycalrun",     "Export skycal run",   STATICSKYTOOL_MODE_EXPORTSKYCALRUN, exportskycalrunArgs);
    PXOPT_ADD_MODE("-importskycalrun",     "Import skycal run",   STATICSKYTOOL_MODE_IMPORTSKYCALRUN, importskycalrunArgs);

    if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
        psError(PS_ERR_UNKNOWN, true, "option parsing failed");
        psFree(argSets);
        psFree(modes);
        psFree(config);
        return NULL;
    }

    psFree(argSets);
    psFree(modes);

    // define Database handle, if used
    // do this last so we don't setup a connection before CLI options are
    // validated
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}
