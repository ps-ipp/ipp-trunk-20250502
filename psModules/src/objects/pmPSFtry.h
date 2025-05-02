/* @file  pmPSFtry.h
 *
 * This file contains code that allows the user to try to fit several
 * PSF models to an image.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.22 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_PSF_TRY_H
# define PM_PSF_TRY_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/**
 *
 * This structure contains a pointer to the collection of sources which will
 * be used to test the PSF model form. It lists the pmModelType type of model
 * being tests, and contains an element to store the resulting psf
 * representation. In addition, this structure carries the complete collection of
 * EXT (floating parameter) and PSF (fixed parameter) model fits to each of the
 * sources modelEXT and modelPSF. It also contains a mask which is set by the
 * model fitting and psf fitting steps. For each model, the value of the quality
 * metric is stored in the vector metric and the fitted instrumental magnitude is
 * stored in fitMag. The quality metric for the PSF model is the aperture
 * magnitude minus the fitted magnitude for each source. This collection of
 * aperture residuals is examined in the analysis process, and a linear trend of
 * the residual with the inverse object flux (ie, 100:4his structure contains a
 * pointer to the collection of sources which will be used to test the PSF model
 * form. It lists the pmModelType type of modmag) is fitted. The result of this
 * fit is a measured sky bias (systematic error in the sky measured by the fits),
 * an effective infinite-magnitude aperture correction (ApResid), and the scatter
 * of the aperture correction for the ensemble of PSF stars (dApResid). The
 * ultimate metric to intercompare multiple types of PSF models is the value of
 * the aperture correction scatter.
 *
 * XXX: There are many more members in the SDRS then in the prototype code.
 * I stuck with the prototype code.
 *
 *
 */
typedef struct
{
    pmPSF      *psf;                    ///< Add comment.
    psArray    *sources;                ///< pointers to the original sources
    psVector   *mask;                   ///< PS_TYPE_VECTOR_MASK to flag good and bad sources 
    psVector   *metric;                 ///< Add comment.
    psVector   *metricErr;              ///< Add comment.
    psVector   *fitMag;                 ///< Add comment.
}
pmPSFtry;


/** pmPSFtryMaskValues
 *
 * The following datatype defines the masks used by the pmPSFtry analysis to
 * identify sources which should or should not be included in the analysis.
 *
 */
typedef enum {
    PSFTRY_MASK_CLEAR    = 0x00,        ///< Add comment.
    PSFTRY_MASK_OUTLIER  = 0x01,        ///< 1: outlier in psf polynomial fit (provided by psPolynomials)
    PSFTRY_MASK_EXT_FAIL = 0x02,        ///< 2: ext model failed to converge
    PSFTRY_MASK_PSF_FAIL = 0x04,        ///< 3: psf model failed to converge
    PSFTRY_MASK_BAD_PHOT = 0x08,        ///< 4: invalid source photometry
    PSFTRY_MASK_BAD_MODEL= 0x10,        ///< 5: could not build PSF from EXT (!??)
    PSFTRY_MASK_ALL      = 0x1f,        ///< Add comment.
} pmPSFtryMaskValues;


/** pmPSFtryAlloc()
 *
 * Allocate a pmPSFtry data structure.
 *
 */

pmPSFtry *pmPSFtryAlloc (const psArray *sources, const pmPSFOptions *options);
bool psMemCheckPSFtry(psPtr ptr);

/** pmPSFtryModel()
 *
 * This function takes the input collection of sources and performs a complete
 * analysis to determine a PSF model of the given type (specified by model name).
 * The result is a pmPSFtry with the results of the analysis.
 *
 */
pmPSFtry *pmPSFtryModel (
    const psArray *sources,		///< PSF sources to use in the pmPSF model analysis
    const char *modelName,  		///< human-readable name of desired model
    pmPSFOptions *options, 
    psImageMaskType maskVal, 
    psImageMaskType mark
    );

/** fit EXT models to all possible psf sources */
bool pmPSFtryFitEXT (pmPSFtry *psfTry, pmPSFOptions *options, psImageMaskType maskVal, psImageMaskType markVal);
bool pmPSFtryFitEXT_Threaded (psThreadJob *job);

bool pmPSFtryMakePSF (bool *pGoodFit, pmPSFtry *psfTry);

bool pmPSFtryFitPSF (pmPSFtry *psfTry, pmPSFOptions *options, psImageMaskType maskVal, psImageMaskType markVal);
bool pmPSFtryFitPSF_Threaded (psThreadJob *job);

bool pmPSFThreads (void);

/** pmPSFtryMetric()
 *
 * This function is used to measure the PSF model metric for the set of
 * results contained in the pmPSFtry structure.
 *
 */
bool pmPSFtryMetric(pmPSFtry *psfTry);

/** pmPSFtryMetric_Alt()
 *
 * This function is used to measure the PSF model metric for the set of
 * results contained in the pmPSFtry structure (alternative implementation).
 *
 */
bool pmPSFtryMetric_Alt(
    pmPSFtry *try,                      ///< Add comment.
    float RADIUS                        ///< Add comment.
);

bool pmPSFFitShapeParams (bool *pGoodFit, pmPSF *psf, psArray *sources, psVector *x, psVector *y, psVector *srcMask);

float psVectorSystematicError (psVector *residuals, psVector *errors, float clipFraction);

/// @}
# endif

/**
 *
 * This function takes a collection of pmModel fitted models from across a
 * single image and builds a pmPSF representation of the PSF. The input array of
 * model fits may consist of entries to be ignored (noted by a non-zero mask
 * entry). The analysis of the models fits a 2D polynomial for each parameter to
 * the collection of model parameters as a function of position (and
 * normalization?). In this process, some of the input models may be marked as
 * outliers and excluded from the fit. These elements will be marked with a
 * specific mask value (1 == PSFTRY_MASK_OUTLIER).
 *
 */

