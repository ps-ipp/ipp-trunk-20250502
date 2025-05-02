/** @file ppMergeVersion.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

#include "ppMergeVersion.h"
#include "ppMergeVersionDefinitions.h"

#ifndef PPMERGE_VERSION
#error "PPMERGE_VERSION is not set"
#endif
#ifndef PPMERGE_BRANCH
#error "PPMERGE_BRANCH is not set"
#endif
#ifndef PPMERGE_SOURCE
#error "PPMERGE_SOURCE is not set"
#endif

psString ppMergeVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPMERGE_BRANCH, PPMERGE_VERSION);
    return value;
}

psString ppMergeSource(void)
{
    return psStringCopy(PPMERGE_SOURCE);
}

psString ppMergeVersionLong(void)
{
    psString version = ppMergeVersion();  // Version, to return
    psString source = ppMergeSource();    // Source

    psStringPrepend(&version, "ppMerge ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppMergeVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppMerge at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    ppStatsVersionHeader(header);

    psString version = ppMergeVersion(); // Software version
    psString source  = ppMergeSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "MERGE_V", PS_META_REPLACE, NULL, PPMERGE_VERSION);
    
    psStringPrepend(&version, "ppMerge version: ");
    psStringPrepend(&source, "ppMerge source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppMergeVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppMerge", PS_LOG_INFO, "ppMerge at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString ppMerge = ppMergeVersionLong(); // ppMerge version

    psLogMsg("ppImage", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", ppMerge);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppStats);
    psFree(ppMerge);

    return;
}
