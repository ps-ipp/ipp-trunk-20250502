/** @file psastroVersion.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "psastroInternal.h"
#include "psastroVersionDefinitions.h"

#ifndef PSASTRO_VERSION
#error "PSASTRO_VERSION is not set"
#endif
#ifndef PSASTRO_BRANCH
#error "PSASTRO_BRANCH is not set"
#endif
#ifndef PSASTRO_SOURCE
#error "PSASTRO_SOURCE is not set"
#endif

psString psastroVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PSASTRO_BRANCH, PSASTRO_VERSION);
    return value;
}

psString psastroSource(void)
{
  return psStringCopy(PSASTRO_SOURCE);
}

psString psastroVersionLong(void)
{
    psString version = psastroVersion();  // Version, to return
    psString source = psastroSource();    // Source

    psStringPrepend(&version, "psastro ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool psastroVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psString version = psastroVersion(); // Software version
    psString source = psastroSource();   // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "ASTRO_V", PS_META_REPLACE, NULL, PSASTRO_VERSION);
    
    psStringPrepend(&version, "psastro version: ");
    psStringPrepend(&source, "psastro source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}

bool psastroVersionHeaderFull(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "psastro at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    ppStatsVersionHeader(header);
    psastroVersionHeader(header);

    return true;
}

void psastroVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("psastro", PS_LOG_INFO, "psastro at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppStats = ppStatsVersionLong(); // ppStats version
    psString psastro = psastroVersionLong(); // psastro version

    psLogMsg("psastro", PS_LOG_INFO, "%s", pslib);
    psLogMsg("psastro", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("psastro", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("psastro", PS_LOG_INFO, "%s", psastro);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppStats);
    psFree(psastro);

    return;
}
