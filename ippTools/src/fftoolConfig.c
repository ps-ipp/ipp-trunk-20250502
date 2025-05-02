/*
 * fftool.c
 *
 * Copyright (C) 2013 Institute for Astronomy, University of Hawaii
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
#include "fftool.h"

pxConfig *fftoolConfig(pxConfig *config, int argc, char **argv)
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
    (void) now;

    // -definebyquery
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_warp_label", PS_META_DUPLICATE_OK, "search by warp label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_skycal_label", PS_META_DUPLICATE_OK, "search by skycal label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_warp_data_group", PS_META_DUPLICATE_OK, "search by warp data_group (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_skycal_data_group", PS_META_DUPLICATE_OK, "search by skycal data_group (LIKE comparison, multiple OK)", NULL);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-select_warp_id", 0, "search by warp ID", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-select_skycal_id", 0, "search by skycal ID", 0);

    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_skycell_id", 0, "search for skycell_id (LIKE comparision)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_tess_id", 0, "search for tess_id", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-select_filter", PS_META_DUPLICATE_OK, "search by filter (LIKE comparison, multiple OK)", NULL);
    psMetadataAddF32(definebyqueryArgs,  PS_LIST_TAIL, "-select_good_frac_min", 0, "mimimum good_frac in warp", 0.0);

    pxskycellAddArguments(definebyqueryArgs);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_sources_path_base", 0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_label", 0, "define label (required)", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(definebyqueryArgs,  PS_LIST_TAIL, "-set_reduction", 0, "define reduction", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-rerun",  0, "queue new run even if one already exists for inputs", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -defineforstacks
    psMetadata *defineforstacksArgs = psMetadataAlloc();
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-select_skycal_label", PS_META_DUPLICATE_OK, "search by skycal label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-select_skycal_data_group", PS_META_DUPLICATE_OK, "search by skycal data_group (LIKE comparison, multiple OK)", NULL);
    psMetadataAddS64(defineforstacksArgs, PS_LIST_TAIL, "-select_skycal_id", 0, "search by skycal ID", 0);

    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-select_skycell_id", 0, "search for skycell_id (LIKE comparision)", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-select_tess_id", 0, "search for tess_id", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-select_filter", PS_META_DUPLICATE_OK, "search by filter (LIKE comparison, multiple OK)", NULL);
    psMetadataAddF32(defineforstacksArgs,  PS_LIST_TAIL, "-select_good_frac_min", 0, "mimimum good_frac in warp", 0.0);

    pxskycellAddArguments(defineforstacksArgs);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_sources_path_base", 0, "define workdir", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_label", 0, "define label (required)", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_dist_group", 0, "define dist group", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(defineforstacksArgs,  PS_LIST_TAIL, "-set_reduction", 0, "define reduction", NULL);
    psMetadataAddBool(defineforstacksArgs, PS_LIST_TAIL, "-rerun",  0, "queue new run even if one already exists for inputs", false);
    psMetadataAddBool(defineforstacksArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(defineforstacksArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0, "search by state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0, "search by label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", NULL);
    pxskycellAddArguments(updaterunArgs);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0, "define new value for label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0, "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_dist_group", 0, "define new dist_group", NULL);
    psMetadataAddStr(updaterunArgs,  PS_LIST_TAIL, "-set_note", 0, "define note", NULL);

    // -todo
    psMetadata *todoArgs = psMetadataAlloc();
    psMetadataAddS64(todoArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddStr(todoArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    pxskycellAddArguments(todoArgs);
    psMetadataAddU64(todoArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(todoArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -addresult
    psMetadata *addresultArgs = psMetadataAlloc();
    psMetadataAddS64(addresultArgs, PS_LIST_TAIL, "-ff_id", 0, "define full force ID (required)", 0);
    psMetadataAddS64(addresultArgs, PS_LIST_TAIL, "-warp_id", 0, "define warp ID (required)", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location(required)", 0);
    psMetadataAddF32(addresultArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddS16(addresultArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-software_ver", 0, "define software version", 0);
    psMetadataAddS16(addresultArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -result
    psMetadata *resultArgs= psMetadataAlloc();
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp ID", 0);
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exposure ID", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-exp_name", 0, "search by exposure name", 0);
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-skycal_id", 0, "search by skycal ID", 0);
    psMetadataAddS64(resultArgs, PS_LIST_TAIL, "-stack_id", 0, "search by stack ID", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-filter", 0, "search by filter (LIKE comparison)", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID (LIKE comparison)", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID (LIKE comparison)", 0);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-label", 0, "search by label", NULL);
    psMetadataAddStr(resultArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", NULL);
    psMetadataAddS16(resultArgs, PS_LIST_TAIL, "-quality", 0, "search by quality value", 0);
    pxskycellAddArguments(resultArgs);
    psMetadataAddS16(resultArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddU64(resultArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(resultArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -revert
    psMetadata *revertArgs= psMetadataAlloc();
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp ID", 0);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddS16(revertArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    pxskycellAddArguments(revertArgs);

    // -updateresult
    psMetadata *updateresultArgs = psMetadataAlloc();
    psMetadataAddS64(updateresultArgs, PS_LIST_TAIL, "-ff_id", 0, "define full force ID (required)", 0);
    psMetadataAddS64(updateresultArgs, PS_LIST_TAIL, "-warp_id", 0, "define warp ID (required)", 0);
    psMetadataAddS16(updateresultArgs, PS_LIST_TAIL, "-fault", 0, "set fault code (required)", 0);
    psMetadataAddS16(updateresultArgs, PS_LIST_TAIL, "-quality", 0, "set quality code", 0);

    // -toadvance
    psMetadata *toadvanceArgs = psMetadataAlloc();
    psMetadataAddS64(toadvanceArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    pxskycellAddArguments(toadvanceArgs);
    psMetadataAddU64(toadvanceArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(toadvanceArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -addsummary
    psMetadata *addsummaryArgs = psMetadataAlloc();
    psMetadataAddS64(addsummaryArgs, PS_LIST_TAIL, "-ff_id", 0, "define full force ID (required)", 0);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location(required)", 0);
    psMetadataAddF32(addsummaryArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddS16(addsummaryArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", 0);
    psMetadataAddStr(addsummaryArgs, PS_LIST_TAIL, "-software_ver", 0, "define software version", 0);
    psMetadataAddS16(addsummaryArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -updatesummary
    psMetadata *updatesummaryArgs = psMetadataAlloc();
    psMetadataAddS64(updatesummaryArgs, PS_LIST_TAIL, "-ff_id", 0, "define full force ID (required)", 0);
    psMetadataAddStr(updatesummaryArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location", 0);
    psMetadataAddF32(updatesummaryArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddS16(updatesummaryArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddStr(updatesummaryArgs, PS_LIST_TAIL, "-hostname", 0, "define hostname", 0);
    psMetadataAddStr(updatesummaryArgs, PS_LIST_TAIL, "-software_ver", 0, "define software version", 0);
    psMetadataAddS16(updatesummaryArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -summary
    psMetadata *summaryArgs= psMetadataAlloc();
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-warp_id", 0, "search by warp ID", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-exp_id", 0, "search by exposure ID", 0);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-exp_name", 0, "search by exposure name", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-skycal_id", 0, "search by skycal ID", 0);
    psMetadataAddS64(summaryArgs, PS_LIST_TAIL, "-stack_id", 0, "search by stack ID", 0);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-filter", 0, "search by filter (LIKE comparison)", 0);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-tess_id", 0, "search by tess ID (LIKE comparison)", 0);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID (LIKE comparison)", 0);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-label", 0, "search by label", NULL);
    psMetadataAddStr(summaryArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", NULL);
    pxskycellAddArguments(summaryArgs);
    psMetadataAddS16(summaryArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddU64(summaryArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(summaryArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -revertsummary
    psMetadata *revertsummaryArgs= psMetadataAlloc();
    psMetadataAddS64(revertsummaryArgs, PS_LIST_TAIL, "-ff_id", 0, "search by full force ID", 0);
    psMetadataAddStr(revertsummaryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddS16(revertsummaryArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    pxskycellAddArguments(revertsummaryArgs);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-ff_id", 0,          "export this full force ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);
//    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "export run in cleaned state", false);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);



    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery", "Define a new full force run",     FFTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-defineforstacks", "Define a new full force runs for inputs to stacks",     FFTOOL_MODE_DEFINEFORSTACKS, defineforstacksArgs);
    PXOPT_ADD_MODE("-updaterun",     "Update a run",         FFTOOL_MODE_UPDATERUN,     updaterunArgs);
    PXOPT_ADD_MODE("-todo",          "Get runs to do",       FFTOOL_MODE_TODO,          todoArgs);
    PXOPT_ADD_MODE("-addresult",     "Add result for fullforce run on a warp",    FFTOOL_MODE_ADDRESULT,     addresultArgs);
    PXOPT_ADD_MODE("-result",        "Get result fullforce run on a warp",    FFTOOL_MODE_RESULT,        resultArgs);
    PXOPT_ADD_MODE("-revert",        "Revert failed fullforce run on a warp",    FFTOOL_MODE_REVERT,        revertArgs);
    PXOPT_ADD_MODE("-updateresult",  "Update result for fullforce run on a warp", FFTOOL_MODE_UPDATERESULT,  updateresultArgs);
    PXOPT_ADD_MODE("-toadvance",     "list completed runs to summarize", FFTOOL_MODE_TOADVANCE,  toadvanceArgs);
    PXOPT_ADD_MODE("-addsummary",    "insert summary result", FFTOOL_MODE_ADDSUMMARY,  addsummaryArgs);
    PXOPT_ADD_MODE("-updatesummary", "update summary result", FFTOOL_MODE_UPDATESUMMARY,  updatesummaryArgs);
    PXOPT_ADD_MODE("-revertsummary", "revert faulted summary", FFTOOL_MODE_REVERTSUMMARY,  revertsummaryArgs);
    PXOPT_ADD_MODE("-summary",       "list summary results", FFTOOL_MODE_SUMMARY,  summaryArgs);
    PXOPT_ADD_MODE("-exportrun",     "list summary results", FFTOOL_MODE_EXPORTRUN,  exportrunArgs);
    PXOPT_ADD_MODE("-importrun",     "list summary results", FFTOOL_MODE_IMPORTRUN,  importrunArgs);

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
