#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <math.h>
#include "psMemory.h"
#include "psError.h"
#include "psAssert.h"
#include "psConstants.h"
#include "psRegion.h"

static void regionFree(psRegion *region)
{
    // There are non dynamic allocated items
}

psRegion *psRegionAlloc(float x0,
                        float x1,
                        float y0,
                        float y1)
{
    psRegion *region = psAlloc(sizeof(psRegion)); // New region, to be returned
    psMemSetDeallocator(region, (psFreeFunc)regionFree);
    // No complex structures, so no special deallocator
    *region = psRegionSet(x0, x1, y0, y1);
    return region;
}

psRegion psRegionSet(float x0,
                     float x1,
                     float y0,
                     float y1)
{
    psRegion out;

    out.x0 = x0;
    out.y0 = y0;
    out.x1 = x1;
    out.y1 = y1;

    return out;
}

psRegion psRegionFromString(const char* region)
{
    psS32 col0;
    psS32 col1;
    psS32 row0;
    psS32 row1;

    // section should be of the form '[col0:col1,row0:row1]'
    if (region == NULL) {
        return psRegionSet(0,0,0,0);
    }

    if (sscanf(region,"[%d:%d,%d:%d]",&col0,&col1,&row0,&row1) < 4) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified subsection string, '%s', can not be parsed.  Must be in the form '[x1:x2,y1:y2]'."),
                region);
        return psRegionSet(NAN,NAN,NAN,NAN);
    }

    // [0:0,0:0] is complete image region
    if ((col0 == 0) && (col1 == 0) && (row0 == 0) && (row1 == 0)) {
        return psRegionSet(0,0,0,0);
    }

    if ((col1 > 0) && (col0 > col1)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified subset range, [%d:%d,%d:%d], is invalid.  Ranges must be incremental."),
                col0,col1,row0,row1);
        return psRegionSet(NAN,NAN,NAN,NAN);
    }

    if ((row1 > 0) && (row0 > row1)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified subset range, [%d:%d,%d:%d], is invalid.  Ranges must be incremental."),
                col0,col1,row0,row1);
        return psRegionSet(NAN,NAN,NAN,NAN);
    }

    return psRegionSet(col0-1,col1,row0-1,row1);
}

psRegion psRegionAndParityFromString(int *xParity, int *yParity, const char* region)
{
    PS_ASSERT_PTR_NON_NULL (xParity, psRegionSet(NAN,NAN,NAN,NAN));
    PS_ASSERT_PTR_NON_NULL (yParity, psRegionSet(NAN,NAN,NAN,NAN));

    psS32 col0;
    psS32 col1;
    psS32 row0;
    psS32 row1;

    // unless otherwise detected
    *xParity = +1;
    *yParity = +1;

    // section should be of the form '[col0:col1,row0:row1]'
    if (region == NULL) {
        return psRegionSet(0,0,0,0);
    }

    // XXX this is perhaps excessive: if the string is empty, assume 0,0,0,0
    if (*region == 0) {
        return psRegionSet(0,0,0,0);
    }

    if (sscanf(region,"[%d:%d,%d:%d]",&col0,&col1,&row0,&row1) < 4) {
	// psError(PS_ERR_BAD_PARAMETER_NULL, true,
	// ("Specified subsection string, '%s', can not be parsed.  Must be in the form '[x1:x2,y1:y2]'."),
	// region);
        // return psRegionSet(NAN,NAN,NAN,NAN);
        return psRegionSet(0,0,0,0);
    }

    // [0:0,0:0] is complete image region
    if ((col0 == 0) && (col1 == 0) && (row0 == 0) && (row1 == 0)) {
        return psRegionSet(0,0,0,0);
    }

    if ((col1 > 0) && (col0 > col1)) {
	*xParity = -1;
	PS_SWAP (col0, col1);
    }

    if ((row1 > 0) && (row0 > row1)) {
	*yParity = -1;
	PS_SWAP (row0, row1);
    }

    return psRegionSet(col0-1,col1,row0-1,row1);
}

psString psRegionToString(const psRegion region)
{
    char *result = NULL;

    // [0:0,0:0] is complete image region
    if ((region.x0 == 0) && (region.x1 == 0) && (region.y0 == 0) && (region.y1 == 0)) {
        psStringAppend(&result, "[0:0,0:0]");
    } else {
        psStringAppend(&result, "[%g:%g,%g:%g]",
                       region.x0+1, region.x1,
                       region.y0+1, region.y1);
    }
    return result;
}

// define a square region centered on the given coordinate
psRegion psRegionForSquare(double x,
                           double y,
                           double radius)
{
    psRegion region;
    region = psRegionSet (x - radius, x + radius + 1,
                          y - radius, y + radius + 1);
    return (region);
}

bool psRegionIsNaN(psRegion region)
{
    return isnan(region.x0) || isnan(region.x1) || isnan(region.y0) || isnan(region.y1);
}

bool psMemCheckRegion(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)regionFree );
}


