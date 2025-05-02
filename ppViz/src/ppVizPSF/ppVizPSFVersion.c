/** @file ppVizPSFVersion.c
 *
 *  @brief
 *
 *  @ingroup ppVizPSF
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

#include "ppVizPSF.h"
#include "ppVizPSFVersionDefinitions.h"

#ifndef PPVIZPSF_VERSION
#error "PPVIZPSF_VERSION is not set"
#endif
#ifndef PPVIZPSF_BRANCH
#error "PPVIZPSF_BRANCH is not set"
#endif
#ifndef PPVIZPSF_SOURCE
#error "PPVIZPSF_SOURCE is not set"
#endif

psString ppVizPSFVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPVIZPSF_BRANCH, PPVIZPSF_VERSION);
    return value;
}

psString ppVizPSFSource(void)
{
    return psStringCopy(PPVIZPSF_SOURCE);
}

psString ppVizPSFVersionLong(void)
{
    psString version = ppVizPSFVersion();  // Version, to return
    psString source = ppVizPSFSource();    // Source

    psStringPrepend(&version, "ppVizPSF ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppVizPSFVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppVizPSF at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppVizPSFVersion(); // Software version
    psString source  = ppVizPSFSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "VIZPSF_V", PS_META_REPLACE, NULL, PPVIZPSF_VERSION);
    
    psStringPrepend(&version, "ppVizPSF version: ");
    psStringPrepend(&version, "ppVizPSF source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

void ppVizPSFVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppVizPSF", PS_LOG_INFO, "ppVizPSF at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppVizPSF = ppVizPSFVersionLong(); // ppVizPSF version

    psLogMsg("ppVizPSF", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppVizPSF", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppVizPSF", PS_LOG_INFO, "%s", ppVizPSF);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppVizPSF);

    return;
}
