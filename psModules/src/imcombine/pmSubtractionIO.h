#ifndef PM_SUBTRACTION_IO_H
#define PM_SUBTRACTION_IO_H

#include <pslib.h>

#include <pmHDU.h>
#include <pmFPA.h>

/// Write subtraction kernels within a readout to a FITS file
bool pmReadoutWriteSubtractionKernels(
    pmReadout *readout,                 ///< Readout for which to write subtraction kernels (in analysis MD)
    psFits *fits                        ///< FITS file to which to write
    );


bool pmReadoutReadSubtractionKernels(
    pmReadout *readout,                 ///< Readout for which to read subtraction kernels (into analysis MD)
    psFits *fits                        ///< FITS file to which to write
    );


bool pmSubtractionReadKernels(const pmFPAview *view, ///< View into which to read
                              pmFPAfile *file, ///< File from which to read
                              pmConfig *config ///< Configuration
                              );

bool pmSubtractionWriteKernels(const pmFPAview *view, ///< View from which to write
                               pmFPAfile *file, ///< File to which to write
                               pmConfig *config ///< Configuration
                               );

bool pmSubtractionWritePHU(const pmFPAview *view, // View to PHU
                           pmFPAfile *file, ///< File to which to write
                           pmConfig *config ///< Configuration
                           );

#endif
