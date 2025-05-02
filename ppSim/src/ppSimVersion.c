#include "ppSim.h"
#include "ppSimVersionDefinitions.h"

#ifndef PPSIM_VERSION
#error "PPSIM_VERSION is not set"
#endif
#ifndef PPSIM_BRANCH
#error "PPSIM_BRANCH is not set"
#endif
#ifndef PPSIM_SOURCE
#error "PPSIM_SOURCE is not set"
#endif

psString ppSimVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PPSIM_BRANCH, PPSIM_VERSION);
    return value;
}

psString ppSimSource(void)
{
    return psStringCopy(PPSIM_SOURCE);
}

psString ppSimVersionLong(void)
{
    psString version = ppSimVersion();  // Version, to return
    psString source = ppSimSource();    // Source

    psStringPrepend(&version, "ppSim ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

    return version;
};
