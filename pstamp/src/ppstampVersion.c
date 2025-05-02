#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppstamp.h"

static const char *cvsTag = "$Name: not supported by cvs2svn $";// CVS tag name

psString ppstampVersion(void)
{
    psString version = NULL;            // Version, to return
    psStringAppend(&version, "%s-%s",PACKAGE_NAME,PACKAGE_VERSION);
    return version;
}

psString ppstampVersionLong(void)
{
    psString version = ppstampVersion(); // Version, to return
    psString tag = psStringStripCVS(cvsTag, "Name"); // CVS tag
    psStringAppend(&version, " (cvs tag %s) %s, %s", tag, __DATE__, __TIME__);
    psFree(tag);
    return version;
}


void ppstampVersionMetadata(psMetadata *metadata, ppstampOptions *options)
{
    PS_ASSERT_METADATA_NON_NULL(metadata,);

    psString pslib = psLibVersionLong();// psLib version
    psString psmodules = psModulesVersionLong(); // psModules version
    psString ppstamp = ppstampVersionLong(); // ppstamp version

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now
    psString timeString = psTimeToISO(time); // The time in an ISO string
    psFree(time);
    psString head = NULL;               // Head string
    psStringAppend(&head, "ppstamp processing at %s. Component information:", timeString);
    psFree(timeString);

    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, head, "");
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, pslib, "");
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, psmodules, "");
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, ppstamp, "");
    psString roi = NULL;
    psStringAppend(&roi, "ppstamp: region of interest: %s", psRegionToString(options->roi));
    psMetadataAddStr(metadata, PS_LIST_TAIL, "HISTORY",  PS_META_DUPLICATE_OK, roi, "");

    psFree(roi);
    psFree(head);
    psFree(pslib);
    psFree(psmodules);
    psFree(ppstamp);

    return;
}
