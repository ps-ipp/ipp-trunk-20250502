#include "ppSmooth.h"
#include "ppSmoothVersionDefinitions.h"

#ifndef PPSMOOTH_VERSION
#error "PPSMOOTH_VERSION is not set"
#endif
#ifndef PPSMOOTH_BRANCH
#error "PPSMOOTH_BRANCH is not set"
#endif
#ifndef PPSMOOTH_SOURCE
#error "PPSMOOTH_SOURCE is not set"
#endif

psString ppSmoothVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPSMOOTH_BRANCH, PPSMOOTH_VERSION);
    return value;
}

psString ppSmoothSource(void)
{
    return psStringCopy(PPSMOOTH_SOURCE);
}

psString ppSmoothVersionLong(void)
{
    psString version = ppSmoothVersion();  // Version, to return
    psString source = ppSmoothSource(); // Source

    psStringPrepend(&version, "ppSmooth ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

// not thread safe (use outside of threaded areas)
bool ppSmoothVersionUpdateHeader (pmFPA *fpa, pmChip *chip, pmCell *cell) {

    static pmHDU *lastHDU = NULL;

    // Put version information into the header
    pmHDU *hdu = pmHDUGetHighest(fpa, chip, cell);
    if (hdu == lastHDU) return false;

    ppSmoothVersionHeader(hdu->header);
    lastHDU = hdu;
    return true;
}

bool ppSmoothVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString history = NULL;               // History string
    psStringAppend(&history, "ppSmooth at %s", timeString);
    psFree(timeString);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
    psFree(history);

    psLibVersionHeader(header);
    psModulesVersionHeader(header);

    psString version = ppSmoothVersion(); // ppSmooth software version
    psString source  = ppSmoothSource();  // ppSmooth software source

    psMetadataAddStr(header, PS_LIST_TAIL, "SMOOTH_V", PS_META_REPLACE, NULL, PPSMOOTH_VERSION);
    
    psStringPrepend(&version, "ppSmooth version: ");
    psStringPrepend(&source, "ppSmooth source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);
    
    return true;
}


void ppSmoothVersionPrint(void)
{
    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psLogMsg("ppSmooth", PS_LOG_INFO, "ppSmooth at %s", timeString);
    psFree(timeString);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppSmooth = ppSmoothVersionLong(); // ppSmooth version

    psLogMsg("ppSmooth", PS_LOG_INFO, "%s", pslib);
    psLogMsg("ppSmooth", PS_LOG_INFO, "%s", psmodules);
    psLogMsg("ppSmooth", PS_LOG_INFO, "%s", ppSmooth);

    psFree(pslib);
    psFree(psmodules);
    psFree(ppSmooth);

    return;
}
