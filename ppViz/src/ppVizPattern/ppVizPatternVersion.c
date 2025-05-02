/** @file ppVizPatternVersion.c
 *
 *  @brief
 *
 *  @ingroup ppVizPattern
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

#include "ppVizPattern.h"
#include "ppVizPatternVersionDefinitions.h"

#ifndef PPVIZPATTERN_VERSION
#error "PPVIZPATTERN_VERSION is not set"
#endif
#ifndef PPVIZPATTERN_BRANCH
#error "PPVIZPATTERN_BRANCH is not set"
#endif
#ifndef PPVIZPATTERN_SOURCE
#error "PPVIZPATTERN_SOURCE is not set"
#endif

psString ppVizPatternVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPVIZPATTERN_BRANCH, PPVIZPATTERN_VERSION);
    return value;
}

psString ppVizPatternSource(void)
{
    return psStringCopy(PPVIZPATTERN_SOURCE);
}

psString ppVizPatternVersionLong(void)
{
    psString version = ppVizPatternVersion();  // Version, to return
    psString source = ppVizPatternSource();    // Source

    psStringPrepend(&version, "ppVizPattern ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppVizPatternVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppVizPattern at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppVizPatternVersion(); // Software version
    psString source  = ppVizPatternSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "VIZPAT_V", PS_META_REPLACE, NULL, PPVIZPATTERN_VERSION);
    
    psStringPrepend(&version, "ppVizPattern version: ");
    psStringPrepend(&version, "ppVizPattern source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppVizPatternVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppVizPattern", PS_LOG_INFO, "ppVizPattern at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppVizPattern = ppVizPatternVersionLong(); // ppVizPattern version

    psLogMsg("ppVizPattern", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppVizPattern", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppVizPattern", PS_LOG_INFO, "%s", ppVizPattern);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppVizPattern);

    return;
}
