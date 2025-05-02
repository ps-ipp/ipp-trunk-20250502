#ifndef PM_DET_EFF_H
#define PM_DET_EFF_H

#define PM_DETEFF_ANALYSIS "DETEFF"     // Location of detection efficiency on pmReadout.analysis

// Detection efficiency characterisation
typedef struct {
    float magRef;                       // Reference magnitude
    int numSources;                     // Number of sources
    int numBins;                        // Number of bins
    psVector *magOffsets;               // Magnitude offsets for each bin
    psVector *counts;                   // Counts of sources recovered for each bin
    psVector *magDiffMean;              // Mean magnitude difference for each bin
    psVector *magDiffStdev;             // Stdev of magnitude difference for each bin
    psVector *magErrMean;               // Mean magnitude error for each bin
} pmDetEff;


/// Allocator
pmDetEff *pmDetEffAlloc(float magRef,   // Reference magnitude
                        int numSources, // Number of sources
                        int numBins     // Number of bins
                        );

/// Write detection efficiency to FITS file
bool pmDetEffWrite(psFits *fits,        // FITS file to which to write
                   pmDetEff *deteff,    // Detection efficiency to write
                   const psMetadata *header, // Header to write
                   const char *extname  // Extension name
                   );

/// Write detection efficiency from a readout to a FITS file
bool pmReadoutWriteDetEff(psFits *fits,// FITS file to which to write
                          const pmReadout *readout, // Readout with detection efficiency
                          const psMetadata *header, // Header to write
                          const char *extname // Extension name
    );

/// Read detection efficiency
pmDetEff *pmDetEffRead(psFits *fits,    // FITS file from which to read
                       const char *extname // Extension name
                       );

/// Read detection efficiency into a readout
bool pmReadoutReadDetEff(psFits *fits,// FITS file to which to write
                         const pmReadout *readout, // Readout with detection efficiency
                         const char *extname // Extension name
    );

#define PM_ASSERT_DETEFF_NON_NULL(DE, RETURN) { \
    if (!(DE)) { \
        psError(PS_ERR_UNEXPECTED_NULL, true, "Detection efficiency %s is NULL", #DE); \
        return RETURN; \
    } \
}

#define PM_ASSERT_DETEFF_RESULTS(DE, RETURN) { \
    PM_ASSERT_DETEFF_NON_NULL(DE, RETURN); \
    PS_ASSERT_VECTOR_NON_NULL((DE)->magOffsets, RETURN); \
    PS_ASSERT_VECTOR_SIZE((DE)->magOffsets, (long)(DE)->numBins, RETURN); \
    PS_ASSERT_VECTOR_TYPE((DE)->magOffsets, PS_TYPE_F32, RETURN); \
    PS_ASSERT_VECTOR_NON_NULL((DE)->counts, RETURN); \
    PS_ASSERT_VECTOR_SIZE((DE)->counts, (long)(DE)->numBins, RETURN); \
    PS_ASSERT_VECTOR_TYPE((DE)->counts, PS_TYPE_S32, RETURN); \
    PS_ASSERT_VECTOR_NON_NULL((DE)->magDiffMean, RETURN); \
    PS_ASSERT_VECTOR_SIZE((DE)->magDiffMean, (long)(DE)->numBins, RETURN); \
    PS_ASSERT_VECTOR_TYPE((DE)->magDiffMean, PS_TYPE_F32, RETURN); \
    PS_ASSERT_VECTOR_NON_NULL((DE)->magDiffStdev, RETURN); \
    PS_ASSERT_VECTOR_SIZE((DE)->magDiffStdev, (long)(DE)->numBins, RETURN); \
    PS_ASSERT_VECTOR_TYPE((DE)->magDiffStdev, PS_TYPE_F32, RETURN); \
    PS_ASSERT_VECTOR_NON_NULL((DE)->magErrMean, RETURN); \
    PS_ASSERT_VECTOR_SIZE((DE)->magErrMean, (long)(DE)->numBins, RETURN); \
    PS_ASSERT_VECTOR_TYPE((DE)->magErrMean, PS_TYPE_F32, RETURN); \
}

#endif
