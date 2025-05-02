#ifndef PM_SUBTRACTION_ANALYSIS_H
#define PM_SUBTRACTION_ANALYSIS_H

#include <pslib.h>
#include <pmSubtractionKernels.h>

// Names for things put on the readout analysis metadata
#define PM_SUBTRACTION_ANALYSIS_KERNEL       "SUBTRACTION.KERNEL"       // Kernel used for convolving
#define PM_SUBTRACTION_ANALYSIS_KERNEL_IMAGE "SUBTRACTION.KERNEL.IMAGE" // Image with kernel realisations
#define PM_SUBTRACTION_ANALYSIS_STAMPS       "SUBTRACTION.STAMPS"       // Number of stamps
#define PM_SUBTRACTION_ANALYSIS_DEV_MEAN     "SUBTRACTION.DEV.MEAN"     // Mean stamp deviation
#define PM_SUBTRACTION_ANALYSIS_DEV_RMS      "SUBTRACTION.DEV.RMS"      // RMS stamp deviation
#define PM_SUBTRACTION_ANALYSIS_MODE         "SUBTRACTION.MODE"         // Subtraction mode
#define PM_SUBTRACTION_ANALYSIS_REGION       "SUBTRACTION.REGION"       // Subtraction region
#define PM_SUBTRACTION_ANALYSIS_VARFACTOR_1  "SUBTRACTION.VARFACTOR.1"  // Variance factor for image 1
#define PM_SUBTRACTION_ANALYSIS_VARFACTOR_2  "SUBTRACTION.VARFACTOR.2"  // Variance factor for image 2
#define PM_SUBTRACTION_ANALYSIS_NORM         "SUBTRACTION.NORM"         // Normalisation
#define PM_SUBTRACTION_ANALYSIS_BGDIFF       "SUBTRACTION.BGDIFF"       // Background difference
#define PM_SUBTRACTION_ANALYSIS_MX           "SUBTRACTION.MX"           // Kernel moment in x
#define PM_SUBTRACTION_ANALYSIS_MY           "SUBTRACTION.MY"           // Kernel moment in y
#define PM_SUBTRACTION_ANALYSIS_MXX          "SUBTRACTION.MXX"          // Kernel moment in xx
#define PM_SUBTRACTION_ANALYSIS_MXY          "SUBTRACTION.MXY"          // Kernel moment in xy
#define PM_SUBTRACTION_ANALYSIS_MYY          "SUBTRACTION.MYY"          // Kernel moment in yy

#define PM_SUBTRACTION_ANALYSIS_CONVOL_MAX   "SUBTRACTION.CONVOL.MAX"   // Maximum Convolution fraction
#define PM_SUBTRACTION_ANALYSIS_DECONV_MAX   "SUBTRACTION.DECONV.MAX"   // Maximum Deconvolution fraction

#define PM_SUBTRACTION_ANALYSIS_FRES_SIGMA_MEAN  "SUBTRACTION.FRES.MEAN" // RMS stamp deviation
#define PM_SUBTRACTION_ANALYSIS_FRES_SIGMA_STDEV "SUBTRACTION.FRES.STDEV" // RMS stamp deviation
#define PM_SUBTRACTION_ANALYSIS_FRES_OUTER_MEAN  "SUBTRACTION.FRES.OUTER.MEAN" // RMS stamp deviation
#define PM_SUBTRACTION_ANALYSIS_FRES_OUTER_STDEV "SUBTRACTION.FRES.OUTER.STDEV"	// RMS stamp deviation
#define PM_SUBTRACTION_ANALYSIS_FRES_TOTAL_MEAN  "SUBTRACTION.FRES.TOTAL.MEAN" // RMS stamp deviation
#define PM_SUBTRACTION_ANALYSIS_FRES_TOTAL_STDEV "SUBTRACTION.FRES.TOTAL.STDEV"	// RMS stamp deviation

// Derive QA information about the subtraction
bool pmSubtractionAnalysis(
    psMetadata *analysis,               ///< Metadata container for QA information
    psMetadata *header,                 ///< Metadata container for QA information to put in header
    pmSubtractionKernels *kernels,      ///< Kernels
    psRegion *region,                   ///< Region for subtraction
    int numCols, int numRows            ///< Size of image
    );


#endif
