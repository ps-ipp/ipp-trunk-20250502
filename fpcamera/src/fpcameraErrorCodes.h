/** @file fpcameraErrorCodes.h
 *
 *  @brief
 *
 *  @ingroup fpcamera
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#if !defined(FPCAMERA_ERROR_CODES_H)
#define FPCAMERA_ERROR_CODES_H
/*
 * The line
 *  FPCAMERA_ERR_$X{ErrorCode},
 * (without the X)
 *
 * will be replaced by values from errorCodes.dat
 */
typedef enum {
    FPCAMERA_ERR_BASE = 4500,
    FPCAMERA_ERR_UNKNOWN,
    FPCAMERA_ERR_NOT_IMPLEMENTED,
    FPCAMERA_ERR_ARGUMENTS,
    FPCAMERA_ERR_CONFIG,
    FPCAMERA_ERR_IO,
    FPCAMERA_ERR_WCS,
    FPCAMERA_ERR_DATA,
    FPCAMERA_ERR_REFSTARS,
    FPCAMERA_ERR_NERROR
} fpcameraErrorCode;

void fpcameraErrorRegister(void);

#endif
