/** @file pmResiduals.h
 *
 * Functions to manipulate the residual tables (data - model).
 *
 * @author EAM, IfA
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:25 $
 * Copyright 2004 IfA, University of Hawaii
 */

# ifndef PM_RESIDUALS_H
# define PM_RESIDUALS_H
/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** residual tables for sources
 */
typedef struct {
    psImage *Ro;
    psImage *Rx;
    psImage *Ry;
    psImage *variance;
    psImage *mask;
    int xBin;
    int yBin;
    int xCenter;
    int yCenter;
} pmResiduals;

pmResiduals *pmResidualsAlloc (int xSize, int ySize, int xBin, int yBin);
bool psMemCheckResiduals(psPtr ptr);

// macros to abstract the resid mask type : these values must be consistent
#define PM_TYPE_RESID_MASK PS_TYPE_U8        /**< the psElemType to use for mask image */
#define PM_TYPE_RESID_MASK_DATA U8           /**< the data member to use for mask image */
#define PM_TYPE_RESID_MASK_NAME "psU8"       /**< the data type for mask as a string */
typedef psU8 pmResidMaskType;               ///< the C datatype for a mask image
#define PM_NOT_RESID_MASK(A)(UINT8_MAX-(A))

/// @}
# endif
