#ifndef PM_SUBTRACTION_SIMPLE_H
#define PM_SUBTRACTION_SIMPLE_H

#include <pslib.h>

#include <pmSubtractionKernels.h>
#include <pmSubtractionStamps.h>
#include <pmSubtraction.h>

bool pmSubtractionSimpleMatch(pmReadout *conv1,
			      pmReadout *conv2,
			      const pmReadout *ro1,
			      const pmReadout *ro2,
			      const psArray *sources,
			      int size,
			      psImageMaskType maskVal,
			      psImageMaskType maskBad,
			      psImageMaskType maskPoor,
			      float deconvolveThreshold
			      );


#endif
