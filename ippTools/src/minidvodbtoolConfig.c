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
#include "pxadd.h"
#include "pxminidvodb.h"
#include "minidvodbtool.h"

pxConfig *addtoolConfig(pxConfig *config, int argc, char **argv)
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
    pxcamSetSearchArgs(definebyqueryArgs);
       psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-minidvodb_group", PS_META_DUPLICATE_OK, "search by minidvodbRun minidvodb_group", NULL);

       
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_minidvodb_rsync_path",        0, "define workdir", NULL);
    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_destination_host",          0, "define label", NULL);
    //    psMetadataAddStr(definebyqueryArgs, PS_LIST_TAIL, "-set_reduction",      0, "define reduction class", NULL);
   psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-pretend",           0, "do not actually modify the database", false);
    psMetadataAddBool(definebyqueryArgs, PS_LIST_TAIL, "-simple",            0, "use the simple output format", false);
   
    psMetadata *listminidvodbcopyArgs = psMetadataAlloc();
    psMetadataAddU64(listminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(listminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodbcopy_id",        0, "search by minidvodbcopy_id", NULL);
    psMetadataAddStr(listminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodb_group",         0, "search by minidvodbRun.minidvodb_group", NULL);
    psMetadataAddStr(listminidvodbcopyArgs, PS_LIST_TAIL, "-destination_host",        0, "search by minidvodbCopy.destination_host", NULL);
    psMetadataAddBool(listminidvodbcopyArgs, PS_LIST_TAIL, "-pending",        0, "limit to pending items", false);
    psMetadataAddU64(listminidvodbcopyArgs, PS_LIST_TAIL, "-limit",        0, "limit to N items", 0);
    psMetadataAddBool(listminidvodbcopyArgs, PS_LIST_TAIL, "-simple",        0, "simple output", false);
    psMetadataAddBool(listminidvodbcopyArgs, PS_LIST_TAIL, "-faulted",        0, "limit to faulted state", false);

    psMetadata *revertminidvodbcopyArgs = psMetadataAlloc();
    psMetadataAddU64(revertminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodbcopy_id",        0, "search by minidvodbcopy_id", 0);
    psMetadataAddU64(revertminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(revertminidvodbcopyArgs, PS_LIST_TAIL, "-destination_host",        0, "search by destination_host", NULL);
    psMetadataAddS16(revertminidvodbcopyArgs, PS_LIST_TAIL, "-fault",        0, "search by minidvodbCopyfault", 0);
    psMetadataAddBool(revertminidvodbcopyArgs, PS_LIST_TAIL, "-all",  0,            "allow everything to be queued without search terms", false);

    psMetadata *updateminidvodbcopyArgs = psMetadataAlloc();
    psMetadataAddU64(updateminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodbcopy_id",        0, "search by minidvodbcopy_id", 0);
    psMetadataAddU64(updateminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodb_id",        0, "search by minidvodb_id", 0);
    psMetadataAddStr(updateminidvodbcopyArgs, PS_LIST_TAIL, "-state",        0, "search by state", NULL);
    psMetadataAddStr(updateminidvodbcopyArgs, PS_LIST_TAIL, "-host",        0, "search by host", NULL);
    psMetadataAddStr(updateminidvodbcopyArgs, PS_LIST_TAIL, "-minidvodb_rsync_path",        0, "search by rsync", NULL);
    psMetadataAddS16(updateminidvodbcopyArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);
  

    psMetadataAddS16(updateminidvodbcopyArgs, PS_LIST_TAIL, "-set_fault",  0,            "set fault code", 0);
  
    psMetadataAddF32(updateminidvodbcopyArgs, PS_LIST_TAIL, "-set_dtime",  0,    "set elapsed time for transfer", 0);
    psMetadataAddStr(updateminidvodbcopyArgs, PS_LIST_TAIL, "-set_minidvodb_rsync_path",        0, "change the minidvodb_rsync_path", NULL); 
    psMetadataAddStr(updateminidvodbcopyArgs, PS_LIST_TAIL, "-set_destination_host",        0, "change the host", NULL);
    psMetadataAddStr(updateminidvodbcopyArgs, PS_LIST_TAIL, "-set_state",        0, "change the state", NULL);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definebyquery",        "create runs from cam stage",           ADDTOOL_MODE_DEFINEBYQUERY, definebyqueryArgs);
       PXOPT_ADD_MODE("-listminidvodbcopy","list copy minidvodbs",           ADDTOOL_MODE_LISTMINIDVODBCOPY, listminidvodbcopyArgs);
    PXOPT_ADD_MODE("-revertminidvodbcopy","revert copy minidvobs",        ADDTOOL_MODE_REVERTMINIDVODBCOPY,     revertminidvodbcopyArgs);
    PXOPT_ADD_MODE("-updateminidvodbcopy","change minidvodb copy properties",ADDTOOL_MODE_UPDATEMINIDVODBCOPY,  updateminidvodbcopyArgs);
 
 


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
