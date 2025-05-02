#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "psMemory.h"
#include "psConfigure.h"
#include "psMetadata.h"

#include "psMetadataHeader.h"

bool psLibVersionHeader(psMetadata *header)
{
    PS_ASSERT_METADATA_NON_NULL(header, false);

    psString version = psLibVersion();  // Software version
    psString source = psLibSource();    // Software source
    psString revision = psLibRevision();
    psMetadataAddStr(header, PS_LIST_TAIL, "PSLIB_V", PS_META_REPLACE, NULL, revision);
    
    psStringPrepend(&version, "psLib version: ");
    psStringPrepend(&source, "psLib source: ");

    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, version);
    psMetadataAddStr(header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, source);

    psFree(version);
    psFree(source);
    psFree(revision);
    return true;
}
