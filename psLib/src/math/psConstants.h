/* @file  psConstants.h
 *
 * @brief Definitions of various constants and common macros
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.97 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 * XXX: Add parenthesis around all arguments so that these macros can be
 *      called with complex expressions.
 *
 * XXX: All functions which use the PS_ASSERT macros must be scrutinized so
 * that we ensure that an argument which is expected to be output is
 * psFree'ed before reurning NULL.
 *
 * XXX: The macros have a name similar to PS_CHECK_CONDITION() and generally
 * throw a psError if the CONDITION is true.  However, some throw the error
 * if the CONDITION is false.  This should be consistant.
 *
 */

#ifndef PS_CONSTANTS_H
#define PS_CONSTANTS_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include <math.h> // for M_PI

/*****************************************************************************
These are common mathimatical constants used by various functions in the psLib.
 *****************************************************************************/
#ifndef M_PI
#define M_PI   3.1415926535897932384626433832795029  /* pi */
#define M_PI_2 1.5707963267948966192313216916397514  /* pi/2 */
#define M_PI_4 0.7853981633974483096156608458198757  /* pi/4 */
#define M_1_PI 0.3183098861837906715377675267450287  /* 1/pi */
#define M_2_PI 0.6366197723675813430755350534900574  /* 2/pi */
#endif // #ifndef M_PI

#define DEG_TO_RAD(DEGREES) ((DEGREES) * M_PI / 180.0)
#define MIN_TO_RAD(MINUTES) ((MINUTES) * M_PI / (180.0 * 60.0))
#define SEC_TO_RAD(SECONDS) ((SECONDS) * M_PI / (180.0 * 60.0 * 60.0))
#define RAD_TO_DEG(RADIANS) ((RADIANS) * 180.0 / M_PI)
#define RAD_TO_MIN(RADIANS) ((RADIANS) * 180.0 * 60.0 / M_PI)
#define RAD_TO_SEC(RADIANS) ((RADIANS) * 180.0 * 60.0 * 60.0 / M_PI)

# define PS_DEG_RAD 57.295779513082322
# define PS_RAD_DEG  0.017453292519943

/*****************************************************************************
    Misc. macros:
*****************************************************************************/
#define PS_MAX(A, B) \
(((A) > (B)) ? (A) : (B))

#define PS_MIN(A, B) \
(((A) < (B)) ? (A) : (B))

#define PS_SQR(A) \
((A) * (A))

#define PS_SWAP(X,Y) {double tmp=(X); (X) = (Y); (Y) = tmp;}

// These defines for bitwise opertaions are necessary to yield results of the proper size (use instead of ~)
#define PS_NOT_U8(A)(UINT8_MAX-(A)) // Perform bitwise NOT on A which is of type U8
#define PS_NOT_U16(A)(UINT16_MAX-(A)) // Perform bitwaise NOT on A which is of type U16
#define PS_NOT_U32(A)(UINT32_MAX-(A)) // Perform bitwise NOT on A which is of type U8
#define PS_NOT_U64(A)(UINT64_MAX-(A)) // Perform bitwaise NOT on A which is of type U16

/// @}
#endif
