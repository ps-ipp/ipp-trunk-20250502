/* @file  pmPSF.h
 *
 * This file contains typedefs for the Point-Spread Function and prototypes
 * for functions that calculate the PSF.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.22 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_PSF_H
# define PM_PSF_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmPSF data structure
 *
 * It is useful to generate a model to define the point-spread-function which
 * describes the flux distribution for unresolved sources in an image. In
 * general, the PSF varies with position in the image. We allow any of the source
 * models defined for the pmModel to represent the PSF. For a given source model,
 * the 2D spatial variation of all of the source parameters, except the first
 * four PSF-independent parameters, are represented as polynomial, stored in a
 * psArray. The other elements of the structure define the quality of the PSF
 * determination.
 *
 */
struct pmPSF {
    pmModelType type;                   ///< PSF Model in use

    float chisq;                        ///< PSF goodness statistic (unused??)
    float ApResid;                      ///< apMag - psfMag (for PSF stars)
    float dApResid;                     ///< scatter of ApResid
    float skyBias;                      ///< implied residual sky offset from ApResid fit
    float skySat;                       ///< roll-over of ApResid fit
    int nPSFstars;                      ///< number of stars used to measure PSF
    int nApResid;                       ///< number of stars used to measure ApResid

    bool poissonErrorsPhotLMM;          ///< use poission errors for non-linear model fitting
    bool poissonErrorsPhotLin;          ///< use poission errors for linear model fitting
    bool poissonErrorsParams;           ///< use poission errors for model parameter fitting

    pmTrend2D *ApTrend;                 ///< ApResid vs (x,y)
    pmTrend2D *FluxScale;               ///< Flux for PSF at (x,y) for normalization = 1.0
    psPolynomial1D *ChiTrend;           ///< Chisq vs flux fit (correction for systematic errors)

    pmGrowthCurve *growth;              ///< apMag vs Radius
    pmResiduals *residuals;             ///< normalized residual image (no spatial variation)

    psArray *params;                    ///< Model parameters (psPolynomial2D)
    psStats *psfTrendStats;             ///< psf parameter trend clipping stats

    pmTrend2DMode psfTrendMode;
    int trendNx;
    int trendNy;
    int fieldNx;
    int fieldNy;
    int fieldXo;
    int fieldYo;
};

typedef struct {
    pmModelType   type;
    psStats      *stats;                // psfTrend clipping stats

    pmTrend2DMode psfTrendMode;
    int           psfTrendNx;
    int           psfTrendNy;
    int           psfFieldNx;
    int           psfFieldNy;
    int           psfFieldXo;
    int           psfFieldYo;

    bool          poissonErrorsPhotLMM; ///< use poission errors for non-linear model fitting
    bool          poissonErrorsPhotLin; ///< use poission errors for linear model fitting
    bool          poissonErrorsParams; ///< use poission errors for model parameter fitting

    bool          chiFluxTrend;         // Fit a trend in Chi2 as a function of flux?
    pmSourceFitOptions *fitOptions;

    float         fitRadius;
    float         apRadius;
} pmPSFOptions;

# define PM_PAR_E0 PM_PAR_SXX
# define PM_PAR_E1 PM_PAR_SYY
# define PM_PAR_E2 PM_PAR_SXY

/**
 *
 * Allocator for the pmPSF structure.
 *
 */

pmPSF *pmPSFAlloc (const pmPSFOptions *options);
bool psMemCheckPSF(psPtr ptr);
pmPSFOptions *pmPSFOptionsAlloc(void);
bool psMemCheckPSFOptions(psPtr ptr);

double pmPSF_SXYfromModel (psF32 *modelPar);
double pmPSF_SXYtoModel (psF32 *fittedPar);

pmPSF *pmPSFBuildSimple (char *typeName, float sxx, float syy, float sxy, ...);

bool pmPSF_AxesToModel (psF32 *modelPar, psEllipseAxes axes, bool useReff);
bool pmPSF_FitToModel (psF32 *fittedPar, float minMinorAxis, bool useReff);

psEllipsePol pmPSF_ModelToFit (psF32 *modelPar, bool useReff);
psEllipseAxes pmPSF_ModelToAxes (psF32 *modelPar, bool useReff);

/// Calculate FWHM value from a PSF
float pmPSFtoFWHM(
    const pmPSF *psf,                   // PSF of interest
    float x, float y                    // Position of interest
    );


/// @}
# endif
