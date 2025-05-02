#include <stdio.h>
#include <pslib.h>
#include <psphotErrorCodes.h>

#include "ppSub.h"

psExit ppSubExitCode(psExit exitValue)
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
    if (errorCode != PS_ERR_NONE) {
        pmFPAfileFreeSetStrict(false);
        psErrorStackPrint(stderr, "Error in subtraction:");
        switch (errorCode) {
          case PPSUB_ERR_UNKNOWN:
          case PS_ERR_UNKNOWN:
            psLogMsg("ppSub", PS_LOG_WARN, "Unknown error code: %x", errorCode);
            exitValue = PS_EXIT_UNKNOWN_ERROR;
            break;
          case PS_ERR_IO:
          case PS_ERR_DB_CLIENT:
          case PS_ERR_DB_SERVER:
          case PS_ERR_BAD_FITS:
          case PS_ERR_OS_CALL_FAILED:
          case PM_ERR_SYS:
          case PPSUB_ERR_IO:
            psLogMsg("ppSub", PS_LOG_WARN, "I/O error code: %x", errorCode);
            exitValue = PS_EXIT_SYS_ERROR;
            break;
          case PS_ERR_BAD_PARAMETER_VALUE:
          case PS_ERR_BAD_PARAMETER_TYPE:
          case PS_ERR_BAD_PARAMETER_NULL:
          case PS_ERR_BAD_PARAMETER_SIZE:
          case PPSUB_ERR_ARGUMENTS:
          case PPSUB_ERR_CONFIG:
            psLogMsg("ppSub", PS_LOG_WARN, "Configuration error code: %x", errorCode);
            exitValue = PS_EXIT_CONFIG_ERROR;
            break;
          case PSPHOT_ERR_PSF:
          case PPSUB_ERR_DATA:
          case PPSUB_ERR_NO_OVERLAP:
            psLogMsg("ppSub", PS_LOG_WARN, "Data error code: %x", errorCode);
            exitValue = PS_EXIT_DATA_ERROR;
            break;
          case PS_ERR_UNEXPECTED_NULL:
          case PS_ERR_PROGRAMMING:
          case PPSUB_ERR_NOT_IMPLEMENTED:
          case PPSUB_ERR_PROG:
            psLogMsg("ppSub", PS_LOG_WARN, "Programming error code: %x", errorCode);
            exitValue = PS_EXIT_PROG_ERROR;
            break;
          default:
            // It's a programming error if we're not dealing with the error correctly
            psLogMsg("ppSub", PS_LOG_WARN, "Unrecognised error code: %x", errorCode);
            exitValue = PS_EXIT_PROG_ERROR;
            break;
        }
    }

    return exitValue;
}
