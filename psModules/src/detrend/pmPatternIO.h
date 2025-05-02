#ifndef PM_PATTERN_IO_H
#define PM_PATTERN_IO_H

#include <pslib.h>

#include <pmHDU.h>
#include <pmFPA.h>

/// Write pattern correction within a readout to a FITS file
bool pmReadoutWritePattern(
    pmReadout *readout,                 ///< Readout for which to write pattern correction (in analysis MD)
    psFits *fits                        ///< FITS file to which to write
    );


bool pmReadoutReadPattern(
    pmReadout *readout,                 ///< Readout for which to read pattern correction (into analysis MD)
    psFits *fits                        ///< FITS file to which to write
    );


bool pmPatternRead(const pmFPAview *view, ///< View into which to read
                   pmFPAfile *file, ///< File from which to read
                   pmConfig *config ///< Configuration
    );

bool pmPatternWrite(const pmFPAview *view, ///< View from which to write
                    pmFPAfile *file, ///< File to which to write
                    pmConfig *config ///< Configuration
    );

bool pmPatternWritePHU(const pmFPAview *view, // View to PHU
                       pmFPAfile *file, ///< File to which to write
                       pmConfig *config ///< Configuration
    );


bool pmPatternRowAmpRead (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmPatternRowAmpReadFPA (pmFPAfile *file);
bool pmPatternRowAmpReadChips (pmFPAfile *file);

bool pmPatternDeadCellsRead (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmPatternDeadCellsReadFPA (pmFPAfile *file);
bool pmPatternDeadCellsReadChips (pmFPAfile *file);

#endif
