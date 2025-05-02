/* @file  pmModelClass.h
 *
 * The object model function types are desined to allow for the flexible addition of new
 * object models. Every object model, with parameters represented by pmModel, has an
 * associated set of functions which provide necessary support operations.  A These
 * functions allow the programmer to select the approriate function or property for a
 * specific object model class.
 *
 * Every model instance belongs to a class of models, defined by the value of the
 * pmModelType type entry. Various functions need access to information about each of the
 * models. Some of this information varies from model to model, and may depend on the
 * current parameter values or other data quantities. In order to keep the code from
 * requiring the information about each model to be coded into the low-level fitting
 * routines, we define a collection of functions which allow us to abstract this type of
 * model-dependent information. These generic functions take the model type and return the
 * corresponding function pointer for the specified model. Each model is defined by
 * creating this collection of specific functions, and placing them in a single file for
 * each model. We define the following structure to carry the collection of information
 * about the models.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-11-27 03:14:57 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_MODEL_FUNCS_H
# define PM_MODEL_FUNCS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

// type of model carried by the pmModel structure
typedef int pmModelType;

typedef enum {
    PM_MODEL_STATUS_NONE           = 0x0000, ///< model fit not yet attempted, no other info
    PM_MODEL_STATUS_FITTED         = 0x0001, ///< model fit completed
    PM_MODEL_STATUS_NONCONVERGE    = 0x0002, ///< model fit did not converge
    PM_MODEL_STATUS_OFFIMAGE       = 0x0004, ///< model fit drove out of range
    PM_MODEL_STATUS_BADARGS        = 0x0008, ///< model fit called with invalid args
    PM_MODEL_STATUS_LIMITS         = 0x0010, ///< model parameters hit limits
    PM_MODEL_STATUS_WEAK_FIT       = 0x0020, ///< model fit met loose tolerance, but not tight tolerance
    PM_MODEL_STATUS_NAN_CHISQ      = 0x0040, ///< model fit failed with a NAN chisq 
    PM_MODEL_SERSIC_PCM_FAIL_GUESS = 0x0080, ///< sersic model fit failed on the initial moments-based guess
    PM_MODEL_SERSIC_PCM_FAIL_GRID  = 0x0100, ///< sersic model fit failed on the grid search
    PM_MODEL_PCM_FAIL_GUESS        = 0x0200, ///< non-sersic model fit failed on the initial moments-based guess
    PM_MODEL_BEST_FIT              = 0x0400, ///< this model was the best fit and was subtracted
    PM_MODEL_STATUS_NAN_SHAPE      = 0x0800, ///< model ellipse parameters do not transform to valid ellipse
    PM_MODEL_STATUS_NAN_SHAPE_ERR  = 0x1000, ///< model ellipse parameters errors do not transform to valid ellipse
    PM_MODEL_STATUS_FAIL_SHAPE_ERR = 0x2000, ///< could not find an MC solution for ellipse parameter errors
} pmModelStatus;

typedef enum {
    PM_MODEL_OP_NONE     = 0x00,
    PM_MODEL_OP_FUNC     = 0x01,
    PM_MODEL_OP_RES0     = 0x02,
    PM_MODEL_OP_RES1     = 0x04,
    PM_MODEL_OP_FULL     = 0x07,
    PM_MODEL_OP_SKY      = 0x08,
    PM_MODEL_OP_CENTER   = 0x10,
    PM_MODEL_OP_NORM     = 0x20,
    PM_MODEL_OP_NOISE    = 0x40,
    PM_MODEL_OP_MODELVAR = 0x80,
} pmModelOpMode;

/// Parameter limit types
typedef enum {
    PM_MODEL_LIMITS_NONE,               ///< Apply no limits: suitable for debugging
    PM_MODEL_LIMITS_IGNORE,             ///< Ignore all limits: fit can go to town
    PM_MODEL_LIMITS_LAX,                ///< Lax limits: attempting to reproduce even bad data
    PM_MODEL_LIMITS_MODERATE,           ///< Moderate limits: cope with mildly bad data
    PM_MODEL_LIMITS_STRICT,             ///< Strict limits: insist on good quality data
} pmModelLimitsType;

/** Symbolic names for the elements of [d]params
 * Note: these are #defines not enums as a given element of [d]params
 * may/will correspond to different parameters in different contexts
 */
#define PM_PAR_SKY  0   ///< Sky
#define PM_PAR_I0   1   ///< Central intensity
#define PM_PAR_XPOS 2   ///< X center of object
#define PM_PAR_YPOS 3   ///< Y center of object
#define PM_PAR_SXX  4   ///< shape X^2 moment
#define PM_PAR_SYY  5   ///< shape Y^2 moment
#define PM_PAR_SXY  6   ///< shape XY moment
#define PM_PAR_7    7   ///< Model-dependent parameter
#define PM_PAR_8    8   ///< Model-dependent parameter

// these are used by pmModel_TRAIL, with refers to L and Theta explicitly
#define PM_PAR_LENGTH 4 ///< trail length
#define PM_PAR_THETA  5 ///< position angle
#define PM_PAR_SIGMA  6 ///< position angle

/*** these prototype classes are used to define elements of the pmModelClass structure below ***/
 
typedef struct pmModel  pmModel;
typedef struct pmSource pmSource;
typedef struct pmPSF    pmPSF;

//  This function is the model chi-square minimization function for this model.
typedef psMinimizeLMChi2Func pmModelFunc;

//  This function sets the parameter limits for this model.
typedef psMinimizeLMLimitFunc pmModelLimits;

// This function returns the integrated flux for the given model parameters.
typedef psF64 (*pmModelFlux)(const psVector *params);

// This function returns the radius at which the given model and parameters
// achieves the given flux.
typedef psF64 (*pmModelRadius)(const psVector *params, double flux);

// This function returns the FWHM given the supplied sigma (major or minor)
typedef psF64 (*pmModelSetFWHM)(const psVector *params, double sigma);

//  This function provides the model guess parameters based on the details of
//  the given source.
typedef bool (*pmModelGuessFunc)(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);

//  This function constructs the PSF model for the given source based on the
//  supplied psf and the EXT model for the object.
typedef bool (*pmModelFromPSFFunc)(pmModel *modelPSF, pmModel *modelEXT, const pmPSF *psf);

//  This function sets the model parameters based on the PSF for a given coordinate and central
//  intensity
typedef bool (*pmModelParamsFromPSF)(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);

//  This function returns the success / failure status of the given model fit
typedef bool (*pmModelFitStatusFunc)(pmModel *model);

//  This function sets the parameter limits for the given model
typedef bool (*pmModelSetLimitsFunc)(pmModelLimitsType type);

/// @}
# endif /* PM_MODEL_FUNCS_H */
