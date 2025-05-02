/** @file ppBackgroundVersion.c
 *
 *  @brief
 *
 *  @ingroup ppBackground
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

#include "ppBackground.h"
#include "ppBackgroundVersionDefinitions.h"

#ifndef PPBACKGROUND_VERSION
#error "PPBACKGROUND_VERSION is not set"
#endif
#ifndef PPBACKGROUND_BRANCH
#error "PPBACKGROUND_BRANCH is not set"
#endif
#ifndef PPBACKGROUND_SOURCE
#error "PPBACKGROUND_SOURCE is not set"
#endif

psString ppBackgroundVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPBACKGROUND_BRANCH, PPBACKGROUND_VERSION);
    return value;
}

psString ppBackgroundSource(void)
{
    return psStringCopy(PPBACKGROUND_SOURCE);
}

psString ppBackgroundVersionLong(void)
{
    psString version = ppBackgroundVersion();  // Version, to return
    psString source = ppBackgroundSource();    // Source

    psStringPrepend(&version, "ppBackground ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppBackgroundVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppBackground at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppBackgroundVersion(); // Software version
    psString source  = ppBackgroundSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "VIZPAT_V", PS_META_REPLACE, NULL, PPBACKGROUND_VERSION);
    
    psStringPrepend(&version, "ppBackground version: ");
    psStringPrepend(&version, "ppBackground source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppBackgroundVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppBackground", PS_LOG_INFO, "ppBackground at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppBackground = ppBackgroundVersionLong(); // ppBackground version

    psLogMsg("ppBackground", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppBackground", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppBackground", PS_LOG_INFO, "%s", ppBackground);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppBackground);

    return;
}
