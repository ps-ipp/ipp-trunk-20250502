#include "ppStack.h"
#include "ppStackVersionDefinitions.h"

#ifndef PPSTACK_VERSION
#error "PPSTACK_VERSION is not set"
#endif
#ifndef PPSTACK_BRANCH
#error "PPSTACK_BRANCH is not set"
#endif
#ifndef PPSTACK_SOURCE
#error "PPSTACK_SOURCE is not set"
#endif

psString ppStackVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPSTACK_BRANCH, PPSTACK_VERSION);
    return value;
}

psString ppStackSource(void)
{
    return psStringCopy(PPSTACK_SOURCE);
}

psString ppStackVersionLong(void)
{
    psString version = ppStackVersion();  // Version, to return
    psString source = ppStackSource();    // Source

    psStringPrepend(&version, "ppStack ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};


bool ppStackVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppStack at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);
    psphotVersionHeader(header);
    ppStatsVersionHeader(header);

    psString version = ppStackVersion(); // Software version
    psString source  = ppStackSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "STACK_V", PS_META_REPLACE, NULL, PPSTACK_VERSION);
    
    psStringPrepend(&version, "ppStack version: ");
    psStringPrepend(&source, "ppStack source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);
    
    return true;
}

void ppStackVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppStack", PS_LOG_INFO, "ppStack at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString psphot = psphotVersionLong(); // psphot version
    psString ppStats = ppStatsVersionLong(); // psastro version
    psString ppStack = ppStackVersionLong(); // ppStack version

    psLogMsg("ppStack", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppStack", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppStack", PS_LOG_INFO, "%s", psphot);
    psLogMsg("ppStack", PS_LOG_INFO, "%s", ppStats);
    psLogMsg("ppStack", PS_LOG_INFO, "%s", ppStack);

    psFree(pslib);
    psFree(psmodules);
    psFree(psphot);
    psFree(ppStats);
    psFree(ppStack);

    return;
}
