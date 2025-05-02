#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "psError.h"
#include "psFits.h"
#include "psFitsFloat.h"
#include "psMemory.h"

#include "psFitsFloatFile.h"

bool psFitsFloatImageSet(const psFits *fits, psFitsFloat type)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);

    char *convName;                     // Convention name

    switch (type) {
      case PS_FITS_FLOAT_NONE:
        return true;
      case PS_FITS_FLOAT_16_0:
        convName = "FLOAT_16_0";
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to recognise convention: %x", type);
        return false;
    }

    int status = 0;                     // Status from cfitsio
    fits_write_key_str(fits->fd, "PSBITPIX", convName, "Custom floating point convention name", &status);
    if (psFitsError(status, true, "Could not write PSBITPIX header to file.")) {
        return false;
    }
    return true;
}


psFitsFloat psFitsFloatImageCheck(const psFits *fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, PS_FITS_FLOAT_NONE);

    psFitsOptions *options = fits->options; // FITS I/O options

    if (!options || !options->conventions.psBitpix) {
        return PS_FITS_FLOAT_NONE;
    }

    int status = 0;                     // Status of CFITSIO calls
    char convName[FLEN_CARD];           // Convention name for custom floating-point
    if (fits_read_key_str(fits->fd, "PSBITPIX", convName, NULL, &status) && status != KEY_NO_EXIST) {
        psFitsError(status, true, "Unable to read header.");
        return PS_FITS_FLOAT_NONE;
    }

    // XXX convName is static if (!convName || status == KEY_NO_EXIST) {
    if (status == KEY_NO_EXIST) {
        return PS_FITS_FLOAT_NONE;
    }

    return psFitsFloatTypeFromString(convName);
}
