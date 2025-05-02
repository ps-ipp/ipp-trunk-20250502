#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "ppTranslateVersionDefinitions.h"

#ifndef PPTRANSLATE_VERSION
#error "PPTRANSLATE_VERSION is not set"
#endif
#ifndef PPTRANSLATE_BRANCH
#error "PPTRANSLATE_BRANCH is not set"
#endif
#ifndef PPTRANSLATE_SOURCE
#error "PPTRANSLATE_SOURCE is not set"
#endif

psString ppTranslateVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPTRANSLATE_BRANCH, PPTRANSLATE_VERSION);
    return value;
}

psString ppTranslateSource(void)
{
    return psStringCopy(PPTRANSLATE_SOURCE);
}

psString ppTranslateVersionLong(void)
{
    psString version = ppTranslateVersion();  // Version, to return
    psString source = ppTranslateSource();    // Source

    psStringPrepend(&version, "ppTranslate ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};


bool ppTranslateVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppTranslate at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);

    psString version = ppTranslateVersion(); // Software version
    psString source  = ppTranslateSource();  // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "TRANSL_V", PS_META_REPLACE, NULL, PPTRANSLATE_VERSION);
    
    psStringPrepend(&version, "ppTranslate version: ");
    psStringPrepend(&source, "ppTranslate source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);
    psFree(time);

    return true;
}

void ppTranslateVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppTranslate", PS_LOG_INFO, "ppTranslate at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString ppTranslate = ppTranslateVersionLong(); // ppTranslate version

    psLogMsg("ppTranslate", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppTranslate", PS_LOG_INFO, "%s", ppTranslate);

    psFree(pslib);
    psFree(ppTranslate);

    return;
}
