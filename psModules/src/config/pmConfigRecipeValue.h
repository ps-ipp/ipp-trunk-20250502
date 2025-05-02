#ifndef PM_CONFIG_RECIPE_VALUE_H
#define PM_CONFIG_RECIPE_VALUE_H

#include <pslib.h>

#include "pmConfig.h"
#include "pmFPA.h"
#include "pmFPAview.h"

/// Return a recipe value according to the provided view (i.e., chip- and/or cell-dependent)
psMetadataItem *pmConfigRecipeValueByView(const pmConfig *config, // Configuration
                                          const char *recipeName, // Name of recipe
                                          const char *valueName,  // Name of value in recipe
                                          const pmFPA *fpa,       // FPA of interest
                                          const pmFPAview *view   // View to component of interest
    );

#endif
