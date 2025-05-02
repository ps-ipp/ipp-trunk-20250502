/** @file pswarpExit.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

psExit pswarpExitCode(psExit exitValue)
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
        psErrorStackPrint(stderr, "Unable to perform warp.");
        switch (errorCode) {
          case PSWARP_ERR_UNKNOWN:
          case PS_ERR_UNKNOWN:
            psLogMsg("pswarp", PS_LOG_WARN, "Unknown error code: %x", errorCode);
            return PS_EXIT_UNKNOWN_ERROR;
          case PS_ERR_IO:
          case PS_ERR_DB_CLIENT:
          case PS_ERR_DB_SERVER:
          case PS_ERR_BAD_FITS:
          case PS_ERR_OS_CALL_FAILED:
          case PM_ERR_SYS:
          case PSWARP_ERR_IO:
            psLogMsg("pswarp", PS_LOG_WARN, "I/O error code: %x", errorCode);
            return PS_EXIT_SYS_ERROR;
          case PS_ERR_BAD_PARAMETER_VALUE:
          case PS_ERR_BAD_PARAMETER_TYPE:
          case PS_ERR_BAD_PARAMETER_NULL:
          case PS_ERR_BAD_PARAMETER_SIZE:
          case PSWARP_ERR_ARGUMENTS:
          case PSWARP_ERR_CONFIG:
            psLogMsg("pswarp", PS_LOG_WARN, "Configuration error code: %x", errorCode);
            return PS_EXIT_CONFIG_ERROR;
          case PSPHOT_ERR_PSF:
          case PSWARP_ERR_DATA:
          case PSWARP_ERR_NO_OVERLAP:
            psLogMsg("pswarp", PS_LOG_WARN, "Data error code: %x", errorCode);
            return PS_EXIT_DATA_ERROR;
          case PS_ERR_UNEXPECTED_NULL:
          case PS_ERR_PROGRAMMING:
          case PSWARP_ERR_NOT_IMPLEMENTED:
            psLogMsg("pswarp", PS_LOG_WARN, "Programming error code: %x", errorCode);
            exitValue = PS_EXIT_PROG_ERROR;
            break;
          default:
            // It's a programming error if we're not dealing with the error correctly
            psLogMsg("pswarp", PS_LOG_WARN, "Unrecognised error code: %x", errorCode);
            return PS_EXIT_PROG_ERROR;
        }
    }

    return exitValue;
}

