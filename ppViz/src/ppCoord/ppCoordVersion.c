/** @file ppCoordVersion.c
 *
 *  @brief
 *
 *  @ingroup ppCoord
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

#include "ppCoord.h"
#include "ppCoordVersionDefinitions.h"

#ifndef PPCOORD_VERSION
#error "PPCOORD_VERSION is not set"
#endif
#ifndef PPCOORD_BRANCH
#error "PPCOORD_BRANCH is not set"
#endif
#ifndef PPCOORD_SOURCE
#error "PPCOORD_SOURCE is not set"
#endif

psString ppCoordVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPCOORD_BRANCH, PPCOORD_VERSION);
    return value;
}

psString ppCoordSource(void)
{
    return psStringCopy(PPCOORD_SOURCE);
}

psString ppCoordVersionLong(void)
{
    psString version = ppCoordVersion();  // Version, to return
    psString source = ppCoordSource();    // Source

    psStringPrepend(&version, "ppCoord ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppCoordVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppCoord at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppCoordVersion(); // Software version
    psString source  = ppCoordSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "COORD_V", PS_META_REPLACE, NULL, PPCOORD_VERSION);
    
    psStringPrepend(&version, "ppCoord version: ");
    psStringPrepend(&version, "ppCoord source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppCoordVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppCoord", PS_LOG_INFO, "ppCoord at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppCoord = ppCoordVersionLong(); // ppCoord version

    psLogMsg("ppCoord", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppCoord", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppCoord", PS_LOG_INFO, "%s", ppCoord);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppCoord);

    return;
}
