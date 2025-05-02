#ifndef PM_EXPNUM_IO_H
#define PM_EXPNUM_IO_H

#include <pslib.h>

#include <pmHDU.h>
#include <pmFPA.h>


bool pmExpNumRead(const pmFPAview *view, ///< View into which to read
                   pmFPAfile *file, ///< File from which to read
                   pmConfig *config ///< Configuration
    );

#ifdef notyet
// Currently we don't need write functions for the EXPNUM images. 
// Only ppStack creates them and it uses a MASK file type

/// Write pattern correction within a readout to a FITS file
bool pmReadoutWritePattern(
    pmReadout *readout,                 ///< Readout for which to write pattern correction (in analysis MD)
    psFits *fits                        ///< FITS file to which to write
    );
bool pmPatternWrite(const pmFPAview *view, ///< View from which to write
                    pmFPAfile *file, ///< File to which to write
                    pmConfig *config ///< Configuration
    );

bool pmPatternWritePHU(const pmFPAview *view, // View to PHU
                       pmFPAfile *file, ///< File to which to write
                       pmConfig *config ///< Configuration
    );

#endif // notyet

#endif
