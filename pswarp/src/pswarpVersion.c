/** @file pswarpVersion.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pswarp.h"
#include "pswarpVersionDefinitions.h"

#ifndef PSWARP_VERSION
#error "PSWARP_VERSION is not set"
#endif
#ifndef PSWARP_BRANCH
#error "PSWARP_BRANCH is not set"
#endif
#ifndef PSWARP_SOURCE
#error "PSWARP_SOURCE is not set"
#endif

psString pswarpVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PSWARP_BRANCH, PSWARP_VERSION);
    return value;
}

psString pswarpSource(void)
{
    return psStringCopy(PSWARP_SOURCE);
}

psString pswarpVersionLong(void)
{
    psString version = pswarpVersion();  // Version, to return
    psString source = pswarpSource();    // Source

    psStringPrepend(&version, "pswarp ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};


bool pswarpVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "pswarp at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    psphotVersionHeader(header);
    ppStatsVersionHeader(header);

    psString version = pswarpVersion(); // Software version
    psString source  = pswarpSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "WARP_V", PS_META_REPLACE, NULL, PSWARP_VERSION);
    
    psStringPrepend(&version, "pswarp version: ");
    psStringPrepend(&source, "pswarp source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void pswarpVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("pswarp", PS_LOG_INFO, "pswarp at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString psphot = psphotVersionLong(); // psphot version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString pswarp = pswarpVersionLong(); // pswarp version

    psLogMsg("pswarp", PS_LOG_INFO, "%s", pslib);
    psLogMsg("pswarp", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("pswarp", PS_LOG_INFO, "%s", psphot);
    psLogMsg("pswarp", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("pswarp", PS_LOG_INFO, "%s", pswarp);

    psFree(pslib);
    psFree(psmodules);
    psFree(psphot);
    psFree(ppStats);
    psFree(pswarp);

    return;
}
