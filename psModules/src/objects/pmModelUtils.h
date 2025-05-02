/* @file  pmModelUtils.h
 *
 * Utility functions for working with pmSources
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-12-15 01:22:11 $
 * Copyright 2007 IfA, University of Hawaii
 */

# ifndef PM_MODEL_UTILS_H
# define PM_MODEL_UTILS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/**
 *
 * This function constructs a pmModel instance based on the pmPSF description
 * of the PSF. The input is a pmModel with at least the values of the centroid
 * coordinates (possibly normalization if this is needed) defined. The values of
 * the PSF-dependent parameters are specified for the specific realization based
 * on the coordinates of the object.
 *
 */
pmModel *pmModelFromPSF(
    pmModel *model,                     ///< Add comment
    const pmPSF *psf                    ///< Add comment
);

pmModel *pmModelFromPSFforXY (
    const pmPSF *psf,
    float Xo,
    float Yo,
    float Io
    );

bool pmModelSetFlux (
    pmModel *model,
    float flux
    );

bool pmModelSetPosition (float *Xo, float *Yo, pmSource *source);
bool pmModelSetNorm (float *Io, pmSource *source);
bool pmModelSetShape (float *Sxx, float *Sxy, float *Syy, pmMoments *moments, bool useReff, float Scale);

bool pmModelUseReff (pmModelType type);
bool pmModelAxesToParams (float *Sxx, float *Sxy, float *Syy, psEllipseAxes axes, bool useReff);
bool pmModelParamsToAxes (psEllipseAxes *axes, float Sxx, float Sxy, float Syy, bool useReff);

// XXX void pmModelSetModelVarOption (bool option);
// XXX bool pmModelGetModelVarOption (void);

/// @}
# endif /* PM_MODEL_UTILS_H */
