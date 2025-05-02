/** @file fpcameraVersion.c
 *
 *  @brief
 *
 *  @ingroup libfpcamera
 *
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "fpcamera.h"
#include "fpcameraVersionDefinitions.h"

#ifndef FPCAMERA_VERSION
#error "FPCAMERA_VERSION is not set"
#endif
#ifndef FPCAMERA_BRANCH
#error "FPCAMERA_BRANCH is not set"
#endif
#ifndef FPCAMERA_SOURCE
#error "FPCAMERA_SOURCE is not set"
#endif

psString fpcameraVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", FPCAMERA_BRANCH, FPCAMERA_VERSION);
    return value;
}

psString fpcameraSource(void)
{
  return psStringCopy(FPCAMERA_SOURCE);
}

psString fpcameraVersionLong(void)
{
    psString version = fpcameraVersion();  // Version, to return
    psString source = fpcameraSource();    // Source

    psStringPrepend(&version, "fpcamera ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool fpcameraVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psString version = fpcameraVersion(); // Software version
    psString source = fpcameraSource();   // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "FPCAM_V", PS_META_REPLACE, NULL, FPCAMERA_VERSION);
    
    psStringPrepend(&version, "fpcamera version: ");
    psStringPrepend(&source, "fpcamera source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

bool fpcameraVersionHeaderFull(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "fpcamera at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    ppStatsVersionHeader(header);
    fpcameraVersionHeader(header);

    return true;
}

void fpcameraVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("fpcamera", PS_LOG_INFO, "fpcamera at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString fpcamera = fpcameraVersionLong(); // fpcamera version

    psLogMsg("fpcamera", PS_LOG_INFO, "%s", pslib);
    psLogMsg("fpcamera", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("fpcamera", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("fpcamera", PS_LOG_INFO, "%s", fpcamera);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppStats);
    psFree(fpcamera);

    return;
}
