#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoApplyCorr.h"

static const char *cvsTag = "$Name: not supported by cvs2svn $";// CVS tag name

psString dvoApplyCorrVersion(void)
{
    psString version = NULL;            // Version, to return
    psStringAppend(&version, "%s-%s",PACKAGE_NAME,PACKAGE_VERSION);
    return version;
}

psString dvoApplyCorrVersionLong(void)
{
    psString version = dvoApplyCorrVersion(); // Version, to return
    psString tag = psStringStripCVS(cvsTag, "Name"); // CVS tag
    psStringAppend(&version, " (cvs tag %s) %s, %s", tag, __DATE__, __TIME__);
    psFree(tag);
    return version;
}


void dvoApplyCorrVersionMetadata(psMetadata *metadata)
{
    PS_ASSERT_METADATA_NON_NULL(metadata,);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString dvoApplyCorr = dvoApplyCorrVersionLong(); // dvoApplyCorr version

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString head = NULL;               // Head string
    psStringAppend(&head, "dvoApplyCorr processing at %s. Component information:", timeString);
    psFree(timeString);

    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, head, "");
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, pslib, "");
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, psmodules, "");
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, dvoApplyCorr, "");

    psFree(head);
    psFree(pslib);
    psFree(psmodules);
    psFree(dvoApplyCorr);

    return;
}
