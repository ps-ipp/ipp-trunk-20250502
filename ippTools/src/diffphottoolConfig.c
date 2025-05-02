/*
 * diffphottoolConfig.c
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
#include "diffphottool.h"

pxConfig *diffphottoolConfig(pxConfig *config, int argc, char **argv)
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

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_workdir", 0, "define workdir (required)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_label", 0, "define label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_reduction", 0, "define reduction class", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-set_registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_data_group", 0, "define data group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-set_note", 0, "define note", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label (LIKE comparison, multiple OK)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-data_group", PS_META_DUPLICATE_OK, "search by data_group", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-comment", 0, "search for comment (LIKE)", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-filter", 0, "search for filter", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-dateobs_begin", 0, "search for exposures by time (>=)", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-dateobs_end", 0, "search for exposures by time (<)", NULL);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-pretend",  0, "do not actually modify the database", false);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "search by diffphot ID", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0, "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label", 0, "search by label (LIKE comparison)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group", 0, "search by data_group (LIKE comparison)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0, "define new value for label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "define new state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0, "define new data_group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0, "define new note", NULL);

    // -input
    psMetadata *inputArgs = psMetadataAlloc();
    psMetadataAddS64(inputArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "search by diffphot ID", 0);
    psMetadataAddStr(inputArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID", NULL);
    psMetadataAddU64(inputArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(inputArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -pending
    psMetadata *pendingArgs = psMetadataAlloc();
    psMetadataAddS64(pendingArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "search by diffphot ID", 0);
    psMetadataAddStr(pendingArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", 0);
    psMetadataAddU64(pendingArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(pendingArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -done
    psMetadata *doneArgs = psMetadataAlloc();
    psMetadataAddS64(doneArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "define diffphot ID (required)", 0);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-skycell_id", 0, "define skycell of file (required)", NULL);
    psMetadataAddS16(doneArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);
    psMetadataAddS16(doneArgs, PS_LIST_TAIL, "-quality", 0, "set quality", 0);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-path_base", 0, "define base output location (required)", NULL);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-hostname", 0, "set hostname (required)", NULL);
    psMetadataAddF32(doneArgs, PS_LIST_TAIL, "-dtime_script", 0, "define elapsed time in script (seconds)", NAN);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-ver_pslib", 0, "define psLib version", NULL);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-ver_psmodules", 0, "define psModules version", NULL);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-ver_ppstats", 0, "define ppStats version", NULL);
    psMetadataAddStr(doneArgs, PS_LIST_TAIL, "-ver_psphot", 0, "define psphot version", NULL);
    psMetadataAddS64(doneArgs, PS_LIST_TAIL, "-magicked", 0, "define magicked state", 0);

    // -advance
    psMetadata *advanceArgs = psMetadataAlloc();
    psMetadataAddS64(advanceArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "select by diffphot ID", 0);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "select by label", NULL);
    psMetadataAddS32(advanceArgs, PS_LIST_TAIL, "-limit", 0, "limit number of results", 0);

    // -revert
    psMetadata *revertArgs = psMetadataAlloc();
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "search by diffphot ID", 0);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell_id", NULL);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS16(revertArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddBool(revertArgs, PS_LIST_TAIL, "-all", 0, "allow no search terms", 0);

    // -data
    psMetadata *dataArgs = psMetadataAlloc();
    psMetadataAddS64(dataArgs, PS_LIST_TAIL, "-diff_phot_id", 0, "search by diffphot ID", 0);
    psMetadataAddStr(dataArgs, PS_LIST_TAIL, "-skycell_id", 0, "search by skycell ID", NULL);
    psMetadataAddU64(dataArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(dataArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definerun", "", DIFFPHOTTOOL_MODE_DEFINERUN, definerunArgs);
    PXOPT_ADD_MODE("-updaterun", "", DIFFPHOTTOOL_MODE_UPDATERUN, updaterunArgs);
    PXOPT_ADD_MODE("-input",     "", DIFFPHOTTOOL_MODE_INPUT,     inputArgs);
    PXOPT_ADD_MODE("-pending",   "", DIFFPHOTTOOL_MODE_PENDING,   pendingArgs);
    PXOPT_ADD_MODE("-done",      "", DIFFPHOTTOOL_MODE_DONE,      doneArgs);
    PXOPT_ADD_MODE("-advance",   "", DIFFPHOTTOOL_MODE_ADVANCE,   advanceArgs);
    PXOPT_ADD_MODE("-revert",    "", DIFFPHOTTOOL_MODE_REVERT,    revertArgs);
    PXOPT_ADD_MODE("-data",      "", DIFFPHOTTOOL_MODE_DATA,      dataArgs);

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
