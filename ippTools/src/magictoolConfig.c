/*
 * magictoolConfig.c
 *
 * Copyright (C) 2007  Joshua Hoblitt
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
#include "magictool.h"

pxConfig *magictoolConfig(pxConfig *config, int argc, char **argv)
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
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-workdir",     0, "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label",       0, "define label (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-dvodb",       0, "define dvodb", NULL);
    psMetadataAddTime(definebyqueryArgs, PS_LIST_TAIL, "-registered", 0, "time detrend run was registered", now);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-data_group",  0, "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-note",  0, "define note", NULL);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-diff_id", 0, "search diff_id", 0);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-exp_id", 0, "search exp_id", 0);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-diff_label", 0, "select diff label", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-select_filter", 0, "select filter", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-available", 0, "process what's immediately available?", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-rerun", 0, "generate new run even if existing?", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-multidiff", 0, "allow multiple diffs to be magicked with the same label", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend", 0, "list results but do not queue", false);

    // -definerun
    psMetadata *definerunArgs = psMetadataAlloc();
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-workdir", 0, "define workdir (required)", NULL);
    psMetadataAddS64(definerunArgs, PS_LIST_TAIL, "-exp_id", 0, "define exp_id (required)", 0);
    // following argument is for compatability
    psMetadataAddS64(definerunArgs, PS_LIST_TAIL, "-diff_id", 0, "define diff_id", 0);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-inverse", 0, "use the inverse subtraction?", false);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-label", 0, "define label", NULL);
    psMetadataAddStr(definerunArgs, PS_LIST_TAIL, "-dvodb", 0, "define dvodb", NULL);
    psMetadataAddTime(definerunArgs, PS_LIST_TAIL, "-registered", 0, "time detrend run was registered", now);
    psMetadataAddBool(definerunArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0, "set state (required)", NULL);
    psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-set_fault", 0, "set fault code", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0, "define label", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note",  0, "define note", NULL);
    psMetadataAddBool(updaterunArgs, PS_LIST_TAIL, "-clearfault",  0, "set fault to zero", NULL);

    // -addinputskyfile
    psMetadata *addinputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(addinputskyfileArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(addinputskyfileArgs, PS_LIST_TAIL, "-node", 0, "define symbolic node name (required)", NULL);

    // -inputskyfile
    psMetadata *inputskyfileArgs = psMetadataAlloc();
    psMetadataAddS64(inputskyfileArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magictool ID", 0);
    psMetadataAddS64(inputskyfileArgs, PS_LIST_TAIL, "-diff_id", 0, "search by difftool ID", 0);
    psMetadataAddStr(inputskyfileArgs, PS_LIST_TAIL, "-node", 0, "search by symbolic node name", NULL);
    psMetadataAddU64(inputskyfileArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(inputskyfileArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -totree
    psMetadata *totreeArgs = psMetadataAlloc();
    psMetadataAddS64(totreeArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddU64(totreeArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddStr(totreeArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddBool(totreeArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -inputtree
    psMetadata *inputtreeArgs = psMetadataAlloc();
    psMetadataAddS64(inputtreeArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(inputtreeArgs, PS_LIST_TAIL, "-dep_file", 0, "order of operations dep. file", NULL);
    psMetadataAddS16(inputtreeArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -reverttree
    psMetadata *reverttreeArgs = psMetadataAlloc();
    psMetadataAddS64(reverttreeArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magictool ID", 0);
    psMetadataAddS16(reverttreeArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);
    psMetadataAddStr(reverttreeArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);

    // -inputs
    psMetadata *inputsArgs = psMetadataAlloc();
    psMetadataAddS64(inputsArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magictool ID", 0);
    psMetadataAddStr(inputsArgs, PS_LIST_TAIL, "-node", 0, "search by symbolic node name", NULL);
    psMetadataAddU64(inputsArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(inputsArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -tooprocess
    psMetadata *toprocessArgs = psMetadataAlloc();
    psMetadataAddS64(toprocessArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddU64(toprocessArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddStr(toprocessArgs, PS_LIST_TAIL, "-label",       PS_META_DUPLICATE_OK, "define label", NULL);
    psMetadataAddBool(toprocessArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -addresult
    psMetadata *addresultArgs = psMetadataAlloc();
    psMetadataAddS64(addresultArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-node", 0, "define symbolic node name (required)", NULL);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-path_base", 0, "define path_base", NULL);
    psMetadataAddS16(addresultArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -revertnode
    psMetadata *revertnodeArgs = psMetadataAlloc();
    psMetadataAddS64(revertnodeArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magictool ID", 0);
    psMetadataAddStr(revertnodeArgs, PS_LIST_TAIL, "-node", 0, "search by node name", NULL);
    psMetadataAddStr(revertnodeArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddS16(revertnodeArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);

    // -tomask
    psMetadata *tomaskArgs = psMetadataAlloc();
    psMetadataAddU64(tomaskArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(tomaskArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -addmask
    psMetadata *addmaskArgs = psMetadataAlloc();
    psMetadataAddS64(addmaskArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID (required)", 0);
    psMetadataAddStr(addmaskArgs, PS_LIST_TAIL, "-path_base", 0, "define path_base (required)", NULL);
    psMetadataAddS32(addmaskArgs, PS_LIST_TAIL, "-streaks", 0, "define number of streaks", 0);
    psMetadataAddS16(addmaskArgs, PS_LIST_TAIL, "-fault", 0, "set fault code", 0);

    // -revertmask
    psMetadata *revertmaskArgs = psMetadataAlloc();
    psMetadataAddS64(revertmaskArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magictool ID", 0);
    psMetadataAddS16(revertmaskArgs, PS_LIST_TAIL, "-fault", 0, "search by fault code", 0);

    // -mask
    psMetadata *maskArgs = psMetadataAlloc();
    psMetadataAddS64(maskArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID", 0);
    psMetadataAddU64(maskArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(maskArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -censorun
    psMetadata *censorrunArgs = psMetadataAlloc();
    psMetadataAddS64(censorrunArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID", 0);
    psMetadataAddS64(censorrunArgs, PS_LIST_TAIL, "-exp_id", 0, "define exposure ID", 0);
    psMetadataAddStr(censorrunArgs, PS_LIST_TAIL, "-label",       0, "define label", NULL);

    // -exposure
    psMetadata *exposureArgs = psMetadataAlloc();
    psMetadataAddS64(exposureArgs, PS_LIST_TAIL, "-magic_id", 0, "define magictool ID", 0);
    psMetadataAddBool(exposureArgs, PS_LIST_TAIL, "-inverse", 0, "select the inverse subtraction?", false);
    psMetadataAddBool(exposureArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -setgotocleaned
    psMetadata *setgotocleanedArgs = psMetadataAlloc();
    psMetadataAddS64(setgotocleanedArgs, PS_LIST_TAIL, "-magic_id", 0, "select by magictool ID", 0);
    psMetadataAddS64(setgotocleanedArgs, PS_LIST_TAIL, "-exp_id", 0, "select by exposure ID)", 0);
    psMetadataAddStr(setgotocleanedArgs, PS_LIST_TAIL, "-label",  0, "select by label", NULL);
    psMetadataAddStr(setgotocleanedArgs, PS_LIST_TAIL, "-data_group",  0, "select by data_group", NULL);
    psMetadataAddStr(setgotocleanedArgs, PS_LIST_TAIL, "-set_label",  0, "set new label", NULL);
    psMetadataAddBool(setgotocleanedArgs, PS_LIST_TAIL, "-clearfault",  0, "clear cleanup errors", NULL);

    // -tocleanup
    psMetadata *tocleanupArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanupArgs, PS_LIST_TAIL, "-magic_id", 0, "search by magic ID", 0);
    psMetadataAddStr(tocleanupArgs, PS_LIST_TAIL, "-label",    PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddStr(tocleanupArgs, PS_LIST_TAIL, "-data_group",    PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddU64(tocleanupArgs, PS_LIST_TAIL, "-limit", 0, "limit result set to N items", 0);
    psMetadataAddBool(tocleanupArgs, PS_LIST_TAIL, "-simple", 0, "use the simple output format", false);

    // -setworkdirstate
    psMetadata *setworkdirstateArgs = psMetadataAlloc();
    psMetadataAddS64(setworkdirstateArgs, PS_LIST_TAIL, "-magic_id", 0, "select by magictool ID (required)", 0);
    psMetadataAddStr(setworkdirstateArgs, PS_LIST_TAIL, "-set_workdir_state", 0, "new workdir_state (required)", 0);

    psFree(now);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes   = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",       "", MAGICTOOL_MODE_DEFINEBYQUERY,       definebyqueryArgs);
    PXOPT_ADD_MODE("-definerun",           "", MAGICTOOL_MODE_DEFINERUN,           definerunArgs);
    PXOPT_ADD_MODE("-updaterun",           "", MAGICTOOL_MODE_UPDATERUN,           updaterunArgs);
    PXOPT_ADD_MODE("-addinputskyfile",     "", MAGICTOOL_MODE_ADDINPUTSKYFILE,     addinputskyfileArgs);
    PXOPT_ADD_MODE("-inputskyfile",        "", MAGICTOOL_MODE_INPUTSKYFILE,        inputskyfileArgs);
    PXOPT_ADD_MODE("-totree",              "", MAGICTOOL_MODE_TOTREE,              totreeArgs);
    PXOPT_ADD_MODE("-inputtree",           "", MAGICTOOL_MODE_INPUTTREE,           inputtreeArgs);
    PXOPT_ADD_MODE("-reverttree",          "", MAGICTOOL_MODE_REVERTTREE,          reverttreeArgs);
    PXOPT_ADD_MODE("-toprocess",           "", MAGICTOOL_MODE_TOPROCESS,           toprocessArgs);
    PXOPT_ADD_MODE("-inputs",              "", MAGICTOOL_MODE_INPUTS,              inputsArgs);
    PXOPT_ADD_MODE("-addresult",           "", MAGICTOOL_MODE_ADDRESULT,           addresultArgs);
    PXOPT_ADD_MODE("-revertnode",          "", MAGICTOOL_MODE_REVERTNODE,          revertnodeArgs);
    PXOPT_ADD_MODE("-tomask",              "", MAGICTOOL_MODE_TOMASK,              tomaskArgs);
    PXOPT_ADD_MODE("-addmask",             "", MAGICTOOL_MODE_ADDMASK,             addmaskArgs);
    PXOPT_ADD_MODE("-revertmask",          "", MAGICTOOL_MODE_REVERTMASK,          revertmaskArgs);
    PXOPT_ADD_MODE("-mask",                "", MAGICTOOL_MODE_MASK,                maskArgs);
    PXOPT_ADD_MODE("-censorrun",           "", MAGICTOOL_MODE_CENSORRUN,           censorrunArgs);
    PXOPT_ADD_MODE("-exposure",            "", MAGICTOOL_MODE_EXPOSURE,            exposureArgs);
    PXOPT_ADD_MODE("-setgotocleaned",      "", MAGICTOOL_MODE_SETGOTOCLEANED,      setgotocleanedArgs);
    PXOPT_ADD_MODE("-tocleanup",           "", MAGICTOOL_MODE_TOCLEANUP,           tocleanupArgs);
    PXOPT_ADD_MODE("-setworkdirstate",     "", MAGICTOOL_MODE_SETWORKDIRSTATE,     setworkdirstateArgs);

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
