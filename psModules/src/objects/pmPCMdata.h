/* @file  pmPCMdata.h
 * structures and functions to support PSF-convolved model fitting
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.29 $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2010 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_PCM_DATA_H
# define PM_PCM_DATA_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

// XXX this is basically for testing -- when I am happy with the convolution process, I'll strip this out
# define USE_1D_CACHE 0
# define USE_1D_GAUSS 1

/** pmPCMdata : PSF Convolved Model data storage structure
 *
 * Structure to carry the data needed to generate a PSF-convolve model fit
 *
 */
// 
typedef struct {
    psImage *modelFlux;
    psArray *dmodelsFlux;
    psImage *modelConvFlux;
    psArray *dmodelsConvFlux;

    pmModel *modelConv;
    psKernel *psf;
    psKernelFFT *psfFFT;

    psMinConstraint *constraint;
    int nPix;
    int nPar;
    int nDOF;

    bool poissonErrors;

    bool use1Dgauss;
    float kappa;
    float sigma;
    float nsigma;

    // psArray *smdata;
    psImageSmoothCacheData *smdata;
    psImageSmooth2dCacheData *smdata2d;
} pmPCMdata;

// structures & functions to support psf-convolved model fitting

// psf-convolved model fitting
bool psphotModelWithPSF_LMM (
    psMinimization *min,
    psImage *covar,
    psVector *params,
    psMinConstraint *constraint,
    pmSource *source,
    const psKernel *psf,
    psMinimizeLMChi2Func func);

psF32 psphotModelWithPSF_SetABX(
    psImage  *alpha,
    psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    pmPCMdata *pcm,
    const pmSource *source,
    const psKernel *psf,
    psMinimizeLMChi2Func func);

pmPCMdata *pmPCMdataAlloc (
    const psVector *params,
    const psVector *paramMask,
    pmSource *source);

pmPCMdata *pmPCMinit(pmSource *source, pmSourceFitOptions *fitOptions, pmModel *model, psImageMaskType maskVal, float psfSize);
bool pmPCMupdate(pmPCMdata *pcm, pmSource *source, pmSourceFitOptions *fitOptions, pmModel *model);

psImage *pmPCMdataSaveImage (pmPCMdata *pcm);

psF32 pmPCM_SetABX(
    psImage  *alpha,
    psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    pmPCMdata *pcm,
    const pmSource *source);

bool pmPCM_MinimizeChisq (
    psMinimization *min,
    psImage *covar,
    psVector *params,
    pmSource *source,
    pmPCMdata *pcm);

psKernel *pmPCMkernelFromPSF (pmSource *source, int nPix);

bool pmSourceModelGuessPCM (pmPCMdata *pcm, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);

bool pmSourceFitPCM (pmPCMdata *pcm, pmSource *source, pmSourceFitOptions *fitOptions, psImageMaskType maskVal, psImageMaskType markVal, int psfSize);

bool pmPCMCacheModel (pmSource *source, psImageMaskType maskVal, int psfSize, float nsigma);

bool pmPCMMakeModel (pmSource *source, pmModel *model, float Nsigma, psImageMaskType maskVal, int psfSize);

/// @}
# endif /* PM_PCM_DATA_H */
