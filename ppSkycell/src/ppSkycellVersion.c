/** @file ppSkycellVersion.c
 *
 *  @brief
 *
 *  @ingroup ppSkycell
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

#include "ppSkycell.h"
#include "ppSkycellVersionDefinitions.h"

#ifndef PPSKYCELL_VERSION
#error "PPSKYCELL_VERSION is not set"
#endif
#ifndef PPSKYCELL_BRANCH
#error "PPSKYCELL_BRANCH is not set"
#endif
#ifndef PPSKYCELL_SOURCE
#error "PPSKYCELL_SOURCE is not set"
#endif

psString ppSkycellVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPSKYCELL_BRANCH, PPSKYCELL_VERSION);
    return value;
}

psString ppSkycellSource(void)
{
    return psStringCopy(PPSKYCELL_SOURCE);
}

psString ppSkycellVersionLong(void)
{
    psString version = ppSkycellVersion();  // Version, to return
    psString source = ppSkycellSource();    // Source

    psStringPrepend(&version, "ppSkycell ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppSkycellVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppSkycell at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppSkycellVersion(); // Software version
    psString source  = ppSkycellSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "SKYCELL_V", PS_META_REPLACE, NULL, PPSKYCELL_VERSION);
    
    psStringPrepend(&version, "ppSkycell version: ");
    psStringPrepend(&version, "ppSkycell source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);
    
    return true;
}

void ppSkycellVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppSkycell", PS_LOG_INFO, "ppSkycell at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppSkycell = ppSkycellVersionLong(); // ppSkycell version

    psLogMsg("ppSkycell", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppSkycell", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppSkycell", PS_LOG_INFO, "%s", ppSkycell);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppSkycell);

    return;
}
