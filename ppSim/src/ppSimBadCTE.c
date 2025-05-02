# include "ppSim.h"

// Add a "bad CTE" region to the image : it is not really modeled as bad CTE, rather it is
// simply smoothed by a Gaussian kernel.  This has the same effect on the image variance as bad
// CTE, but not (quite) the same effect on stellar photometry

bool ppSimBadCTE(psImage *image,	// Signal image, modified and returned
		 const pmConfig *config // configuration
  )
{
    assert(image->type.type == PS_TYPE_F32);

    bool status;

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSIM_RECIPE); // Recipe
    if (!psMetadataLookupBool (&status, recipe, "BADCTE")) {
	return true;
    }
    
    char *region = psMetadataLookupStr (&status, recipe, "BADCTE.REGION");
    psRegion badCTE = psRegionForImage (image, psRegionFromString (region));
    if (psRegionIsNaN (badCTE)) psAbort("analysis region mis-defined");

    float sigma = psMetadataLookupF32 (&status, recipe, "BADCTE.SIGMA");
    int Nsigma = psMetadataLookupS32 (&status, recipe, "BADCTE.NSIGMA");

    psImage *subset = psImageSubset (image, badCTE);
    if (!subset) psAbort ("error in badCTE region?");

    psImageSmooth (subset, sigma, Nsigma);

    return true;
}

