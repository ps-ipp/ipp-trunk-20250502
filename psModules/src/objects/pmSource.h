/* @file  pmSource.h
 *
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.29 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_H
# define PM_SOURCE_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmSourceType enumeration
 *
 * A given source may be identified as most-likely to be one of several source
 * types. The pmSource entry pmSourceType defines the current best-guess for this
 * source.
 *
 */
typedef enum {
    PM_SOURCE_TYPE_UNKNOWN,             ///< not yet classified
    PM_SOURCE_TYPE_DEFECT,              ///< a cosmic-ray
    PM_SOURCE_TYPE_SATURATED,           ///< random saturated pixels (eg, bleed trails)
    PM_SOURCE_TYPE_STAR,                ///< a good-quality star (subtracted model is PSF)
    PM_SOURCE_TYPE_EXTENDED,            ///< an extended object (eg, galaxy) (subtracted model is EXT)
} pmSourceType;

typedef enum {
    PM_SOURCE_TMPF_MODEL_GUESS       = 0x0001,
    PM_SOURCE_TMPF_SUBTRACTED        = 0x0002,
    PM_SOURCE_TMPF_SIZE_MEASURED     = 0x0004,
    PM_SOURCE_TMPF_SIZE_CR_CANDIDATE = 0x0008,
    PM_SOURCE_TMPF_MOMENTS_MEASURED  = 0x0010,
    PM_SOURCE_TMPF_CANDIDATE_PSFSTAR = 0x0020,
    PM_SOURCE_TMPF_RADIAL_KEEP       = 0x0040,
    PM_SOURCE_TMPF_RADIAL_SKIP       = 0x0080,
    PM_SOURCE_TMPF_PETRO_KEEP        = 0x0100,
    PM_SOURCE_TMPF_PETRO_SKIP        = 0x0200,
    PM_SOURCE_TMPF_EXT_FIT           = 0x0400,  // not just galaxies (trails as well)
    PM_SOURCE_TMPF_PETRO             = 0x0800,
} pmSourceTmpF;

/** pmSource data structure
 *
 *  This source has the capacity for several types of measurements. The
 *  simplest measurement of a source is the location and flux of the peak pixel
 *  associated with the source:
 *
 * a pmSource is the information about a (possible) blob of flux in a specific image.  A source
 * may represent an insignificant or undetected source.  There may be multiple representations
 * of an image (eg, alternate smoothed copies); sources on alternate images may have a pointer
 * to the version on the primary image (source->parent).  A set of sources on different, but
 * related images (eg, multiple exposures or different filters) which (may) represent the same
 * astronomical object are grouped together with the pmPhotObj type (set pmPhotObj.h).
 * 
 * A single source may be fitted with multiple models (not at the same time!).  The PSF model
 * fit is a fit of the position (optionally) and the flux to the PSF model at the location of
 * the source.  Alternate model fits are extended source models. The best model fit is used to
 * subtract the object from the image.
 *
 *  XXX do I have to re-organize this (again!) to allow an arbitrary set of extended model fits??
 *  XXX put the Mag and Err inside the pmModel?
 *  XXX keep the modelEXT or add to the psArray
 *
 *
 */
struct pmSource {
    const int id;                       ///< Unique ID for object (generated on alloc)
    int seq;                            ///< ID for output (generated on write OR set on read)
    pmPeak  *peak;                      ///< Description of peak pixel.
    psImage *pixels;                    ///< Rectangular region including object pixels.
    psImage *variance;			///< Image variance.
    psImage *modelVar;			///< variance based on current models
    psImage *maskObj;                   ///< unique mask for this object which marks included pixels associated with objects.
    psImage *maskView;                  ///< view into global image mask for this object region
    psImage *modelFlux;                 ///< cached copy of the best model for this source
    psImage *psfImage;			///< cached copy of the psf model for this source
    pmMoments *moments;                 ///< Basic moments measured for the object.
    pmModel *modelPSF;                  ///< PSF Model fit (parameters and type)
    pmModel *modelEXT;                  ///< EXT Model fit used for subtraction (parameters and type)
    psArray *modelFits;                 ///< collection of extended source models (best == modelEXT)
    psArray *extFitPars;		///< extra extended fit parameters
    pmSourceType type;                  ///< Best identification of object.
    pmSourceMode mode;                  ///< analysis flags set for object.
    pmSourceMode2 mode2;                ///< analysis flags set for object.
    pmSourceTmpF tmpFlags;              ///< internal-only flags

    float psfMag;                       ///< calculated from flux in modelPSF
    float psfMagErr;                    ///< error in psfMag
    float psfFlux;                      ///< calculated from flux in modelPSF
    float psfFluxErr;                   ///< error in psfFlux
    float extMag;                       ///< calculated from flux in modelEXT -- NOTE this is not actually used
    float apMag;                        ///< apMag corresponding to psfMag or extMag (depending on type)
    float apMagRaw;                     ///< raw mag in given aperture
    float apRadius;			///< radius for aperture magnitude
    int   apNpixels;			///< number of unmasked pixels in aperture
    float apFlux;                       ///< apFlux corresponding to psfMag or extMag (depending on type)
    float apFluxErr;                    ///< apFluxErr corresponding to psfMag or extMag (depending on type)

    float windowRadius;			///< size of box used for full analysis
    float skyRadius;			///< radius at which profile hits local sky (or goes flat)
    float skyFlux;			///< mean flux per pixel in aperture at which profile hits local sky (or goes flat)
    float skySlope;			///< mean flux slope at which profile hits local sky (or goes flat)

    float pixWeightNotBad;              ///< PSF-weighted coverage of unmasked (not BAD) pixels
    float pixWeightNotPoor;             ///< PSF-weighted coverage of unmasked (not POOR) pixels

    float psfChisq;                     ///< probability of PSF
    float crNsigma;                     ///< Nsigma deviation from PSF to CR
    float extNsigma;                    ///< Nsigma deviation from PSF to EXT
    float sky;				///< The sky at the center of the object 
    float skyErr;			///< The sky error at the center of the object
    float extSN;                        ///< for externally supplied source the kron signal to noise (used by full force)

    psRegion region;                    ///< area on image covered by selected pixels
    psArray *blends;                    ///< collection of sources thought to be confused with object
    pmSourceSatstar *satstar;
    pmSourceExtendedPars *extpars;      ///< extended source parameters
    pmSourceDiffStats *diffStats;       ///< extra parameters for difference detections
    psArray *galaxyFits;                ///< fits to galaxy models (psphotFullForce only)
    pmSourceLensing *lensingOBJ;        ///< lensing moments parameters (per object)
    pmSourceLensing *lensingPSF;        ///< lensing moments parameters (psf, interpolated)
    psArray *radialAper;		///< radial flux in circular apertures
    pmSource *parent;			///< reference to the master source from which this is derived
    psPtr *tmpPtr;                      ///< pointer that may be used to store data in a particular module. e.g. psphotKronIterate.
    short chipNum;                      ///< camera dependent of chip suppling pixels for fullforce source
    short chipX;                        ///< chip space X coord of fullforce source
    short chipY;                        ///< chip space Y coord of fullforce source
    int imageID;
    psU16 nFrames;
};

/** pmPSFClump data structure
 *
 * A collection of object moment measurements can be used to determine
 * approximate object classes. The key to this analysis is the location and
 * statistics (in the second-moment plane,
 *
 */
typedef struct
{
    float X;
    float dX;
    float Y;
    float dY;
    int nStars;
    int nTotal;
    float nSigma;
}
pmPSFClump;

// private macro to set the source ID (a const)
#define P_PM_SOURCE_SET_ID(S,V) { *(int *)&(S)->id = (V); }

/** pmSourceAlloc()
 *
 */
pmSource  *pmSourceAlloc(void);

/** pmSourceCopy()
 *
 */

bool psMemCheckSource(psPtr ptr);

pmSource  *pmSourceCopy(pmSource *source);
pmSource *pmSourceCopyData(pmSource *in);

// free just the pixels for a source, keeping derived data
void pmSourceFreePixels(pmSource *source);

/** pmSourceDefinePixels()
 *
 * Define psImage subarrays for the source located at coordinates x,y on the
 * image set defined by readout. The pixels defined by this operation consist of
 * a square window (of full width 2Radius+1) centered on the pixel which contains
 * the given coordinate, in the frame of the readout. The window is defined to
 * have limits which are valid within the boundary of the readout image, thus if
 * the radius would fall outside the image pixels, the subimage is truncated to
 * only consist of valid pixels. If readout->mask or readout->weight are not
 * NULL, matching subimages are defined for those images as well. This function
 * fails if no valid pixels can be defined (x or y less than Radius, for
 * example). This function should be used to define a region of interest around a
 * source, including both source and sky pixels.
 *
 */
bool pmSourceDefinePixels(
    pmSource *mySource,                 ///< source to be re-defined
    const pmReadout *readout,  ///< base the source on this readout
    psF32 x,                            ///< center coords of source
    psF32 y,                            ///< center coords of source
    psF32 Radius                        ///< size of box on source
);

bool pmSourceRedefinePixels (
    pmSource *mySource,   ///< source to be re-defined
    const pmReadout *readout,   ///< base the source on this readout
    psF32 x,     ///< center coords of source
    psF32 y,      ///< center coords of source
    psF32 Radius   ///< size of box on source
);

bool pmSourceRedefinePixelsByRegion (
    pmSource *mySource,   ///< source to be re-defined
    const pmReadout *readout,   ///< base the source on this readout
    psRegion newRegion ///< region for source pixel definition
);

/** pmSourcePSFClump()
 *
 * We use the source moments to make an initial, approximate source
 * classification, and as part of the information needed to build a PSF model for
 * the image. As long as the PSF shape does not vary excessively across the
 * image, the sources which are represented by a PSF (the start) will have very
 * similar second moments. The function pmSourcePSFClump searches a collection of
 * sources with measured moments for a group with moments which are all very
 * similar. The function returns a pmPSFClump structure, representing the
 * centroid and size of the clump in the sigma_x, sigma_y second-moment plane.
 *
 * The goal is to identify and characterize the stellar clump within the
 * sigma_x, sigma_y second-moment plane.  To do this, an image is constructed to
 * represent this plane.  The units of sigma_x and sigma_y are in image pixels. A
 * pixel in this analysis image represents 0.1 pixels in the input image. The
 * dimensions of the image need only be 10 pixels. The peak pixel in this image
 * (above a threshold of half of the image maximum) is found. The coordinates of
 * this peak pixel represent the 2D mode of the sigma_x, sigma_y distribution.
 * The sources with sigma_x, sigma_y within 0.2 pixels of this value are then
 *  * used to calculate the median and standard deviation of the sigma_x, sigma_y
 * values. These resulting values are returned via the pmPSFClump structure.
 *
 * The return value indicates the success (TRUE) of the operation.
 */

pmPSFClump pmSourcePSFClump(
    psImage **savedImage, 
    psRegion *region,                   ///< restrict measurement to specified region
    psArray *source,                    ///< The input pmSource
    float PSF_SN_LIM, 
    float PSF_CLUMP_GRID_SCALE, 
    psF32 SX_MAX, 
    psF32 SY_MAX, 
    psF32 SX_MIN, 
    psF32 SY_MIN, 
    psF32 AR_MAX
);

/** pmSourceRoughClass()
 *
 * Based on the specified data values, make a guess at the source
 * classification. The sources are provides as a psArray of pmSource entries.
 * Definable parameters needed to make the classification are provided to the
 * routine with the psMetadata structure. The rules (in SDRS) refer to values which
 * can be extracted from the metadata using the given keywords. Except as noted,
 * the data type for these parameters are psF32.
 *
 */
bool pmSourceRoughClass(
    psRegion *region,                   ///< restrict measurement to specified region
    psArray *sources,                    ///< The input pmSources
    float PSF_SN_LIM,			 ///< min S/N for source to be used for PSF model
    float PSF_CLUMP_NSIGMA,		 ///< size of region around peak of clump for PSF stars
    pmPSFClump clump,                   ///< Statistics about the PSF clump
    psImageMaskType maskSat             ///< Mask value for saturated pixels
);


/** pmSourceMoments()
 *
 * Measure source moments for the given source, using the value of
 * source.moments.sky provided as the local background value and the peak
 * coordinates as the initial source location. The resulting moment values are
 * applied to the source.moments entry, and the source is returned. The moments
 * are measured within the given circular radius of the source.peak coordinates.
 * The return value indicates the success (TRUE) of the operation.
 *
 */
bool pmSourceMoments(
    pmSource *source, ///< The input pmSource for which moments will be computed
    float radius,     ///< Use a circle of pixels around the peak
    float sigma,      ///< size of Gaussian window function (<= 0.0 -> skip window)
    float minSN,	      ///< minimum pixel significance
    float minKronRadius,      ///< minimum pixel significance
    psImageMaskType maskVal
);

/** pmSourceMoments()
 *
 * Measure 1st moments for the given source, using the peak coordinates as the initial
 * source location. The resulting moment values are applied to the source.moments
 * entry. The moments are measured within the given circular radius of the source.peak
 * coordinates.  The return value indicates the success (TRUE) of the operation.
 *
 */
bool pmSourceMomentsGetCentroid(
  pmSource *source, 
  psF32 radius, 
  psF32 sigma, 
  psF32 minSN, 
  psImageMaskType maskVal, 
  float xGuess, float yGuess);

float pmSourceMinKronRadius(psArray *sources, float PSF_SN_LIM);

pmModel *pmSourceGetModel (bool *isPSF, const pmSource *source);

bool pmSourceAdd (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal);
bool pmSourceSub (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal);
bool pmSourceAddWithOffset (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal, int dx, int dy);
bool pmSourceSubWithOffset (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal, int dx, int dy);

bool pmSourceNoiseOpModel (pmModel *model, pmSource *source, pmModelOpMode mode, float FACTOR, float SIZE, bool add, psImageMaskType maskVal, int dx, int dy);
bool pmSourceNoiseOp (pmSource *source, pmModelOpMode mode, float FACTOR, float SIZE, bool add, psImageMaskType maskVal, int dx, int dy);

bool pmSourceSmoothOp (pmSource *source, pmModelOpMode mode, psImage *target, float sigma, bool add, psImageMaskType maskVal, int dx, int dy);
bool pmSourceSmoothOpModel (pmModel *model, pmSource *source, pmModelOpMode mode, psImage *target, float sigma, bool add, psImageMaskType maskVal, int dx, int dy);

bool pmSourceOp (pmSource *source, pmModelOpMode mode, bool add, psImageMaskType maskVal, int dx, int dy);
bool pmSourceCacheModel (pmSource *source, psImageMaskType maskVal);
bool pmSourceCachePSF (pmSource *source, psImageMaskType maskVal);

bool pmSourcePositionUseMoments(pmSource *source);

int  pmSourceSortByY (const void **a, const void **b);
int  pmSourceSortByX (const void **a, const void **b);
int  pmSourceSortBySeq (const void **a, const void **b);
int  pmSourceSortByParentSeq (const void **a, const void **b);
int  pmSourceSortByFlux (const void **a, const void **b);
int  pmSourceSortByParentFlux (const void **a, const void **b);

pmSourceMode pmSourceModeFromString (const char *name);
char *pmSourceModeToString (const pmSourceMode mode);

psU64 pmSourceMemoryUse(pmSource *source);

/// @}
# endif /* PM_SOURCE_H */
