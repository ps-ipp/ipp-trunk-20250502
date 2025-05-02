/* @file  pmModel.h
 * @brief Functions to define and manipulate object models
 *
 * @author GLG, MHPCC
 * @author EAM, IfA
 *
 * @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 *
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_MODEL_H
# define PM_MODEL_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/* pointers for the functions types below are supplied to each pmModel, and can be used by
   the programmer without needing to know the model class */

/** pmModel data structure
 *
 * Every source may have two types of models: a PSF model and a EXT (extended-source)
 * model. The PSF model represents the best fit of the image PSF to the specific
 * object. In this case, the PSF-dependent parameters are specified for the
 * object by the PSF, not by the fit. The EXT model represents the best fit of
 * the given model to the object, with all shape parameters floating in the fit.
 *
 */
struct pmModel {
    pmModelType type;                   ///< Model to be used.
    psVector *params;                   ///< Paramater values.
    psVector *dparams;                  ///< Parameter errors.
    psImage *covar;                     ///< Optional covariance matrix
    float chisq;                        ///< Fit chi-squared.
    float chisqNorm;                    ///< re-normalized fit chi-squared.
    float mag;                          ///< integrated model magnitude
    float magErr;                       ///< integrated model magnitude error
    int nPix;                           ///< number of pixels used for fit
    int nPar;                           ///< number of parameters in fit
    int nDOF;                           ///< number of degrees of freedom (nDOF = nPix - nPar)
    int nIter;                          ///< number of iterations to reach min
    pmModelStatus flags;                ///< model status flags
    float fitRadius;                    ///< fit radius actually used
    pmResiduals *residuals;             ///< normalized PSF residuals
    bool isPCM;				///< is this model fitted with PSF-convolution?

    pmModelClass *class;

    // functions for this model which depend on the model class
    
    // pmModelFunc          modelFunc;
    // pmModelFlux          modelFlux;
    // pmModelRadius        modelRadius;
    // pmModelLimits        modelLimits;
    // pmModelGuessFunc     modelGuess;
    // pmModelFromPSFFunc   modelFromPSF;
    // pmModelParamsFromPSF modelParamsFromPSF;
    // pmModelFitStatusFunc modelFitStatus;
    // pmModelSetLimitsFunc modelSetLimits;
};

/** pmModelAlloc()
 *
 */
pmModel *pmModelAlloc(pmModelType type);
bool psMemCheckModel(psPtr ptr);

// copy model to a new structure
pmModel *pmModelCopy (pmModel *model);

psF32 pmModelEval(pmModel *model, psImage *image, psS32 col, psS32 row);
psF32 pmModelEvalWithOffset(pmModel *model, psImage *image, psS32 col, psS32 row, int dx, int dy);

/** pmModelAdd()
 *
 * Add the given source model flux to/from the provided image. The boolean
 * option center selects if the source is re-centered to the image center or if
 * it is placed at its centroid location. The boolean option sky selects if the
 * background sky is applied (TRUE) or not. The pixel range in the target image
 * is at most the pixel range specified by the source.pixels image. The success
 * status is returned.
 *
 */
bool pmModelAdd(
    psImage *image,                     ///< The output image (float)
    psImage *mask,                      ///< The image pixel mask (valid == 0)
    pmModel *model,                     ///< The input pmModel
    pmModelOpMode mode,                 ///< mode to control how the model is added into the image
    psImageMaskType maskVal             ///< Value to mask
);

/** pmModelSub()
 *
 * Subtract the given source model flux to/from the provided image. The
 * boolean option center selects if the source is re-centered to the image center
 * or if it is placed at its centroid location. The boolean option sky selects if
 * the background sky is applied (TRUE) or not. The pixel range in the target
 * image is at most the pixel range specified by the source.pixels image. The
 * success status is returned.
 *
 */
bool pmModelSub(
    psImage *image,                     ///< The output image (float)
    psImage *mask,                      ///< The image pixel mask (valid == 0)
    pmModel *model,                     ///< The input pmModel
    pmModelOpMode mode,                 ///< mode to control how the model is added into the image
    psImageMaskType maskVal             ///< Value to mask
);

bool pmModelAddWithOffset(psImage *image,
                          psImage *mask,
                          pmModel *model,
                          pmModelOpMode mode,
                          psImageMaskType maskVal,
                          int dx,
                          int dy);

bool pmModelSubWithOffset(psImage *image,
                          psImage *mask,
                          pmModel *model,
                          pmModelOpMode mode,
                          psImageMaskType maskVal,
                          int dx,
                          int dy);

/** pmModelFitStatus()
 *
 * This function wraps the call to the model-specific function returned by
 * pmModelFitStatusFunc_GetFunction.  The model-specific function examines the
 * model parameters, parameter errors, Chisq, S/N, and other parameters available
 * from model to decide if the particular fit was successful or not.
 *
 * XXX: Must code this.
 *
 */
bool pmModelFitStatus(
    pmModel *model                      ///< Model to be used
);


/// Set the model parameter limits for the given model
///
/// Wraps the model-specific pmModelSetLimitsFunc function.
bool pmModelSetLimits(
    const pmModel *model,               ///< Model of interest
    pmModelLimits type                  ///< Type of limits
    );


/// @}
# endif /* PM_MODEL_H */
