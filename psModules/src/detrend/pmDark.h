#ifndef PM_DARK_H
#define PM_DARK_H

#include <pslib.h>
#include <pmHDU.h>
#include <pmFPA.h>
#include <pmConfig.h>

#define PM_DARK_ANALYSIS_ORDINATES "DARK.ORDINATES" // Name for dark ordinates in the cell analysis metadata
#define PM_DARK_ANALYSIS_NORM "DARK.NORM" // Name for dark normalisation concept in cell analysis metadata
#define PM_DARK_HEADER_NORM "PSDRKNRM"  // Header keyword for dark normalisation concept

// An ordinate for fitting darks
typedef struct {
    psString name;                      // Name of ordinate
    psString rule;                      // Rule for generating ordinate (math on concepts)
    int order;                          // Polynomial order to fit
    bool scale;                         // Rescale values?
    float min, max;                     // Minimum and maximum values for rescaling
} pmDarkOrdinate;

// Allocator
pmDarkOrdinate *pmDarkOrdinateAlloc(const char *name, // Name for ordinate
                                    int order // Order for ordinate
    );


// Combine darks -- preparation step
bool pmDarkCombinePrepare(pmCell *output,      // Output cell; readouts will be attached
                          const psArray *inputs, // Input readouts for combination
                          psArray *ordinates,  // Ordinates for fitting
                          const char *normConcept // Concept name to use to divide input pixel values
    );

// Combine darks -- do the actual work
bool pmDarkCombine(pmCell *output,      // Output cell; readouts will be attached
                   const psArray *inputs, // Input readouts for combination
                   int iter,            // Number of rejection iterations
                   float rej,           // Rejection threshold (standard deviations)
                   psImageMaskType maskVal   // Value to mask
    );

// Thread entry point for pmDarkApplyScan
bool pmDarkApplyScan_Threaded(psThreadJob *job // Job to execute
    );

// Apply the dark correction to a scan
bool pmDarkApplyScan(pmReadout *readout, // Readout to correct
                     const pmCell *dark, // Dark to apply
                     const psVector *orders, // Polynomial orders for each ordinate
                     const psVector *values, // Values for each ordinate
                     psImageMaskType bad,    // Value to give bad pixels
                     bool doNorm,       // Normalise values?
                     float norm,        // Value by which to normalise
                     int rowStart, int rowStop // Scan range to work on
    );

// Apply dark
bool pmDarkApply(pmReadout *readout,    // Readout to which to apply dark
                 pmCell *dark,    // Dark to apply
                 psImageMaskType bad         // Mask value to give bad pixels
    );

// I/O functions for darks

// Write all darks within an FPA
bool pmFPAWriteDark(pmFPA *fpa,         // FPA to write
                    psFits *fits,       // FITS file to which to write
                    pmConfig *config,   // Configuration
                    bool blank,         // Write a blank only?
                    bool recurse        // Recurse to lower levels?
    );

// Write all darks within a chip
bool pmChipWriteDark(pmChip *chip,      // Chip to write
                     psFits *fits,      // FITS file to which to write
                     pmConfig *config,  // Configuration
                     bool blank,        // Write a blank only?
                     bool recurse       // Recurse to lower levels?
    );

// Write a dark to a FITS file
bool pmCellWriteDark(pmCell *cell,      // Cell containing dark information
                     psFits *fits,      // FITS file to which to write
                     pmConfig *config,  // Configuration
                     bool blank         // Write a blank only?
    );

// Read dark for all FPA from a FITS file
bool pmFPAReadDark(pmFPA *fpa,          // FPA for which to read
                   psFits *fits,        // FITS file to read
                   pmConfig *config     // Configuration
    );

// Read dark for all chip from a FITS file
bool pmChipReadDark(pmChip *chip,       // Chip for which to read
                    psFits *fits,       // FITS file to read
                    pmConfig *config    // Configuration
    );

// Read dark for a cell from a FITS file
bool pmCellReadDark(pmCell *cell,       // Cell for which to read
                    psFits *fits,       // FITS file to read
                    pmConfig *config    // Configuration
    );

// Write dark table to FITS file
bool pmDarkWrite(psFits *fits,          // FITS file to which to write
                 psMetadata *header,    // Header to write
                 const psArray *ordinates, // Dark ordinates to write
                 const char *normConcept // Normalisation concept name
    );

// Read dark table from FITS file
psArray *pmDarkRead(psString *normConcept, // Normalisation concept name
                    psFits *fits        // FITS file to read
    );

bool pmDarkVisualInit(psArray *values);
bool pmDarkVisualPixelFit(psVector *pixels, psVector *mask);
bool pmDarkVisualCleanup();
bool pmDarkVisualPixelModel(psPolynomialMD *poly, psArray *values);

#endif
