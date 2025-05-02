#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"
#include "ppImageVersionDefinitions.h"

#ifndef PPIMAGE_VERSION
#error "PPIMAGE_VERSION is not set"
#endif
#ifndef PPIMAGE_BRANCH
#error "PPIMAGE_BRANCH is not set"
#endif
#ifndef PPIMAGE_SOURCE
#error "PPIMAGE_SOURCE is not set"
#endif

psString ppImageVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPIMAGE_BRANCH, PPIMAGE_VERSION);
    return value;
}

psString ppImageSource(void)
{
    return psStringCopy(PPIMAGE_SOURCE);
}

psString ppImageVersionLong(void)
{
    psString version = ppImageVersion();  // Version, to return
    psString source = ppImageSource(); // Source

    psStringPrepend(&version, "ppImage ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppImageVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppImage at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    psphotVersionHeader(header);
    psastroVersionHeader(header);
    ppStatsVersionHeader(header);

    psString version = ppImageVersion(); // ppImage software version
    psString source  = ppImageSource();  // ppImage software source

    psMetadataAddStr(header, PS_LIST_TAIL, "IMAGE_V", PS_META_REPLACE, NULL, PPIMAGE_VERSION);
    
    psStringPrepend(&version, "ppImage version: ");
    psStringPrepend(&source, "ppImage source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}


void ppImageVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppImage", PS_LOG_INFO, "ppImage at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString psphot = psphotVersionLong(); // psphot version
    psString psastro = psastroVersionLong(); // psastro version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString ppImage = ppImageVersionLong(); // ppImage version

    psLogMsg("ppImage", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", psphot);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", psastro);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("ppImage", PS_LOG_INFO, "%s", ppImage);

    psFree(pslib);
    psFree(psmodules);
    psFree(psphot);
    psFree(psastro);
    psFree(ppStats);
    psFree(ppImage);

    return;
}
