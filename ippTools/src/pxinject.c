/*
 * pxinject.c
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

#include <stdlib.h>
#include <ippdb.h>
#include <string.h>

#include "pxtools.h"
#include "pxinject.h"

static bool newExpMode(pxConfig *config);
static bool newImfileMode(pxConfig *config);
static bool updatenewExpMode(pxConfig *config);

# define MODECASE(caseName, func) \
    case caseName: \
    if (!func(config)) { \
        goto FAIL; \
    } \
    break;

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pxinjectConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    switch (config->mode) {
        MODECASE(PXINJECT_MODE_NEWEXP, newExpMode);
        MODECASE(PXINJECT_MODE_NEWIMFILE, newImfileMode);
        MODECASE(PXINJECT_MODE_UPDATENEWEXP, updatenewExpMode);
        default:
            psAbort("invalid option (this should not happen)");
    }

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(EXIT_SUCCESS);

FAIL:
    psErrorStackPrint (stderr, "failure\n");
    int exit_status = pxerrorGetExitStatus();

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(exit_status);
}

static bool newExpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_STR(tmp_exp_name, config->args, "-tmp_exp_name", true, false);
    PXOPT_LOOKUP_STR(tmp_camera, config->args, "-tmp_inst", true, false);
    PXOPT_LOOKUP_STR(tmp_telescope, config->args, "-tmp_telescope", true, false);
    PXOPT_LOOKUP_STR(workdir, config->args, "-workdir", true, false);
    PXOPT_LOOKUP_STR(reduction, config->args, "-reduction", false, false);
    PXOPT_LOOKUP_STR(dvodb, config->args, "-dvodb", false, false);
    PXOPT_LOOKUP_STR(tess_id, config->args, "-tess_id", false, false);
    PXOPT_LOOKUP_STR(end_stage, config->args, "-end_stage", false, false);
    PXOPT_LOOKUP_STR(label, config->args, "-label", false, false);
    PXOPT_LOOKUP_BOOL(simple, config->args, "-simple", false);

    if (!newExpInsert(config->dbh,
                0,    // exp_id
		0, // summit_id
                tmp_exp_name,
                tmp_camera,
                tmp_telescope,
                "reg",  // state
                workdir,
                "dirty",
                reduction,
                dvodb,
                tess_id,
                end_stage,
                label,
                NULL    // epoch
            )
        ) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    psS64 exp_id = psDBLastInsertID(config->dbh);

    psMetadata *md = psMetadataAlloc();
    if (!psMetadataAddS64(md, PS_LIST_TAIL, "exp_id", 0, NULL, exp_id)) {
        psError(PS_ERR_UNKNOWN, false, "failed to add item exp_id");
        psFree(md);
    }

    // negate simple so the default is true
    if (!ippdbPrintMetadata(stdout, md, !simple)) {
        psError(PS_ERR_UNKNOWN, false, "failed to print array");
        psFree(md);
        return false;
    }

    psFree(md);

    return true;
}

static bool newImfileMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);
    PXOPT_LOOKUP_STR(tmp_class_id, config->args, "-tmp_class_id", true, false);
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);
    PXOPT_LOOKUP_S32(bytes, config->args, "-bytes", false, false);
    PXOPT_LOOKUP_STR(md5sum, config->args, "-md5sum", false, false);

    // insert with error flag state set to 0 (no errors)
    if (!newImfileInsert(config->dbh, exp_id, tmp_class_id, uri, NULL, bytes, md5sum)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}


static bool updatenewExpMode(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    PXOPT_LOOKUP_S64(exp_id, config->args, "-exp_id", true, false);
    PXOPT_LOOKUP_STR(state, config->args, "-state", true, false);

    if (state) {
        // set detRun.state to state
        return pxnewExpSetState(config, exp_id, state);
    }

    return true;
}
