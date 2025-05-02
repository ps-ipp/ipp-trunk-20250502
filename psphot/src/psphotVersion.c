#include "psphotInternal.h"
#include "psphotVersionDefinitions.h"

#ifdef HAVE_KAPA
#include <kapa.h>
#endif

#ifndef PSPHOT_VERSION
#error "PSPHOT_VERSION is not set"
#endif
#ifndef PSPHOT_BRANCH
#error "PSPHOT_BRANCH is not set"
#endif
#ifndef PSPHOT_SOURCE
#error "PSPHOT_SOURCE is not set"
#endif

psString psphotVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PSPHOT_BRANCH, PSPHOT_VERSION);
    return value;
}

psString psphotSource(void)
{
    return psStringCopy(PSPHOT_SOURCE);
}

psString psphotVersionLong(void)
{
    psString version = psphotVersion();  // Version, to return
    psString source = psphotSource();    // Source

    psStringPrepend(&version, "psphot ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

#ifdef HAVE_KAPA
#if 0
    // XXX Need to get ohana and libdvo versions
    psString ohanaVersion = psStringStripCVS(ohana_version(), "Name");
    psString libdvoVersion = psStringStripCVS(libdvo_version(), "Name");
    psStringAppend(&version, " with libkapa (ohana %s, libdvo: %s)", ohanaVersion, libdvoVersion);
    psFree(ohanaVersion);
    psFree(libdvoVersion);
#else
    psStringAppend(&version, " with libkapa");
#endif

#else
    psStringAppend (&version, " without libkapa");
#endif

    return version;
}

bool psphotVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psString version = psphotVersion(); // Software version
    psString source = psphotSource();   // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "PHOT_V", PS_META_REPLACE, NULL, PSPHOT_VERSION);
    
    psStringPrepend(&version, "psphot version: ");
    psStringPrepend(&source, "psphot source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}


bool psphotVersionHeaderFull(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "psphot at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    psphotVersionHeader(header);

    return true;
}


void psphotVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("psphot", PS_LOG_INFO, "psphot at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString psphot = psphotVersionLong(); // psphot version

    psLogMsg("psphot", PS_LOG_INFO, "%s", pslib);
    psLogMsg("psphot", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("psphot", PS_LOG_INFO, "%s", psphot);

    psFree(pslib);
    psFree(psmodules);
    psFree(psphot);

    return;
}
