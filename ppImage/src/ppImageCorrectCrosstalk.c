#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

#define ESCAPE(MESSAGE) { \
  psError(PS_ERR_UNKNOWN, false, MESSAGE); \
  psFree(view); \
  return false; \
}

// For the moment, this implementation is VERY GPC-specific

bool ppImageCorrectCrosstalk(pmConfig *config, ppImageOptions *options, pmFPAview *view)
{
    return true;
}
