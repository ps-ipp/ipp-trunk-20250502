/*
 * pxinjectConfig.c
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

#include <psmodules.h>

#include "pxtools.h"
#include "pxinject.h"

pxConfig *pxinjectConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (!config->modules) {
        psError(PS_ERR_UNKNOWN, false, "Can't find site configuration");
        psFree(config);
        return NULL;
    }

    // -newExp
    psMetadata *newExpArgs = psMetadataAlloc();
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-tmp_exp_name",  0,            "define the exp_name (required)", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-tmp_inst",  0,            "define the camera name (required)", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-tmp_telescope",  0,            "define the telescope name (required)", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-workdir",  0,            "define workdir (required)", 0);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-reduction",  0,            "define reduction class", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-dvodb",  0,            "define the dvodb for the next processing step", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-tess_id",  0,            "define the tess_id for the next processing step", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-end_stage",  0,            "define the end goal processing step", NULL);
    psMetadataAddStr(newExpArgs, PS_LIST_TAIL, "-label",  0,            "define a label (carried to chip stage)", NULL);
    psMetadataAddBool(newExpArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -newImfile
    psMetadata *newImfileArgs = psMetadataAlloc();
    psMetadataAddS64(newImfileArgs, PS_LIST_TAIL, "-exp_id",  0,            "define the exp_id (required)", 0);
    psMetadataAddStr(newImfileArgs, PS_LIST_TAIL, "-tmp_class_id",  0,            "define the class ID (required)", NULL);
    psMetadataAddStr(newImfileArgs, PS_LIST_TAIL, "-uri",  0,            "define the URI (required)", NULL);
    psMetadataAddS32(newImfileArgs, PS_LIST_TAIL, "-bytes",  0,            "define the size of the file", 0);
    psMetadataAddStr(newImfileArgs, PS_LIST_TAIL, "-md5sum",  0,            "define the size of the file", NULL);

    // -updatenewExp
    psMetadata *updatenewExpArgs = psMetadataAlloc();
    psMetadataAddS64(updatenewExpArgs, PS_LIST_TAIL, "-exp_id",  0,            "define the exp_id (required)", 0);
    psMetadataAddStr(updatenewExpArgs, PS_LIST_TAIL, "-state", 0,            "set state (required)", NULL);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-newExp",      "", PXINJECT_MODE_NEWEXP,       newExpArgs);
    PXOPT_ADD_MODE("-newImfile",   "", PXINJECT_MODE_NEWIMFILE,    newImfileArgs);
    PXOPT_ADD_MODE("-updatenewExp",   "", PXINJECT_MODE_UPDATENEWEXP,    updatenewExpArgs);

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
