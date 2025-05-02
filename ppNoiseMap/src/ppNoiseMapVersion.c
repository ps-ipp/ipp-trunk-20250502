#include "ppNoiseMap.h"
#include "ppNoiseMapVersionDefinitions.h"

#ifndef PPNOISEMAP_VERSION
#error "PPNOISEMAP_VERSION is not set"
#endif
#ifndef PPNOISEMAP_BRANCH
#error "PPNOISEMAP_BRANCH is not set"
#endif
#ifndef PPNOISEMAP_SOURCE
#error "PPNOISEMAP_SOURCE is not set"
#endif

psString ppNoiseMapVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPNOISEMAP_BRANCH, PPNOISEMAP_VERSION);
    return value;
}

psString ppNoiseMapSource(void)
{
    return psStringCopy(PPNOISEMAP_SOURCE);
}

psString ppNoiseMapVersionLong(void)
{
    psString version = ppNoiseMapVersion();  // Version, to return
    psString source = ppNoiseMapSource(); // Source

    psStringPrepend(&version, "ppNoiseMap ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppNoiseMapVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppNoiseMap at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppNoiseMapVersion(); // ppNoiseMap software version
    psString source  = ppNoiseMapSource();  // ppNoiseMap software source

    psMetadataAddStr(header, PS_LIST_TAIL, "NOISEM_V", PS_META_REPLACE, NULL, PPNOISEMAP_VERSION);
    
    psStringPrepend(&version, "ppNoiseMap version: ");
    psStringPrepend(&source, "ppNoiseMap source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}


void ppNoiseMapVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppNoiseMap", PS_LOG_INFO, "ppNoiseMap at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppNoiseMap = ppNoiseMapVersionLong(); // ppNoiseMap version

    psLogMsg("ppNoiseMap", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppNoiseMap", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppNoiseMap", PS_LOG_INFO, "%s", ppNoiseMap);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppNoiseMap);

    return;
}
