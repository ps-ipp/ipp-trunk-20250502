/** @file pswarpOptionss.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.24 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 03:10:36 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

/**
 * Parse the recipe and format into the arguments
 */
bool pswarpOptions(pmConfig *config)
{
    // Select the appropriate recipe
    psMetadata *recipe  = psMetadataLookupPtr(NULL, config->recipes, PSWARP_RECIPE);
    if (!recipe) {
        psError(PSWARP_ERR_CONFIG, true, "Can't find %s recipe!\n", PSWARP_RECIPE);
        return false;
    }

    // Get grid size
    bool status;                        ///< Status of MD lookup
    int nGridX = psMetadataLookupS32(&status, recipe, "GRID.NX");
    if (!status || nGridX <= 0) {
        nGridX = 128;
        psWarning("GRID.NX is not set in the recipe --- defaulting to %d", nGridX);
    }
    int nGridY = psMetadataLookupS32(&status, recipe, "GRID.NY");
    if (!status) {
        nGridY = 128;
        psWarning("GRID.NY is not set in the recipe --- defaulting to %d", nGridY);
    }

    // Get interpolation mode
    const char *name = psMetadataLookupStr (&status, recipe, "INTERPOLATION.MODE"); ///< Name of interp mode
    if (!name) {
        name = "BILINEAR";
        psLogMsg("pswarp", 3, "defaulting to %s interpolation", name);
    }
    psImageInterpolateMode interpolationMode = psImageInterpolateModeFromString(name); ///< Mode for interp.
    if (interpolationMode == PS_INTERPOLATE_NONE) {
        interpolationMode = PS_INTERPOLATE_BILINEAR;
        psLogMsg ("pswarp", 3,
                  "Unknown interpolation mode %s, defaulting to bilinear interpolation\n", name);
        name = "BILINEAR";
    }

    int numKernels = psMetadataLookupS32(&status, recipe, "INTERPOLATION.NUM");
    if (!status) {
        numKernels = 0;
        psWarning("INTERPOLATION.NUM is not set in the recipe --- defaulting to %d", numKernels);
    }

    float poorFrac = psMetadataLookupF32(&status, recipe, "POOR.FRAC"); ///< Frac of bad flux for a "poor"
    if (!status) {
        poorFrac = 0.0;
        psWarning("POOR.FRAC is not set in the %s recipe --- defaulting to %f.", PSWARP_RECIPE, poorFrac);
    }

    bool PSF = psMetadataLookupBool(&status, recipe, "PSF"); ///< Generate a PSF model?
    if (!status) {
        PSF = true;
        psWarning("PSF is not set in the %s recipe --- defaulting to TRUE.", PSWARP_RECIPE);
    }

    bool applyPixelNaN = psMetadataLookupBool(&status,recipe, "APPLY.PIXELNAN"); ///< apply NaN value to masked pixels
    if (!status) {
	applyPixelNaN = true;
	psWarning("APPLY.PIXELNAN is not set in the %s recipe -- defaulting to %d.", PSWARP_RECIPE, applyPixelNaN);
    }
    
    // BACKGROUND.MODEL gets set in config->arguments (to false) if no input model is found
    bool doBKG = psMetadataLookupBool(&status,config->arguments, "BACKGROUND.MODEL"); ///< Generate the warped background model?
    if (!status) {
      // if it is not in config->arguments, look in recipe
      doBKG = psMetadataLookupBool(&status,recipe, "BACKGROUND.MODEL"); ///< Generate the warped background model?
      if (!status) {
	doBKG = false;
      psWarning("BACKGROUND.MODEL is not set in the %s recipe -- defaulting to FALSE.", PSWARP_RECIPE);
      }
    }

    int bkgXgrid = psMetadataLookupS32(&status,recipe, "BKG.XGRID"); ///< Xsize of background model
    if (!status) {
      bkgXgrid = 10;
      psWarning("BKG.XGRID is not set in the %s recipe -- defaultint to %d.",PSWARP_RECIPE,bkgXgrid);
    }
    int bkgYgrid = psMetadataLookupS32(&status,recipe, "BKG.YGRID"); ///< Xsize of background model
    if (!status) {
      bkgYgrid = 10;
      psWarning("BKG.YGRID is not set in the %s recipe -- defaultint to %d.",PSWARP_RECIPE,bkgYgrid);
    }

    // See if we should use a more accurate WCS model.
    int config_additional_orders = psMetadataLookupS32(&status,recipe, "ADDITIONAL_WCS_ORDERS");
    if (!status) {
      // We did not find this recipe option.  We are likely updating an old config.
      config_additional_orders = 0;
    }
    pmAstrometrySetExtraOrders(config_additional_orders);
    // This is the number of orders that should be added to all non-linear inverse transformations
    
    // Set recipe values in the recipe (since we've possibly altered some)
    psMetadataAddS32(recipe, PS_LIST_TAIL, "GRID.NX", PS_META_REPLACE, "Iso-astrom grid spacing in x", nGridX);
    psMetadataAddS32(recipe, PS_LIST_TAIL, "GRID.NY", PS_META_REPLACE, "Iso-astrom grid spacing in y", nGridY);
    psMetadataAddStr(recipe, PS_LIST_TAIL, "INTERPOLATION.MODE", PS_META_REPLACE, "Interpolation mode", name);
    psMetadataAddS32(recipe, PS_LIST_TAIL, "INTERPOLATION.NUM", PS_META_REPLACE, "Interpolation pre-calculated kernels", numKernels);
    psMetadataAddF32(recipe, PS_LIST_TAIL, "POOR.FRAC", PS_META_REPLACE, "Fraction of bad flux for a pixel to be marked as poor", poorFrac);
    psMetadataAddBool(recipe, PS_LIST_TAIL, "PSF", PS_META_REPLACE, "Generate a PSF Model?", PSF);
    psMetadataAddBool(recipe, PS_LIST_TAIL, "BACKGROUND.MODEL", PS_META_REPLACE, "Generate the warped background model?", doBKG);
    psMetadataAddBool(recipe, PS_LIST_TAIL, "APPLY.PIXELNAN", PS_META_REPLACE, "apply NaN values to bad pixels?", applyPixelNaN);
    psMetadataAddS32(recipe, PS_LIST_TAIL, "BKG.XGRID", PS_META_REPLACE, "Xsize of background model", bkgXgrid);
    psMetadataAddS32(recipe, PS_LIST_TAIL, "BKG.YGRID", PS_META_REPLACE, "Ysize of background model", bkgYgrid);
    
    // Set recipe values in the arguments
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "GRID.NX", 0, "Iso-astrom grid spacing in x", nGridX);
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "GRID.NY", 0, "Iso-astrom grid spacing in y", nGridY);
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INTERPOLATION.MODE", 0, "Interpolation mode", interpolationMode);
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "INTERPOLATION.NUM", 0, "Interpolation pre-calculated kernels", numKernels);
    psMetadataAddF32(config->arguments, PS_LIST_TAIL, "POOR.FRAC", 0, "Fraction of bad flux for a pixel to be marked as poor", poorFrac);
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "PSF", PS_META_REPLACE, "Generate a PSF Model?", PSF);
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "BACKGROUND.MODEL", PS_META_REPLACE, "Generate the warped background model?", doBKG);
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "APPLY.PIXELNAN", PS_META_REPLACE, "apply NaN values to bad pixels?", applyPixelNaN);
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "BKG.XGRID", PS_META_REPLACE, "Xsize of background model", bkgXgrid);
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "BKG.YGRID", PS_META_REPLACE, "Ysize of background model", bkgYgrid);
    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "ADDITIONAL_WCS_ORDERS", PS_META_REPLACE, "Additional orders for bilevel fit.", config_additional_orders);

    psTrace("pswarp", 1, "Done with pswarpOptions...\n");

    return (config);
}
