/*
 * addtoolConfig.c
 *
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
#include "pxmerge.h"
#include "mergetool.h"

pxConfig *mergetoolConfig(pxConfig *config, int argc, char **argv)
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
    psMetadata *definebyqueryArgs = psMetadataAlloc();
    psMetadataAddS64(definebyqueryArgs, PS_LIST_TAIL, "-minidvodb_id",             0, "search by minidvodb_id", 0);
    // pxcamSetSearchArgs(definebyqueryArgs);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-mergedvodb", PS_META_DUPLICATE_OK, "search by mergedvodbRun.mergedvodb and use as mergedvodb", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-minidvodb_group", PS_META_DUPLICATE_OK, "search by minidvodbRun.minidvodb_group", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_state",        0, "define state", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_mergedvodb_path",        0, "define workdir", NULL);
    //  psMetadataAddStr(definebyqueryArge, PS_LIST_TAIL, "-set_mergedvodb", 0, "define mergedvodb");

        //    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",      0, "define reduction class", NULL);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",           0, "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);
   
    psMetadata *updaterunArgs = psMetadataAlloc(); 

    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-mergedvodb",        0, "search by mergedvodb", 0);
    psMetadataAddU64(updaterunArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", 0);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_state",        0, "change the state", NULL);
    psMetadataAddStr(updaterunArgs, PS_LIST_TAIL, "-set_mergedvodb_path",        0, "define workdir", NULL);
   

    psMetadata *pendingmergeArgs = psMetadataAlloc();
    psMetadataAddU64(pendingmergeArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(pendingmergeArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", NULL);
    psMetadataAddStr(pendingmergeArgs, PS_LIST_TAIL, "-mergedvodb",         0, "search by mergedvodb", NULL);
 
    psMetadataAddU64(pendingmergeArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(pendingmergeArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
   

    psMetadata *addmergedArgs = psMetadataAlloc();
    psMetadataAddU64(addmergedArgs, PS_LIST_TAIL, "-merge_id", 0, "search by merge_id (required)", 0);
 psMetadataAddStr(addmergedArgs, PS_LIST_TAIL, "-mergedvodb", 0, "search by mergedvodb (required)", 0);
    psMetadataAddF32(addmergedArgs, PS_LIST_TAIL, "-dtime_verify",    0,    "define elapsed time for DVO verify (seconds)", NAN);
    psMetadataAddF32(addmergedArgs, PS_LIST_TAIL, "-dtime_merge",    0,    "define elapsed time for DVO merge (seconds)", NAN);
    psMetadataAddF32(addmergedArgs, PS_LIST_TAIL, "-dtime_script",    0,    "define elapsed time for script (seconds)", NAN);
    psMetadataAddTime(addmergedArgs, PS_LIST_TAIL, "-epoch",         0,    "time merge is finished", NULL);
    psMetadataAddS16(addmergedArgs, PS_LIST_TAIL, "-fault",          0,    "set fault code", 0);




    psMetadata *listmergedArgs = psMetadataAlloc();
    psMetadataAddU64(listmergedArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(listmergedArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", NULL);
    psMetadataAddStr(listmergedArgs, PS_LIST_TAIL, "-mergedvodb",         0, "search by minidvodbRun.minidvodb_group", NULL);
    psMetadataAddU64(listmergedArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(listmergedArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
    psMetadataAddBool(listmergedArgs, PS_LIST_TAIL, "-faulted",        0, "limit to faulted state", false);


    psMetadata *revertmergedArgs = psMetadataAlloc();
    psMetadataAddU64(revertmergedArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", 0);
    psMetadataAddS16(revertmergedArgs, PS_LIST_TAIL, "-fault",        0, "search by minidvodbCopyfault", 0);
    psMetadataAddStr(revertmergedArgs, PS_LIST_TAIL, "-mergedvodb",        0, "search by mergedvodb", NULL);

    psMetadata *updatemergedArgs = psMetadataAlloc();

    psMetadataAddU64(updatemergedArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", 0);
    psMetadataAddS16(updatemergedArgs, PS_LIST_TAIL, "-set_fault",  0,            "set fault code", 0);
    psMetadataAddF32(updatemergedArgs, PS_LIST_TAIL, "-set_dtime_verify",    0,    "define elapsed time for DVO verify (seconds)", 0);
    psMetadataAddF32(updatemergedArgs, PS_LIST_TAIL, "-set_dtime_merge",    0,    "define elapsed time for DVO merge (seconds)", 0);
    psMetadataAddF32(updatemergedArgs, PS_LIST_TAIL, "-set_dtime_script",    0,    "define elapsed time for script (seconds)", 0);
    psMetadataAddStr(updatemergedArgs, PS_LIST_TAIL, "-set_state",        0, "change the state", NULL);
  // -definebyquerymergecopy
    psMetadata *definebyquerymergecopyArgs = psMetadataAlloc();
    psMetadataAddS64(definebyquerymergecopyArgs, PS_LIST_TAIL, "-merge_id",             0, "search by merge_id", 0);
    psMetadataAddStr(definebyquerymergecopyArgs, PS_LIST_TAIL, "-mergedvodb", PS_META_DUPLICATE_OK, "search by mergedvodb", NULL);
    psMetadataAddStr(definebyquerymergecopyArgs, PS_LIST_TAIL, "-set_mergedvodb_rsync_path",        0, "define workdir", NULL);
    psMetadataAddStr(definebyquerymergecopyArgs, PS_LIST_TAIL, "-set_destination_host",          0, "set destination host", NULL);
    psMetadataAddBool(definebyquerymergecopyArgs, PS_LIST_TAIL, "-pretend",           0, "do not actually modify the database", false);
    psMetadataAddBool(definebyquerymergecopyArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);
    psMetadataAddBool(definebyquerymergecopyArgs, PS_LIST_TAIL, "-last_merged",            0, "if multiple minidvodbs have been dvomerged between rsyncs, then only queue the most recent merged dvodb for copying", false);

   psMetadata *listmergedvodbcopyArgs = psMetadataAlloc();
    psMetadataAddU64(listmergedvodbcopyArgs, PS_LIST_TAIL, "-merge_id",        0, "search by mergedvodb_id", 0);
    psMetadataAddStr(listmergedvodbcopyArgs, PS_LIST_TAIL, "-mergedvodbcopy_id",        0, "search by mergedvodbcopy_id", NULL);
    psMetadataAddStr(listmergedvodbcopyArgs, PS_LIST_TAIL, "-mergedvodb",         0, "search by mergedvodbRun.mergedvodb", NULL);
    psMetadataAddStr(listmergedvodbcopyArgs, PS_LIST_TAIL, "-destination_host",        0, "search by mergedvodbCopy.destination_host", NULL);
    psMetadataAddBool(listmergedvodbcopyArgs, PS_LIST_TAIL, "-pending",        0, "limit to pending items", false);
    psMetadataAddU64(listmergedvodbcopyArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(listmergedvodbcopyArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
    psMetadataAddBool(listmergedvodbcopyArgs, PS_LIST_TAIL, "-faulted",        0, "limit to faulted state", false);

    psMetadata *revertmergedvodbcopyArgs = psMetadataAlloc();
    psMetadataAddU64(revertmergedvodbcopyArgs, PS_LIST_TAIL, "-mergedvodbcopy_id",        0, "search by mergedvodbcopy_id", 0);
    psMetadataAddU64(revertmergedvodbcopyArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", 0);
    psMetadataAddStr(revertmergedvodbcopyArgs, PS_LIST_TAIL, "-destination_host",        0, "search by destination_host", NULL);
    psMetadataAddS16(revertmergedvodbcopyArgs, PS_LIST_TAIL, "-fault",        0, "search by mergedvodbCopyfault", 0);
    psMetadataAddBool(revertmergedvodbcopyArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    psMetadata *updatemergedvodbcopyArgs = psMetadataAlloc();
    psMetadataAddU64(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-mergedvodbcopy_id",        0, "search by mergedvodbcopy_id", 0);
    psMetadataAddU64(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-merge_id",        0, "search by merge_id", 0);
    psMetadataAddStr(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-state",        0, "search by state", NULL);
    psMetadataAddStr(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-host",        0, "search by host", NULL);
    psMetadataAddStr(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-mergedvodb_rsync_path",        0, "search by rsync", NULL);
    psMetadataAddS16(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);


    psMetadataAddS16(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-set_fault",  0,            "set fault code", 0);
  
    psMetadataAddF32(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-set_dtime",  0,    "set elapsed time for transfer", 0);
    psMetadataAddStr(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-set_mergedvodb_rsync_path",        0, "change the mergedvodb_rsync_path", NULL); 
    psMetadataAddStr(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-set_destination_host",        0, "change the host", NULL);
    psMetadataAddStr(updatemergedvodbcopyArgs, PS_LIST_TAIL, "-set_state",        0, "change the state", NULL);






    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from minindvodbRun",           MERGETOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
    PXOPT_ADD_MODE("-updaterun",        "update existing mergedvodbRuns",           MERGETOOL_MODE_UPDATERUN, updaterunArgs);
    PXOPT_ADD_MODE("-pendingmerge",        "list pending mergedvodbRuns",           MERGETOOL_MODE_PENDINGMERGE, pendingmergeArgs);
    PXOPT_ADD_MODE("-addmerged",        "add processed mergedvodbRun",           MERGETOOL_MODE_ADDMERGED, addmergedArgs);
    PXOPT_ADD_MODE("-listmerged","list merged dvodbs",           MERGETOOL_MODE_LISTMERGED, listmergedArgs);
    PXOPT_ADD_MODE("-revertmerged","revert merged dvodbs",        MERGETOOL_MODE_REVERTMERGED,     revertmergedArgs);
    PXOPT_ADD_MODE("-updatemerged","change merged dvodb properties",MERGETOOL_MODE_UPDATEMERGED,  updatemergedArgs); 
 PXOPT_ADD_MODE("-definebyquerymergecopy",   "create runs from mergedvodbRun",  MERGETOOL_MODE_DEFINEBYQUERYMERGECOPY, definebyquerymergecopyArgs);
 PXOPT_ADD_MODE("-listmergedvodbcopy","list copy mergedvodbs", MERGETOOL_MODE_LISTMERGEDVODBCOPY, listmergedvodbcopyArgs);
    PXOPT_ADD_MODE("-revertmergedvodbcopy","revert copy mergedvobs",MERGETOOL_MODE_REVERTMERGEDVODBCOPY,     revertmergedvodbcopyArgs);
    PXOPT_ADD_MODE("-updatemergedvodbcopy","change mergedvodb copy properties",MERGETOOL_MODE_UPDATEMERGEDVODBCOPY,  updatemergedvodbcopyArgs);



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
