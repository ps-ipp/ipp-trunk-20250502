#include "ppStack.h"

// this function unlinks the temporary files (if desired)
bool ppStackFinish(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psThreadPoolFinalize();

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    bool mdok;                          // Status of MD lookup
    bool tempDelete = psMetadataLookupBool(&mdok, recipe, "TEMP.DELETE"); // Delete temporary files?

    // Delete temporary images
    if (tempDelete && options->convolve) {
        for (int i = 0; i < options->num; i++) {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                continue;
            }

	    // XXX careful about repeatative resolution of nebulous names (though these are probably not neb names)
            psString imageResolved = pmConfigConvertFilename(options->convImages->data[i], config, false, false);
            psString maskResolved = pmConfigConvertFilename(options->convMasks->data[i], config, false, false);
            psString varianceResolved = pmConfigConvertFilename(options->convVariances->data[i], config, false, false);
            if (unlink(imageResolved) == -1 || unlink(maskResolved) == -1 ||
                unlink(varianceResolved) == -1) {
                psWarning("Unable to delete temporary files for image %d", i);
            }
            psFree(imageResolved);
            psFree(maskResolved);
            psFree(varianceResolved);
        }
    }
    return true;
}


psExit ppStackExitCode(psExit exitValue)
{
    if (exitValue != PS_EXIT_SUCCESS) {
        return exitValue;
    }

    // gcc -Wswitch complains here if err is declared as type psErrorCode
    // the collection of ps*ErrorCode values are enums defined separately for 
    // each module (psphot, pswarp, etc).  the lowest type, psErrorCode is only the base set and does
    // not include the possible psphot values

    // for now, to get around this, we just use an int for the switch

    // psErrorCode errorCode = psErrorCodeLast(); // Error code
    int errorCode = psErrorCodeLast(); // Error code
    if (errorCode == PS_ERR_NONE) {
	return exitValue;
    }

    psErrorStackPrint(stderr, "Unable to perform stack.");
    pmFPAfileFreeSetStrict(false);
    switch (errorCode) {
      case PPSTACK_ERR_UNKNOWN:
      case PS_ERR_UNKNOWN:
	psLogMsg("ppStack", PS_LOG_WARN, "Unknown error code: %x", errorCode);
	exitValue = PS_EXIT_UNKNOWN_ERROR;
	break;
      case PS_ERR_IO:
      case PS_ERR_DB_CLIENT:
      case PS_ERR_DB_SERVER:
      case PS_ERR_BAD_FITS:
      case PS_ERR_OS_CALL_FAILED:
      case PM_ERR_SYS:
      case PPSTACK_ERR_IO:
	psLogMsg("ppStack", PS_LOG_WARN, "I/O error code: %x", errorCode);
	exitValue = PS_EXIT_SYS_ERROR;
	break;
      case PS_ERR_BAD_PARAMETER_VALUE:
      case PS_ERR_BAD_PARAMETER_TYPE:
      case PS_ERR_BAD_PARAMETER_NULL:
      case PS_ERR_BAD_PARAMETER_SIZE:
      case PPSTACK_ERR_ARGUMENTS:
      case PPSTACK_ERR_CONFIG:
	psLogMsg("ppStack", PS_LOG_WARN, "Configuration error code: %x", errorCode);
	exitValue = PS_EXIT_CONFIG_ERROR;
	break;
      case PPSTACK_ERR_PSF:
      case PSPHOT_ERR_PSF:
      case PM_ERR_STAMPS:
      case PM_ERR_SMALL_AREA:
      case PPSTACK_ERR_REJECTED:
      case PPSTACK_ERR_DATA:
	psLogMsg("ppStack", PS_LOG_WARN, "Data error code: %x", errorCode);
	exitValue = PS_EXIT_DATA_ERROR;
	break;
      case PS_ERR_UNEXPECTED_NULL:
      case PS_ERR_PROGRAMMING:
      case PPSTACK_ERR_NOT_IMPLEMENTED:
      case PPSTACK_ERR_PROG:
	psLogMsg("ppStack", PS_LOG_WARN, "Programming error code: %x", errorCode);
	exitValue = PS_EXIT_PROG_ERROR;
	break;
      default:
	// It's a programming error if we're not dealing with the error correctly
	psLogMsg("ppStack", PS_LOG_WARN, "Unrecognised error code: %x", errorCode);
	exitValue = PS_EXIT_PROG_ERROR;
	break;
    }
    return exitValue;
}
