/*
 * receivetoolConfig.c
 *
 * Copyright (C) 2008
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
#include "receivetool.h"

pxConfig *receivetoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration!");
        psFree(config);
        return NULL;
    }

    // -definesource
    psMetadata *definesourceArgs = psMetadataAlloc();
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-source", 0, "define source (required)", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-product", 0, "define product (required)", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-workdir",   0, "define workdir (required)", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-comment", 0, "define comment", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-state", 0, "define state", "enabled");
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-last", 0, "define last fileset", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-status_product", 0, "define status_product", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-ds_dbname", 0, "define ds_dbname", NULL);
    psMetadataAddStr(definesourceArgs, PS_LIST_TAIL, "-ds_dbhost", 0, "define ds_dbhost", NULL);

    // -list
    psMetadata *listArgs = psMetadataAlloc();
    psMetadataAddStr(listArgs, PS_LIST_TAIL, "-source", 0, "search on source", NULL);
    psMetadataAddStr(listArgs, PS_LIST_TAIL, "-product", 0, "search on product", NULL);
    psMetadataAddStr(listArgs, PS_LIST_TAIL, "-comment", 0, "search on comment (LIKE)", NULL);
    psMetadataAddU64(listArgs, PS_LIST_TAIL, "-limit",  0, "limit result set", 0);
    psMetadataAddBool(listArgs, PS_LIST_TAIL, "-simple",  0, "use simple output format?", false);

    // -addfileset
    psMetadata *addfilesetArgs = psMetadataAlloc();
    psMetadataAddS64(addfilesetArgs, PS_LIST_TAIL, "-src_id", 0, "define source_id (required)", 0);
    psMetadataAddStr(addfilesetArgs, PS_LIST_TAIL, "-fileset", PS_META_DUPLICATE_OK, "define fileset (multiple OK, required)", NULL);

    // -updatelast
    psMetadata *updatelastArgs = psMetadataAlloc();
    psMetadataAddS64(updatelastArgs, PS_LIST_TAIL, "-src_id", 0, "define source_id (required)", 0);
    psMetadataAddStr(updatelastArgs, PS_LIST_TAIL, "-fileset", 0, "define last fileset (required)", NULL);

    // -pendingfileset
    psMetadata *pendingfilesetArgs = psMetadataAlloc();
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-source", 0, "search on source", NULL);
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-product", 0, "search on product", NULL);
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-comment", 0, "search on comment (LIKE)", NULL);
    psMetadataAddBool(pendingfilesetArgs, PS_LIST_TAIL, "-simple",  0, "use simple output format?", false);
    psMetadataAddU64(pendingfilesetArgs, PS_LIST_TAIL, "-limit",  0, "limit result set", 0);
    psMetadataAddStr(pendingfilesetArgs, PS_LIST_TAIL, "-label", 0, "search on label", NULL);

    // -updatefileset
    psMetadata *updatefilesetArgs = psMetadataAlloc();
    psMetadataAddS64(updatefilesetArgs, PS_LIST_TAIL, "-fileset_id", 0, "define fileset_id (required)", 0);
    psMetadataAddStr(updatefilesetArgs, PS_LIST_TAIL, "-set_state", 0, "define state", NULL);
    psMetadataAddStr(updatefilesetArgs, PS_LIST_TAIL, "-destdir", 0, "define destdir", NULL);
    psMetadataAddStr(updatefilesetArgs, PS_LIST_TAIL, "-dirinfo", 0, "define dirinfo", NULL);
    psMetadataAddStr(updatefilesetArgs, PS_LIST_TAIL, "-dbinfo", 0, "define dbinfo", NULL);
    psMetadataAddS32(updatefilesetArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);

    // -addfile
    psMetadata *addfileArgs = psMetadataAlloc();
    psMetadataAddS64(addfileArgs, PS_LIST_TAIL, "-fileset_id", 0, "define fileset_id (required)", 0);
    psMetadataAddStr(addfileArgs, PS_LIST_TAIL, "-file_list", 0, "define file list (required)", NULL);

    // -pendingfile
    psMetadata *pendingfileArgs = psMetadataAlloc();
    psMetadataAddStr(pendingfileArgs, PS_LIST_TAIL, "-source", 0, "search on source", NULL);
    psMetadataAddStr(pendingfileArgs, PS_LIST_TAIL, "-product", 0, "search on product", NULL);
    psMetadataAddStr(pendingfileArgs, PS_LIST_TAIL, "-comment", 0, "search on comment (LIKE)", NULL);
    psMetadataAddS64(pendingfileArgs, PS_LIST_TAIL, "-fileset_id", 0, "search on fileset_id", 0);
    psMetadataAddS64(pendingfileArgs, PS_LIST_TAIL, "-file_id", 0, "search on file_id", 0);
    psMetadataAddU64(pendingfileArgs, PS_LIST_TAIL, "-limit",  0, "limit result set", 0);
    psMetadataAddBool(pendingfileArgs, PS_LIST_TAIL, "-simple",  0, "use simple output format?", false);
    psMetadataAddStr(pendingfileArgs, PS_LIST_TAIL, "-label", 0, "search on label", NULL);

    // -addresult
    psMetadata *addresultArgs = psMetadataAlloc();
    psMetadataAddS64(addresultArgs, PS_LIST_TAIL, "-file_id", 0, "define receive_id (required)", 0);
    psMetadataAddStr(addresultArgs, PS_LIST_TAIL, "-path_base", 0, "path_base for component", NULL);
    psMetadataAddF32(addresultArgs, PS_LIST_TAIL, "-dtime_copy", 0, "define time to copy", NAN);
    psMetadataAddF32(addresultArgs, PS_LIST_TAIL, "-dtime_extract", 0, "define time to extract", NAN);
    psMetadataAddS32(addresultArgs, PS_LIST_TAIL, "-fault", 0, "define fault code", 0);

    // -revert
    psMetadata *revertArgs = psMetadataAlloc();
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-file_id", 0, "search on file_id", 0);
    psMetadataAddS64(revertArgs, PS_LIST_TAIL, "-fileset_id", 0, "search on fileset_id", 0);
    psMetadataAddS32(revertArgs, PS_LIST_TAIL, "-fault", 0, "search on fault code", 0);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-source", 0, "search on source", NULL);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-product", 0, "search on product", NULL);
    psMetadataAddStr(revertArgs, PS_LIST_TAIL, "-comment", 0, "search on comment (LIKE)", NULL);

    // -toadvance
    psMetadata *toadvanceArgs = psMetadataAlloc();
    psMetadataAddS64(toadvanceArgs, PS_LIST_TAIL, "-fileset_id", 0, "search on fileset_id", 0);
    psMetadataAddS64(toadvanceArgs, PS_LIST_TAIL, "-source_id", 0, "search on source_id", 0);
    psMetadataAddU64(toadvanceArgs, PS_LIST_TAIL, "-limit",  0, "limit result set", 0);
    psMetadataAddBool(toadvanceArgs, PS_LIST_TAIL, "-simple",  0, "use simple output format?", false);
    // label is not used but pantasks requires it
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-label", 0, "search on label", NULL);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-definesource", "", RECEIVETOOL_MODE_DEFINESOURCE, definesourceArgs);
    PXOPT_ADD_MODE("-list", "", RECEIVETOOL_MODE_LIST, listArgs);
    PXOPT_ADD_MODE("-addfileset", "", RECEIVETOOL_MODE_ADDFILESET, addfilesetArgs);
    PXOPT_ADD_MODE("-updatelast", "", RECEIVETOOL_MODE_UPDATELAST, updatelastArgs);
    PXOPT_ADD_MODE("-updatefileset", "", RECEIVETOOL_MODE_UPDATEFILESET, updatefilesetArgs);
    PXOPT_ADD_MODE("-pendingfileset", "", RECEIVETOOL_MODE_PENDINGFILESET, pendingfilesetArgs);
    PXOPT_ADD_MODE("-toadvance", "", RECEIVETOOL_MODE_TOADVANCE, toadvanceArgs);
    PXOPT_ADD_MODE("-addfile", "", RECEIVETOOL_MODE_ADDFILE, addfileArgs);
    PXOPT_ADD_MODE("-pendingfile", "", RECEIVETOOL_MODE_PENDINGFILE, pendingfileArgs);
    PXOPT_ADD_MODE("-addresult", "", RECEIVETOOL_MODE_ADDRESULT, addresultArgs);
    PXOPT_ADD_MODE("-revert", "", RECEIVETOOL_MODE_REVERT, revertArgs);

    if (!pxGetOptions(stderr, argc, argv, config, modes, argSets)) {
        psError(PS_ERR_UNKNOWN, true, "option parsing failed");
        psFree(argSets);
        psFree(modes);
        psFree(config);
        return NULL;
    }

    psFree(argSets);
    psFree(modes);

    // define database handle, if used
    // do this last so we don't setup a connection before CLI options are validated
    config->dbh = psMemIncrRefCounter(pmConfigDB(config->modules));
    if (!config->dbh) {
        psError(PS_ERR_UNKNOWN, false, "Can't configure database");
        psFree(config);
        return NULL;
    }

    return config;
}
