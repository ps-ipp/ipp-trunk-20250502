#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmConceptsPhotcode.h"

psString pmConceptsPhotcodeForView(pmFPAfile *file, const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(file, NULL);
    PS_ASSERT_PTR_NON_NULL(view, NULL);

    if (view->chip < -1) {
        psError(PS_ERR_IO, true, "Photcodes undefined for FPA: defined by chip\n");
        return NULL;
    }

    // select photcode rule from camera configuration
    bool mdok;                          // Status of MD lookup
    char *rule = psMetadataLookupStr(&mdok, file->camera, "PHOTCODE.RULE");
    if (!mdok || !rule || strlen(rule) == 0) {
        psError(PS_ERR_IO, true, "PHOTCODE.RULE not found in camera configuration.");
        return NULL;
    }

    // convert rule to real photcode
    psString photcode = pmFPAfileNameFromRule(rule, file, view);

    return photcode;
}
