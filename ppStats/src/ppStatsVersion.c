#include "ppStatsInternal.h"
#include "ppStatsVersionDefinitions.h"

#ifndef PPSTATS_VERSION
#error "PPSTATS_VERSION is not set"
#endif
#ifndef PPSTATS_BRANCH
#error "PPSTATS_BRANCH is not set"
#endif
#ifndef PPSTATS_SOURCE
#error "PPSTATS_SOURCE is not set"
#endif

psString ppStatsVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPSTATS_BRANCH, PPSTATS_VERSION);
    return value;
}

psString ppStatsSource(void)
{
    return psStringCopy(PPSTATS_SOURCE);
}

psString ppStatsVersionLong(void)
{
    psString version = ppStatsVersion();  // Version, to return
    psString source = ppStatsSource();    // Source

    psStringPrepend(&version, "ppStats ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};

bool ppStatsVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psString version = ppStatsVersion(); // Software version
    psString source = ppStatsSource();   // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "STATS_V", PS_META_REPLACE, NULL, PPSTATS_VERSION);
    
    psStringPrepend(&version, "ppStats version: ");
    psStringPrepend(&source, "ppStats source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}
