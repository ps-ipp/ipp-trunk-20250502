/*
 * pztoolConfig.c
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
#include "pztool.h"

pxConfig *pztoolConfig(pxConfig *config, int argc, char **argv)
{
    if (!config) {
        config = pxConfigAlloc();
    }

    pmConfigReadParamsSet(false);

    // setup site config
    config->modules = pmConfigRead(&argc, argv, NULL);
    if (! config->modules) {
        psError(psErrorCodeLast(), false, "Can't find site configuration!\n");
        psFree(config);
        return NULL;
    }

    // -adddatastore
    psMetadata *adddatastoreArgs = psMetadataAlloc();
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID", NULL);
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID", NULL);
    psMetadataAddStr(adddatastoreArgs, PS_LIST_TAIL, "-uri", 0,            "define storage uri", NULL);

    // -datastore
    psMetadata *datastoreArgs = psMetadataAlloc();
    psMetadataAddBool(datastoreArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -seen
    psMetadata *seenArgs = psMetadataAlloc();
    psMetadataAddStr(seenArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID", NULL);
    psMetadataAddStr(seenArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID", NULL);
    psMetadataAddStr(seenArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID", NULL);
    psMetadataAddStr(seenArgs, PS_LIST_TAIL, "-exp_type", 0,            "define exposure type", NULL);
    psMetadataAddBool(seenArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -pendingexp
    psMetadata *pendingexpArgs = psMetadataAlloc();
    psMetadataAddS64(pendingexpArgs, PS_LIST_TAIL, "-summit_id", 0,          "define summit_id", 0);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID", NULL);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID", NULL);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID", NULL);
    psMetadataAddTime(pendingexpArgs, PS_LIST_TAIL, "-dateobs_begin", 0,  "search for exposures by time (>=)", NULL);
    psMetadataAddTime(pendingexpArgs, PS_LIST_TAIL, "-dateobs_end", 0,  "search for exposures by time (<=)", NULL);
    psMetadataAddStr(pendingexpArgs, PS_LIST_TAIL, "-exp_type", 0,            "define exposure type", NULL);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-desc", 0,            "sort ouput in descending format", false);
    psMetadataAddU64(pendingexpArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingexpArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -pendingimfile
    psMetadata *pendingimfileArgs = psMetadataAlloc();
    psMetadataAddS64(pendingimfileArgs, PS_LIST_TAIL, "-summit_id", 0,          "define summit_id", 0);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID", NULL);
    psMetadataAddTime(pendingimfileArgs, PS_LIST_TAIL, "-dateobs_begin", 0,  "search for exposures by time (>=)", NULL);
    psMetadataAddTime(pendingimfileArgs, PS_LIST_TAIL, "-dateobs_end", 0,  "search for exposures by time (<=)", NULL);
    psMetadataAddStr(pendingimfileArgs, PS_LIST_TAIL, "-exp_type", 0,            "define exposure type", NULL);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-desc", 0,            "sort ouput in descending format", false);
    psMetadataAddU64(pendingimfileArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(pendingimfileArgs, PS_LIST_TAIL, "-simple", 0,            "use the simple output format", false);

    // -copydone
    psMetadata *copydoneArgs = psMetadataAlloc();
    psMetadataAddS64(copydoneArgs, PS_LIST_TAIL, "-summit_id", 0,          "define summit_id (required)", 0);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID (required)", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID (required)", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID (required)", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-class", 0,            "define class", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-class_id", 0,            "define class_id", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-uri", 0,            "define storage uri", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-workdir",  0,        "define the \"default\" workdir for this exposure", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-dvodb",  0,        "define the dvodb for the next processing step", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-tess_id",  0,        "define the tess_id for the next processing step", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-end_stage",  0,        "define the end goal processing step", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-label",  0,        "define the label for the chip stage", NULL);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-hostname",  0,     "define the host that copied the image", NULL);
    psMetadataAddS32(copydoneArgs, PS_LIST_TAIL, "-bytes",  0,     "define the size in bytes for the copied image", 0);
    psMetadataAddStr(copydoneArgs, PS_LIST_TAIL, "-md5sum",  0,     "define the md5 sum for the copied image", NULL);
    psMetadataAddS16(copydoneArgs, PS_LIST_TAIL, "-fault",  0,            "set fault code", 0);
    psMetadataAddBool(copydoneArgs, PS_LIST_TAIL, "-row_lock", 0,     "lock pzDownImfile rows while advancing an exposure", false);
    // XXX: remove this once advance is fixed
    psMetadataAddU64(copydoneArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);

    // -copied
    psMetadata *copiedArgs = psMetadataAlloc();
    psMetadataAddStr(copiedArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID", NULL);
    psMetadataAddStr(copiedArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID", NULL);
    psMetadataAddStr(copiedArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID", NULL);
    psMetadataAddStr(copiedArgs, PS_LIST_TAIL, "-class", 0,            "define class", NULL);
    psMetadataAddStr(copiedArgs, PS_LIST_TAIL, "-class_id", 0,            "define class_id", NULL);
    psMetadataAddBool(copiedArgs, PS_LIST_TAIL, "-faulted",  0,            "only return imfiles with a fault status set", false);
    psMetadataAddU64(copiedArgs, PS_LIST_TAIL, "-limit",  0,            "limit result set to N items", 0);
    psMetadataAddBool(copiedArgs, PS_LIST_TAIL, "-simple",  0,            "use the simple output format", false);

    // -updatecopied
    psMetadata *updatecopiedArgs = psMetadataAlloc();
    psMetadataAddStr(updatecopiedArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID (required)", NULL);
    psMetadataAddStr(updatecopiedArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID (required)", NULL);
    psMetadataAddStr(updatecopiedArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID (required)", NULL);
    psMetadataAddStr(updatecopiedArgs, PS_LIST_TAIL, "-class", 0,            "define class", NULL);
    psMetadataAddStr(updatecopiedArgs, PS_LIST_TAIL, "-class_id", 0,            "define class_id", NULL);
    psMetadataAddS16(updatecopiedArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -revertcopied
    psMetadata *revertcopiedArgs = psMetadataAlloc();
    psMetadataAddStr(revertcopiedArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID (required)", NULL);
    psMetadataAddStr(revertcopiedArgs, PS_LIST_TAIL, "-inst", 0,            "define camera ID (required)", NULL);
    psMetadataAddStr(revertcopiedArgs, PS_LIST_TAIL, "-telescope", 0,            "define telescope ID (required)", NULL);
    psMetadataAddStr(revertcopiedArgs, PS_LIST_TAIL, "-class", 0,            "define class", NULL);
    psMetadataAddStr(revertcopiedArgs, PS_LIST_TAIL, "-class_id", 0,            "define class_id", NULL);
    psMetadataAddS16(revertcopiedArgs, PS_LIST_TAIL, "-fault",  0,            "search by fault code", 0);

    // -clearcommonfaults
    psMetadata *clearcommonfaultsArgs = psMetadataAlloc();
    //
    // -toadvance
    psMetadata *toadvanceArgs = psMetadataAlloc();
    psMetadataAddS64(toadvanceArgs, PS_LIST_TAIL, "-summit_id", 0,     "define summit_id", 0);
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-exp_name", 0,      "define exposure ID", NULL);
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-inst", 0,          "define camera ID", NULL);
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-telescope", 0,     "define telescope ID", NULL);
    psMetadataAddStr(toadvanceArgs, PS_LIST_TAIL, "-label",  0,        "define the label for the chip stage", NULL);
    psMetadataAddBool(toadvanceArgs, PS_LIST_TAIL, "-simple",  0,      "use the simple output format", false);
    psMetadataAddU64(toadvanceArgs, PS_LIST_TAIL, "-limit",  0,        "limit result set to N items", 0);

    // -advance
    psMetadata *advanceArgs = psMetadataAlloc();
    psMetadataAddS64(advanceArgs, PS_LIST_TAIL, "-summit_id", 0,     "define summit_id", 0);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-exp_name", 0,   "define exposure ID (required)", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-inst", 0,       "define camera ID (required)", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-telescope", 0,  "define telescope ID (required)", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-workdir",  0,   "define the \"default\" workdir for this exposure (required)", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-dvodb",  0,     "define the dvodb for the next processing step", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-tess_id",  0,   "define the tess_id for the next processing step", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-end_stage",  0, "define the end goal processing step", NULL);
    psMetadataAddStr(advanceArgs, PS_LIST_TAIL, "-label",  0,     "define the label for the chip stage", NULL);

    // -updatepzexp
    psMetadata *updatepzexpArgs = psMetadataAlloc();
    psMetadataAddS64(updatepzexpArgs, PS_LIST_TAIL, "-summit_id", 0,     "define summit_id", 0);
    psMetadataAddStr(updatepzexpArgs, PS_LIST_TAIL, "-exp_name",   0,            "search by exposure name (required)", NULL);
    psMetadataAddStr(updatepzexpArgs, PS_LIST_TAIL, "-inst",       0,            "search by camera (required)", NULL);
    psMetadataAddStr(updatepzexpArgs, PS_LIST_TAIL, "-telescope",  0,            "search by telescope (required)", NULL);
    psMetadataAddStr(updatepzexpArgs, PS_LIST_TAIL, "-set_state",  0,            "define new state (required)", NULL);
    
# if (0)
    // -updatesummitexp
    psMetadata *updatesummitexpArgs = psMetadataAlloc();
    psMetadataAddS64(updatesummitexpArgs, PS_LIST_TAIL, "-summit_id", 0,     "define summit_id", 0);
    psMetadataAddStr(updatesummitexpArgs, PS_LIST_TAIL, "-exp_name",   0,            "search by exposure name (required)", NULL);
    psMetadataAddStr(updatesummitexpArgs, PS_LIST_TAIL, "-inst",       0,            "search by camera (required)", NULL);
    psMetadataAddStr(updatesummitexpArgs, PS_LIST_TAIL, "-telescope",  0,            "search by telescope (required)", NULL);
    psMetadataAddS32(updatesummitexpArgs, PS_LIST_TAIL, "-set_fault",  0,            "define new fault (required)", 0);
# endif
    
    // -updatenewexp
    psMetadata *updatenewexpArgs = psMetadataAlloc();
    psMetadataAddS64(updatenewexpArgs, PS_LIST_TAIL, "-exp_id",   0,            "search by exposure ID", 0);

    psMetadataAddStr(updatenewexpArgs, PS_LIST_TAIL, "-exp_name", 0,            "define exposure ID", NULL);
    psMetadataAddStr(updatenewexpArgs, PS_LIST_TAIL, "-set_state", 0,            "define new state (required)", NULL);

    psMetadata *argSets = psMetadataAlloc();
    psMetadata *modes = psMetadataAlloc();

    PXOPT_ADD_MODE("-adddatastore",    "", PZTOOL_MODE_ADDDATASTORE, adddatastoreArgs);
    PXOPT_ADD_MODE("-datastore",       "", PZTOOL_MODE_DATASTORE,    datastoreArgs);
    PXOPT_ADD_MODE("-seen",            "", PZTOOL_MODE_SEEN,         seenArgs);
    PXOPT_ADD_MODE("-pendingexp",      "", PZTOOL_MODE_PENDINGEXP,   pendingexpArgs);
    PXOPT_ADD_MODE("-pendingimfile",   "", PZTOOL_MODE_PENDINGIMFILE,pendingimfileArgs);
    PXOPT_ADD_MODE("-copydone",        "", PZTOOL_MODE_COPYDONE,     copydoneArgs);
    PXOPT_ADD_MODE("-copied",          "", PZTOOL_MODE_COPIED,      copiedArgs);
    PXOPT_ADD_MODE("-updatecopied",    "", PZTOOL_MODE_UPDATECOPIED,updatecopiedArgs);
    PXOPT_ADD_MODE("-revertcopied",    "", PZTOOL_MODE_REVERTCOPIED,revertcopiedArgs);
    PXOPT_ADD_MODE("-clearcommonfaults","", PZTOOL_MODE_CLEARCOMMONFAULTS,clearcommonfaultsArgs);
    PXOPT_ADD_MODE("-toadvance",          "", PZTOOL_MODE_TOADVANCE,    toadvanceArgs);
    PXOPT_ADD_MODE("-advance",          "", PZTOOL_MODE_ADVANCE,    advanceArgs);
    PXOPT_ADD_MODE("-updatepzexp",     "", PZTOOL_MODE_UPDATEPZEXP, updatepzexpArgs);
    PXOPT_ADD_MODE("-updatenewexp",    "", PZTOOL_MODE_UPDATENEWEXP,updatenewexpArgs);

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
