#ifndef PM_FPA_BIN_H
#define PM_FPA_BIN_H

#include <pslib.h>

#include <pmFPA.h>

/// Rebin a readout
bool pmReadoutRebin(pmReadout *out,     ///< Output readout
                    const pmReadout *in,///< Input readout
                    psImageMaskType maskVal, ///< Value to mask
                    int xBin, int yBin  ///< Binning factors in x and y
    );



#endif
