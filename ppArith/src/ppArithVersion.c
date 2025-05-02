/** @file ppArithVersion.c
 *
 *  @brief
 *
 *  @ingroup ppArith
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 19:45:30 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

#include "ppArith.h"
#include "ppArithVersionDefinitions.h"

#ifndef PPARITH_VERSION
#error "PPARITH_VERSION is not set"
#endif
#ifndef PPARITH_BRANCH
#error "PPARITH_BRANCH is not set"
#endif
#ifndef PPARITH_SOURCE
#error "PPARITH_SOURCE is not set"
#endif

psString ppArithVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPARITH_BRANCH, PPARITH_VERSION);
    return value;
}

psString ppArithSource(void)
{
    return psStringCopy(PPARITH_SOURCE);
}

psString ppArithVersionLong(void)
{
    psString version = ppArithVersion();  // Version, to return
    psString source = ppArithSource();    // Source

    psStringPrepend(&version, "ppArith ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppArithVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppArith at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    ppStatsVersionHeader(header);

    psString version = ppArithVersion(); // Software version
    psString source  = ppArithSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "ARITH_V", PS_META_REPLACE, NULL, PPARITH_VERSION);
    
    psStringPrepend(&version, "ppArith version: ");
    psStringPrepend(&version, "ppArith source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppArithVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppArith", PS_LOG_INFO, "ppArith at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString ppArith = ppArithVersionLong(); // ppArith version

    psLogMsg("ppArith", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppArith", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppArith", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("ppArith", PS_LOG_INFO, "%s", ppArith);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppStats);
    psFree(ppArith);

    return;
}
