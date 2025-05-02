/* @file  pmSourcePhotometry.h
 * @brief functions to measure source photometry
 *
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:28:54 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_PHOTOMETRY_H
# define PM_SOURCE_PHOTOMETRY_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/**
 *
 * The function returns both the magnitude of the fit, defined as -2.5log(flux),
 * where the flux is integrated under the model, theoretically from a radius of 0
 * to infinity. In practice, we integrate the model beyond 50sigma.  The aperture magnitude is
 * defined as -2.5log(flux) , where the flux is summed for all pixels which are
 * not excluded by the aperture mask. The model flux is calculated by calling the
 * model-specific function provided by pmModelFlux_GetFunction.
 *
 * XXX: must code this.
 *
 */

typedef enum {
    PM_SOURCE_PHOT_NONE      = 0x0000,
    PM_SOURCE_PHOT_GROWTH    = 0x0001,
    PM_SOURCE_PHOT_APCORR    = 0x0002,
    PM_SOURCE_PHOT_WEIGHT    = 0x0004,
    PM_SOURCE_PHOT_INTERP    = 0x0008,
    PM_SOURCE_PHOT_DIFFSTATS = 0x0010,
    PM_SOURCE_PHOT_PSFONLY   = 0x0020,
} pmSourcePhotometryMode;

typedef enum {
    PM_SOURCE_PHOTFIT_NONE       = 0,
    PM_SOURCE_PHOTFIT_CONST      = 1,
    PM_SOURCE_PHOTFIT_IMAGE_VAR  = 2,
    PM_SOURCE_PHOTFIT_MODEL_VAR  = 3,
    PM_SOURCE_PHOTFIT_MODEL_SKY  = 4,   // XXX bad name: set variance floor based on mean variance image (variance of sky)
} pmSourceFitVarMode;

bool pmSourcePhotometryModel(
    float *fitMag,                      ///< integrated fit magnitude
    float *fitFlux,                     ///< integrated fit magnitude
    pmModel *model                      ///< model used for photometry
);

bool pmSourcePhotometryAper(
    int *nPixOut,
    float *apMag,
    float *apFluxOut,
    float *apFluxErr,
    pmModel *model,                     ///< model used for photometry
    psImage *image,                     ///< image pixels to be used
    psImage *variance,                  ///< variance pixels to be used
    psImage *mask,                      ///< mask of pixels to ignore
    psImageMaskType maskVal             ///< Value to mask
);

bool pmSourcePhotometryAperSource(
    pmSource *source,			///< aperture flux magnitude
    pmModel *model,                     ///< model used for photometry
    psImage *image,                     ///< image pixels to be used
    psImage *variance,                  ///< variance pixels to be used
    psImage *mask,                      ///< mask of pixels to ignore
    psImageMaskType maskVal             ///< Value to mask
);

bool pmSourceMagnitudesInit (pmConfig *config, psMetadata *recipe);
bool pmSourceMagnitudes (pmSource *source, pmPSF *psf, pmSourcePhotometryMode mode, psImageMaskType maskVal, psImageMaskType markVal, float radius);

bool pmSourcePixelWeight (pmSource *source, pmModel *model, psImage *mask, psImageMaskType maskVal, float radius);
bool pmSourceMaskEval (pmSource *source, psImage *mask, psImageMaskType maskVal);

bool pmSourceChisq (pmModel *model, psImage *image, psImage *mask, psImage *weight, psImageMaskType maskVal);
bool pmSourceChisqUnsubtracted (pmSource *source, pmModel *model, psImageMaskType maskVal);

bool pmSourceMeasureDiffStats (pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);

double pmSourceDataDotModel (const pmSource *Mi, const pmSource *Mj, const pmSourceFitVarMode fitVarMode, const float covarFactor, psImageMaskType maskVal);
double pmSourceModelDotModel (const pmSource *Mi, const pmSource *Mj, const pmSourceFitVarMode fitVarMode, const float covarFactor, psImageMaskType maskVal);
double pmSourceModelWeight(const pmSource *Mi, int term, const pmSourceFitVarMode fitVarMode, const float covarFactor, psImageMaskType maskVal);

bool pmSourceNeighborFlags (pmSource *source);

// retire these:
// double pmSourceCrossProduct(const pmSource *Mi, const pmSource *Mj, const bool unweighted_sum);
// double pmSourceCrossWeight(const pmSource *Mi, const pmSource *Mj, const bool unweighted_sum);
// double pmSourceWeight(const pmSource *Mi, int term, const bool unweighted_sum);

/// @}
# endif /* PM_SOURCE_PHOTOMETRY_H */
