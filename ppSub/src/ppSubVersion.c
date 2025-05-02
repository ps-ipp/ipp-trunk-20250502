/** @file ppSubVersion.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>
#include <psphot.h>

#include "ppSub.h"
#include "ppSubVersionDefinitions.h"

#ifndef PPSUB_VERSION
#error "PPSUB_VERSION is not set"
#endif
#ifndef PPSUB_BRANCH
#error "PPSUB_BRANCH is not set"
#endif
#ifndef PPSUB_SOURCE
#error "PPSUB_SOURCE is not set"
#endif

psString ppSubVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPSUB_BRANCH, PPSUB_VERSION);
    return value;
}

psString ppSubSource(void)
{
    return psStringCopy(PPSUB_SOURCE);
}

psString ppSubVersionLong(void)
{
    psString version = ppSubVersion();  // Version, to return
    psString source = ppSubSource();    // Source

    psStringPrepend(&version, "ppSub ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};


bool ppSubVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppSub at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    psphotVersionHeader(header);
    ppStatsVersionHeader(header);

    psString version = ppSubVersion(); // Software version
    psString source  = ppSubSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "PPSUB_V", PS_META_REPLACE, NULL, PPSUB_VERSION);
    
    psStringPrepend(&version, "ppSub version: ");
    psStringPrepend(&source, "ppSub source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppSubVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppSub", PS_LOG_INFO, "ppSub at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString psphot = psphotVersionLong(); // psphot version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString ppSub = ppSubVersionLong(); // ppSub version

    psLogMsg("ppSub", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppSub", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppSub", PS_LOG_INFO, "%s", psphot);
    psLogMsg("ppSub", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("ppSub", PS_LOG_INFO, "%s", ppSub);

    psFree(pslib);
    psFree(psmodules);
    psFree(psphot);
    psFree(ppStats);
    psFree(ppSub);

    return;
}
