/* @file  pmSourceFitModel.h
 *
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_FIT_MODEL_H
# define PM_SOURCE_FIT_MODEL_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef enum {
    PM_SOURCE_FIT_NORM,
    PM_SOURCE_FIT_PSF,
    PM_SOURCE_FIT_EXT,
    PM_SOURCE_FIT_PSF_AND_SKY,
    PM_SOURCE_FIT_EXT_AND_SKY,
    PM_SOURCE_FIT_INDEX,
    PM_SOURCE_FIT_SHAPE,
    PM_SOURCE_FIT_NO_INDEX,
    PM_SOURCE_FIT_TRAIL,
} pmSourceFitMode;

typedef struct {
    pmSourceFitMode mode;		///< optionally fit all or a subset of parameters
    float nIter;			///< max number of allowed iterations
    float minTol;			///< convergence criterion
    float maxTol;			///< convergence criterion
    float maxChisqDOF;			///< convergence criterion
    float weight;			///< use this weight for constant-weight fits
    float covarFactor;			///< covariance factor for calculating the chisq
    float nsigma;                       ///< how far out to convolve
    bool poissonErrors;			///< use poisson errors for fits?
    bool saveCovariance;
    int gainFactorMode;
    bool chisqConvergence; 
    bool isInteractive;
    bool useReweighting;
} pmSourceFitOptions;

// the pmSourceFitOptions structure is used to control details of the fitting process
pmSourceFitOptions *pmSourceFitOptionsAlloc(void);

// bool pmSourceFitModelInit(
//     pmSourceFitMode mode,		///< what parameter set should be fitted?
//     float nIter,			///< max number of allowed iterations
//     float tol,				///< convergence criterion
//     float weight,			///< use this weight for constant-weight fits
//     bool poissonErrors			///< use poisson errors for fits?
// );

/** pmSourceFitModel()
 *
 * Fit the requested model to the specified source. The starting guess for the model is given
 * by the input source.model parameter values. The pixels of interest are specified by the
 * source.pixels and source.mask entries. This function calls psMinimizeLMChi2() on the image
 * data. The function returns TRUE on success or FALSE on failure.
 *
 */
bool pmSourceFitModel(
    pmSource *source,			///< The input pmSource
    pmModel *model,			///< model to be fitted
    pmSourceFitOptions *options,	///< define parameters to be fitted
    psImageMaskType maskVal		///< Value to mask
);

// // initialize data for a group of object models
// bool pmSourceFitSetInit (pmModelType type);
// 
// // clear data for a group of object models
// void pmSourceFitSetClear (void);
// 
// // function used to set limits for a group of models
// bool pmSourceFitSet_CheckLimits (psMinConstraintMode mode, int nParam, float *params, float *betas);
// 
// // function used to fit a group of object models
// psF32 pmSourceFitSet_Function(psVector *deriv,
//                               const psVector *params,
//                               const psVector *x);
// 
// /** pmSourceFitSet()
//  *
//  * Fit the requested model to the specified source. The starting guess for the model is given
//  * by the input source.model parameter values. The pixels of interest are specified by the
//  * source.pixels and source.mask entries. This function calls psMinimizeLMChi2() on the image
//  * data. The function returns TRUE on success or FALSE on failure.
//  *
//  */
// bool pmSourceFitSet(
//     pmSource *source,   ///< The input pmSource
//     psArray *modelSet,   ///< model to be fitted
//     pmSourceFitMode mode,  ///< define parameters to be fitted
//     psImageMaskType maskVal		///< Vale to mask
// 
// );

/// @}
# endif /* PM_SOURCE_FIT_MODEL_H */
