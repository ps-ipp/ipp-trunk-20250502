/* @file pmSubtraction.h
 *
 * PSF-matched image subtraction, based on the Alard & Lupton (1998) and Alard (2000) methods.
 *
 * @author Paul Price, IfA
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.36 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:25 $
 * Copyright 2004-207 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_SUBTRACTION_TYPES_H
#define PM_SUBTRACTION_TYPES_H

/// @addtogroup imcombine Image Combinations
/// @{

/// Mask values for the subtraction mask
typedef enum {
    PM_SUBTRACTION_MASK_CLEAR          = 0x00, // No masking
    PM_SUBTRACTION_MASK_BAD_1          = 0x01, // Image 1 is bad
    PM_SUBTRACTION_MASK_BAD_2          = 0x02, // Image 2 is bad
    PM_SUBTRACTION_MASK_CONVOLVE_1     = 0x04, // If image 1 is convolved, would be poor or bad
    PM_SUBTRACTION_MASK_CONVOLVE_2     = 0x08, // If image 2 is convolved, would be poor or bad
    PM_SUBTRACTION_MASK_CONVOLVE_BAD_1 = 0x10, // If image 1 is convolved, would be bad
    PM_SUBTRACTION_MASK_CONVOLVE_BAD_2 = 0x20, // If image 2 is convolved, would be bad
    PM_SUBTRACTION_MASK_BORDER         = 0x40, // Image border
    PM_SUBTRACTION_MASK_REJ            = 0x80, // Previously tried as a stamp, and rejected
} pmSubtractionMasks;

/// Type of subtraction kernel
typedef enum {
    PM_SUBTRACTION_KERNEL_NONE,         ///< Nothing --- an error
    PM_SUBTRACTION_KERNEL_POIS,         ///< Pan-STARRS Optimal Image Subtraction --- delta functions
    PM_SUBTRACTION_KERNEL_ISIS,         ///< Traditional kernel --- gaussians modified by polynomials
    PM_SUBTRACTION_KERNEL_ISIS_RADIAL,  ///< ISIS + higher-order radial Hermitians
    PM_SUBTRACTION_KERNEL_HERM,         ///< Hermitian polynomial kernels
    PM_SUBTRACTION_KERNEL_DECONV_HERM,  ///< Deconvolved Hermitian polynomial kernels
    PM_SUBTRACTION_KERNEL_SPAM,         ///< Summed Pixels for Advanced Matching --- summed delta functions
    PM_SUBTRACTION_KERNEL_FRIES,        ///< Fibonacci Radius Increases Excellence of Subtraction
    PM_SUBTRACTION_KERNEL_GUNK,         ///< Grid United with Normal Kernel --- POIS and ISIS hybrid
    PM_SUBTRACTION_KERNEL_RINGS,        ///< Rings Instead of the Normal Gaussian Subtraction
    PM_SUBTRACTION_KERNEL_SIMPLE,       ///< Simple Gaussian kernel to avoid complications
} pmSubtractionKernelsType;

/// Modes --- specifies which image to convolve
typedef enum {
    PM_SUBTRACTION_MODE_ERR,            // Error in the mode
    PM_SUBTRACTION_MODE_1,              // Convolve image 1
    PM_SUBTRACTION_MODE_2,              // Convolve image 2
    PM_SUBTRACTION_MODE_UNSURE,         // deprecated way of choosing the direction
    PM_SUBTRACTION_MODE_SINGLE_AUTO,    // choose between SINGLE1 and SINGLE2
    PM_SUBTRACTION_MODE_DUAL,           // Dual convolution
} pmSubtractionMode;

/// Status of stamp
typedef enum {
    PM_SUBTRACTION_STAMP_INIT,          ///< Initial state
    PM_SUBTRACTION_STAMP_FOUND,         ///< Found a suitable source for this stamp
    PM_SUBTRACTION_STAMP_CALCULATE,     ///< Calculate matrix and vector values for this stamp
    PM_SUBTRACTION_STAMP_USED,          ///< Use this stamp
    PM_SUBTRACTION_STAMP_REJECTED,      ///< This stamp has been rejected
    PM_SUBTRACTION_STAMP_NONE           ///< No stamp in this region
} pmSubtractionStampStatus;

typedef struct {
    double score;
    pmSubtractionMode mode;
    int spatialOrder;
    int nGood;
    psVector *fluxes;
    psVector *chisq;
    psVector *moments;
    psVector *stampMask;
} pmSubtractionQuality;

/// Kernels specification
typedef struct {
    pmSubtractionKernelsType type;      ///< Type of kernels --- allowing the use of multiple kernels
    psString description;               ///< Description of the kernel parameters
    int xMin, xMax, yMin, yMax;         ///< Bounds of image (for normalisation)
    long num;                           ///< Number of kernel components (not including the spatial ones)
    psVector *fwhms;			///< requested fwhms of the kernel Gaussians (ISIS, HERM or DECONV_HERM)
    psVector *orders;                   ///< polynomial orders for each Gaussian (ISIS, HERM or DECONV_HERM)
    psVector *u, *v;                    ///< Offset (for POIS) or polynomial order (for ISIS, HERM or DECONV_HERM)
    psVector *widths;                   ///< measured Gaussian FWHMs of Gauss*poly (ISIS, HERM or DECONV_HERM)
    psVector *uStop, *vStop;            ///< Width of kernel element (SPAM,FRIES only)
    psArray *preCalc;                   ///< Array of images containing pre-calculated kernel (for ISIS, HERM or DECONV_HERM)
    float penalty;                      ///< Penalty for wideness
    psVector *penalties1;               ///< Penalty for each kernel component
    psVector *penalties2;               ///< Penalty for each kernel component
    bool havePenalties;			///< flag to test if we have already calculated the penalties or not.
    int size;                           ///< The half-size of the kernel
    int inner;                          ///< The size of an inner region
    int binning;                        ///< Binning used for the SPAM kernels
    int ringsOrder;			///< 
    int spatialOrder;                   ///< The spatial order of the kernels
    int bgOrder;                        ///< The order for the background fitting
    pmSubtractionMode mode;             ///< Mode for subtraction
    psVector *solution1, *solution2;    ///< Solution for the PSF matching
    psVector *solution1err, *solution2err; ///< error in solution for the PSF matching
    // Quality information
    float mean, rms;                    ///< Mean and RMS of chi^2 from stamps
    int numStamps;                      ///< Number of good stamps
    float fResSigmaMean;		///< mean fractional stdev of residuals
    float fResSigmaStdev;		///< stdev of fractional stdev of residuals
    float fResOuterMean;		///< mean fractional positive swing in residuals
    float fResOuterStdev;		///< stdev of fractional positive swing in residuals
    float fResTotalMean;		///< mean fractional negative swing in residuals
    float fResTotalStdev;		///< stdev of fractional negative swing in residuals
    psArray *sampleStamps;              ///< array of brightest set of stamps for output visualizations
} pmSubtractionKernels;

// pmSubtractionKernels->preCalc is an array of pmSubtractionKernelPreCalc structures
typedef struct {
    psVector *uCoords;                  // used by RINGS
    psVector *vCoords;                  // used by RINGS
    psVector *poly;                     // used by RINGS

    psVector *xKernel;                  // used by ISIS, HERM, DECONV_HERM
    psVector *yKernel;                  // used by ISIS, HERM, DECONV_HERM
    psKernel *kernel;                   // used by ISIS, HERM, DECONV_HERM
} pmSubtractionKernelPreCalc;

/// A list of stamps
typedef struct {
    long num;                           ///< Number of stamps
    psArray *stamps;                    ///< The stamps
    psArray *regions;                   ///< Regions for each stamp
    psArray *x, *y;                     ///< Coordinates for possible stamps (or NULL)
    psArray *flux;                      ///< Fluxes for possible stamps (or NULL)
    int footprint;                      ///< Half-size of stamps
    float normFrac;                     ///< Fraction of flux in window for normalisation window
    float normValue;			///< calculated normalization
    float normValue2;			///< calculated normalization
    psKernel *window1;                  ///< window function generated from ensemble of stamps (input 1)
    psKernel *window2;                  ///< window function generated from ensemble of stamps (input 2)
    psKernel *window;                   ///< weighting window function (sigma = 1.1 * MAX(fwhm))
    float normWindow1;                  ///< Size of window for measuring normalisation
    float normWindow2;                  ///< Size of window for measuring normalisation
    float sysErr;                       ///< Systematic error
    float skyErr;                       ///< increase effective readnoise
} pmSubtractionStampList;

/// A stamp for image subtraction
typedef struct {
    float x, y;                         ///< Position
    float flux;                         ///< Flux
    float xNorm, yNorm;                 ///< Normalised position
    psKernel *image1;                   ///< Reference image postage stamp
    psKernel *image2;                   ///< Input image postage stamp
    psKernel *weight;                   ///< Weight image (1/variance) postage stamp, or NULL
    psArray *convolutions1;             ///< Convolutions of image 1 for each kernel component, or NULL
    psArray *convolutions2;             ///< Convolutions of image 2 for each kernel component, or NULL
    psImage *matrix;                    ///< Least-squares matrix, or NULL
    psVector *vector;                   ///< Least-squares vector, or NULL
    double norm;                        ///< Normalisation difference
    double normI1;                       ///< Sum(flux) for image 1
    double normI2;                       ///< Sum(flux) for image 2
    double normSquare1;                 ///< Sum(flux^2) for image 1 (used for penalty)
    double normSquare2;                 ///< Sum(flux^2) for image 2 (used for penalty)
    pmSubtractionStampStatus status;    ///< Status of stamp
    psVector *MxxI1;			///< second moments of convolution images
    psVector *MyyI1;			///< second moments of convolution images
    psVector *MxxI2;			///< second moments of convolution images
    psVector *MyyI2;			///< second moments of convolution images
    double MxxI1raw;
    double MyyI1raw;
    double MxxI2raw;
    double MyyI2raw;
} pmSubtractionStamp;

#endif
