// This file exists to resolve the co-dependency problem of placing these function definitions in
// psFitsFloat.h --- because they refer to psFits, and psFits refers to psFitsFloat, we would end up with a
// co-dependency problem if these function declarations were to be placed in psFitsFloat.h.
#ifndef PS_FITS_FLOAT_FILE_H
#define PS_FITS_FLOAT_FILE_H

#include <psFits.h>
#include <psFitsFloat.h>

/// Set a flag in the FITS header of the current extension that the image is a custom floating-point
bool psFitsFloatImageSet(const psFits *fits,  ///< FITS file
                         psFitsFloat type ///< Custom floating-point type
    );

/// Check if the current extension contains a custom floating-point, returning the appropriate type
psFitsFloat psFitsFloatImageCheck(const psFits *fits ///< FITS file
    );


#endif
