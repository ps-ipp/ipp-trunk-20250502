/*
 * vptoolConfig.c
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
#include "vptool.h"

pxConfig *vptoolConfig(pxConfig *config, int argc, char **argv)
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
    // allow selecting by all of the args used to queue chipRuns
    pxchipSetSearchArgs (definebyqueryArgs);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by newExp label (LIKE comparison)", NULL);

    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_label",  0,            "define label (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_data_group",  0,      "define data group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_workdir",  0,            "define workdir (required)", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_note",  0,           "define note", NULL);
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-set_dest_id",  0,      "define destination", 0);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",  0,            "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-rerun",   0,           "queue exposures even if a previous vpRun exists", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    // -pendingrun
    psMetadata *pendingrunArgs = psMetadataAlloc();
    psMetadataAddS64(pendingrunArgs, PS_LIST_TAIL, "-vp_id",  0,            "search by chip ID", 0);
    psMetadataAddStr(pendingrunArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by label", 0);
    pxchipSetSearchArgs(pendingrunArgs);
    psMetadataAddU64(pendingrunArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingrunArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -updaterun
    psMetadata *updaterunArgs = psMetadataAlloc();
//     pxchipSetSearchArgs (updaterunArgs);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-vp_id", 0,            "search by vpRun ID", 0);
    psMetadataAddS64(updaterunArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exp_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-state", 0,            "search by state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-label",  0,           "search by label (LIKE comparison)", NULL);
    psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-fault",  0,         "search by fault code", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-data_group",  0,      "search by data_group (LIKE comparison)", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state", 0,        "set state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_label", 0,        "set label", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_data_group", 0,   "set data group", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_note", 0,         "set note", NULL);
    psMetadataAddF32(updaterunArgs, PS_LIST_TAIL, "-set_dtime_script",  0,      "define elapsed time for script (seconds)", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_hostname", 0, "set hostname", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_outroot", 0, "set outroot", NULL);
    psMetadataAddS16(updaterunArgs, PS_LIST_TAIL, "-set_fault",  0,  "set fault code", 0);

    // -revertrun
    psMetadata *revertrunArgs = psMetadataAlloc();
    psMetadataAddS64(revertrunArgs, PS_LIST_TAIL, "-vp_id", 0,            "search by chip ID", 0);
    psMetadataAddStr(revertrunArgs,  PS_LIST_TAIL, "-label",              PS_META_DUPLICATE_OK, "search by vpRun label (LIKE comparison)", NULL);
    psMetadataAddStr(revertrunArgs,  PS_LIST_TAIL, "-data_group",              PS_META_DUPLICATE_OK, "search by vpRun data_group (LIKE comparison)", NULL);
    // pxchipSetSearchArgs(revertrunArgs);
    psMetadataAddS16(revertrunArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -pendingimfile
    psMetadata *pendingimfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingimfileArgs, PS_LIST_TAIL, "-vp_id",  0,            "search by chip ID", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by label", 0);
    pxchipSetSearchArgs(pendingimfileArgs);
    psMetadataAddU64(pendingimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    // -addprocessedcell
    psMetadata *addprocessedcellArgs = psMetadataAlloc();
    psMetadataAddS64(addprocessedcellArgs, PS_LIST_TAIL, "-vp_id",  0,            "define chip ID (required)", 0);
    psMetadataAddStr(addprocessedcellArgs, PS_LIST_TAIL, "-class_id",  0,          "define class ID (required)", NULL);
    psMetadataAddStr(addprocessedcellArgs, PS_LIST_TAIL, "-cell_id",  0,           "define class ID (required)", NULL);

    psMetadataAddStr(addprocessedcellArgs, PS_LIST_TAIL, "-path_base",  0,         "define base output location (required)", NULL);
    psMetadataAddF32(addprocessedcellArgs, PS_LIST_TAIL, "-dtime_photom",  0,      "define elapsed time for photometry (seconds)", NAN);
    psMetadataAddStr(addprocessedcellArgs, PS_LIST_TAIL, "-hostname",  0,          "define hostname", NULL);

    psMetadataAddS16(addprocessedcellArgs, PS_LIST_TAIL, "-fault",  0,              "set fault code", 0);
    psMetadataAddS16(addprocessedcellArgs, PS_LIST_TAIL, "-quality",  0,            "set quality", 0);
    

    // -processedcell
    psMetadata *processedcellArgs = psMetadataAlloc();
    pxchipSetSearchArgs(processedcellArgs);
    psMetadataAddS64(processedcellArgs, PS_LIST_TAIL,  "-vp_id",  0,         "search by  chip ID", 0);
    psMetadataAddStr(processedcellArgs,  PS_LIST_TAIL, "-class_id",           0, "search by class ID", NULL);
    psMetadataAddStr(processedcellArgs,  PS_LIST_TAIL, "-cell_id",           0, "search by class ID", NULL);
    psMetadataAddStr(processedcellArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by vpRun label (LIKE comparison)", NULL);
    psMetadataAddStr(processedcellArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by vpRun data_group (LIKE comparison)", NULL);

    psMetadataAddU64(processedcellArgs,  PS_LIST_TAIL, "-limit",  0,           "limit result set to N items", 0);
    psMetadataAddBool(processedcellArgs, PS_LIST_TAIL, "-simple",  0,         "use the simple output format", false);

    pxspaceAddArguments(processedcellArgs);

#ifdef notdef
    // -updateprocessedcell
    psMetadata *updateprocessedcellArgs = psMetadataAlloc();
    psMetadataAddS64(updateprocessedcellArgs, PS_LIST_TAIL, "-vp_id",  0,            "search by chip ID", 0);
    psMetadataAddStr(updateprocessedcellArgs,  PS_LIST_TAIL, "-class_id",           0, "search by class ID", NULL);
    psMetadataAddS16(updateprocessedcellArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code (required)", 0);
    psMetadataAddStr(updateprocessedcellArgs, PS_LIST_TAIL, "-set_state", 0,         "set state", NULL);
    psMetadataAddS16(updateprocessedcellArgs, PS_LIST_TAIL, "-set_quality",  0,            "set quality", 0);
#endif // notdef
    // -listrun
    psMetadata *listrunArgs = psMetadataAlloc();
    pxchipSetSearchArgs(listrunArgs);
    psMetadataAddS64(listrunArgs, PS_LIST_TAIL, "-vp_id",  0,         "search by  vpRun ID", 0);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "search by vpRun label (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-state",              0, "search by vpRun state", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by vpRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(listrunArgs,  PS_LIST_TAIL, "-dist_group",  PS_META_DUPLICATE_OK, "search by vpRun dist_group (LIKE comparison)", NULL);
    // pxmagicAddArguments(listrunArgs);

    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-pstamp_order",  0,    "order results for postage stamp server", false);
    psMetadataAddU64(listrunArgs,  PS_LIST_TAIL, "-limit",  0,           "limit result set to N items", 0);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-all",  0,            "list everything without search terms", false);
    psMetadataAddBool(listrunArgs, PS_LIST_TAIL, "-simple",  0,         "use the simple output format", false);
    pxspaceAddArguments(listrunArgs);

#ifdef notdef
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

    // -pendingcleanupimfile
    psMetadata *pendingcleanupimfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingcleanupimfileArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "list blocks for specified label", NULL);
    psMetadataAddS64(pendingcleanupimfileArgs, PS_LIST_TAIL, "-vp_id", 0,          "search by chip ID", 0);
    psMetadataAddStr(pendingcleanupimfileArgs, PS_LIST_TAIL, "-exp_id",                 0,            "search by exp_id", NULL);
    psMetadataAddBool(pendingcleanupimfileArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(pendingcleanupimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -donecleanup
    psMetadata *donecleanupArgs = psMetadataAlloc();
    psMetadataAddStr(donecleanupArgs, PS_LIST_TAIL, "-label",  0,            "list blocks for specified label", NULL);
    psMetadataAddBool(donecleanupArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);
    psMetadataAddU64(donecleanupArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -revertcleanup
    psMetadata *revertcleanupArgs = psMetadataAlloc();
    psMetadataAddS64(revertcleanupArgs, PS_LIST_TAIL, "-vp_id", 0,            "search by chip ID", 0);
    psMetadataAddStr(revertcleanupArgs,  PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by vpRun label (LIKE comparison)", NULL);
    psMetadataAddStr(revertcleanupArgs,  PS_LIST_TAIL, "-data_group",  PS_META_DUPLICATE_OK, "search by vpRun data_group (LIKE comparison)", NULL);
    psMetadataAddStr(revertcleanupArgs, PS_LIST_TAIL, "-state", 0,             "search by current state", NULL);
    psMetadataAddS16(revertcleanupArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -run
    psMetadata *runArgs = psMetadataAlloc();
    psMetadataAddStr(runArgs, PS_LIST_TAIL, "-label", PS_META_DUPLICATE_OK, "search by label", NULL);
    psMetadataAddBool(runArgs, PS_LIST_TAIL, "-simple",  0,     "use the simple output format", false);
    psMetadataAddU64(runArgs, PS_LIST_TAIL, "-limit",  0,       "limit result set to N items", 0);
    psMetadataAddStr(runArgs, PS_LIST_TAIL, "-state", 0,        "search by state (required)", NULL);
    pxchipSetSearchArgs(runArgs);
    psMetadataAddS64(runArgs, PS_LIST_TAIL, "-vp_id",  0,         "search by  chip ID", 0);
    psMetadataAddStr(runArgs,  PS_LIST_TAIL, "-reduction",          0, "search by reduction class", NULL);

    // -advanceexp
    psMetadata *advanceexpArgs = psMetadataAlloc();
    psMetadataAddS64(advanceexpArgs, PS_LIST_TAIL, "-vp_id",  0,          "search by chip ID", 0);
    psMetadataAddStr(advanceexpArgs, PS_LIST_TAIL, "-label",  PS_META_DUPLICATE_OK, "advance exposures for specified label", NULL);
    psMetadataAddU64(advanceexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -tocleanedimfile
    psMetadata *tocleanedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tocleanedimfileArgs, PS_LIST_TAIL, "-vp_id", 0,          "chip ID to update", 0);
    psMetadataAddStr(tocleanedimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -tofullimfile
    psMetadata *tofullimfileArgs = psMetadataAlloc();
    psMetadataAddS64(tofullimfileArgs, PS_LIST_TAIL, "-vp_id", 0,          "chip ID to update", 0);
    psMetadataAddStr(tofullimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -topurgedimfile
    psMetadata *topurgedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(topurgedimfileArgs, PS_LIST_TAIL, "-vp_id", 0,          "chip ID to update", 0);
    psMetadataAddStr(topurgedimfileArgs, PS_LIST_TAIL, "-class_id",  0,        "class ID to update", NULL);

    // -toscrubbedimfile
    psMetadata *toscrubbedimfileArgs = psMetadataAlloc();
    psMetadataAddS64(toscrubbedimfileArgs, PS_LIST_TAIL, "-vp_id", 0,        "chip ID to update", 0);
    psMetadataAddStr(toscrubbedimfileArgs, PS_LIST_TAIL, "-class_id", 0,       "class ID to update", NULL);

    // -exportrun
    psMetadata *exportrunArgs = psMetadataAlloc();
    // pxchipSetSearchArgs (exportrunArgs); XXX include search terms?
    psMetadataAddS64(exportrunArgs, PS_LIST_TAIL, "-vp_id", 0,          "export this chip ID (required)", 0);
    psMetadataAddStr(exportrunArgs, PS_LIST_TAIL, "-outfile", 0,          "export to this file (required)", NULL);
    psMetadataAddBool(exportrunArgs, PS_LIST_TAIL, "-clean",  0,          "mark tables as cleaned", false);
    psMetadataAddU64(exportrunArgs, PS_LIST_TAIL, "-limit",   0,          "limit result set to N items", 0);

    // -importrun
    psMetadata *importrunArgs = psMetadataAlloc();
    psMetadataAddStr(importrunArgs, PS_LIST_TAIL, "-infile",  0,          "import from this file (required)", NULL);

    // -runstate
    psMetadata *runstateArgs = psMetadataAlloc();
    psMetadataAddS64(runstateArgs, PS_LIST_TAIL, "-vp_id", 0,           "search by chip ID", 0);
    psMetadataAddS64(runstateArgs, PS_LIST_TAIL, "-exp_id", 0,            "search by exposure tag", 0);
    psMetadataAddStr(runstateArgs, PS_LIST_TAIL, "-exp_name", 0,          "search by exposure tag", 0);
    psMetadataAddStr(runstateArgs, PS_LIST_TAIL,  "-label",  PS_META_DUPLICATE_OK, "search by warpRun label", NULL);
    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-no_magic",  0,        "magic is not necessary for result", false);

    psMetadataAddU64(runstateArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(runstateArgs, PS_LIST_TAIL, "-simple",  0,          "use the simple output format", false);
#endif //notdef

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from raw exposures",       VPTOOL_MODE_DEFINEBYQUERY,        definebyqueryArgs);
    PXOPT_ADD_MODE("-pendingrun",           "list runs ready to process",           VPTOOL_MODE_PENDINGRUN,           pendingrunArgs);
    PXOPT_ADD_MODE("-updaterun",            "change vpRun properties",              VPTOOL_MODE_UPDATERUN,            updaterunArgs);
    PXOPT_ADD_MODE("-revertrun","clear a faulted vpRun",                            VPTOOL_MODE_REVERTRUN, revertrunArgs);
    PXOPT_ADD_MODE("-pendingimfile",        "list pending imfiles",                 VPTOOL_MODE_PENDINGIMFILE,        pendingimfileArgs);

    PXOPT_ADD_MODE("-addprocessedcell",   "add a processed cell",               VPTOOL_MODE_ADDPROCESSEDCELL,   addprocessedcellArgs);
    PXOPT_ADD_MODE("-processedcell",      "show processed cells",               VPTOOL_MODE_PROCESSEDCELL,      processedcellArgs);
    PXOPT_ADD_MODE("-listrun",              "list vpRuns",                        VPTOOL_MODE_LISTRUN,              listrunArgs);

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
