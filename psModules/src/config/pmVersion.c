#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "pmVersion.h"
#include "pmVersionDefinitions.h"

#ifndef PSMODULES_VERSION
#error "PSMODULES_VERSION is not set"
#endif
#ifndef PSMODULES_BRANCH
#error "PSMODULES_BRANCH is not set"
#endif
#ifndef PSMODULES_SOURCE
#error "PSMODULES_SOURCE is not set"
#endif

psString psModulesVersion(void)
{
    char *value = NULL;
    psStringAppend(&value, "%s@%s", PSMODULES_BRANCH, PSMODULES_VERSION);
    return value;
}

psString psModulesSource(void)
{
    return psStringCopy(PSMODULES_SOURCE);
}


psString psModulesVersionLong(void)
{
    psString version = psModulesVersion();  // Version, to return
    psString source = psModulesSource();    // Source

    psStringPrepend(&version, "psModules ");
    psStringAppend(&version, " from %s, built %s, %s", source, __DATE__, __TIME__);
    psFree(source);

#ifdef __OPTIMIZE__
    psStringAppend(&version, " optimised");
#else
    psStringAppend(&version, " unoptimised");
#endif

#ifdef HAVE_NEBCLIENT
    psStringAppend(&version, " with nebclient");
#else
    psStringAppend(&version, " without nebclient");
#endif

    return version;
};


bool psModulesVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psString version = psModulesVersion(); // Software version
    psString source = psModulesSource();   // Software source

    psMetadataAddStr(header, PS_LIST_TAIL, "MODULE_V", PS_META_REPLACE, NULL, PSMODULES_VERSION);
    
    psStringPrepend(&version, "psModules version: ");
    psStringPrepend(&source, "psModules source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);

    return true;
}
