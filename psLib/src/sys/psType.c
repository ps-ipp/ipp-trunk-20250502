/** @file  psType.c
*
*  @brief Contains psType checking functions
*
*  @author Robert DeSonia, MHPCC
*  @author Robert Lupton, Princeton University
*  @author Joshua Hoblitt, University of Hawaii
*
*  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-02-03 05:54:08 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdbool.h>

#include "psType.h"
#include "psBits.h"
#include "psFits.h"
#include "psPixels.h"
#include "psSphereOps.h"
#include "psMinimizeLMM.h"
#include "psImageConvolve.h"
#include "psTime.h"
#include "psLine.h"
#include "psRegion.h"
#include "psHistogram.h"
#include "psMemory.h"

bool psMemCheckType(psDataType type,
                    psPtr ptr)
{
    if (!ptr) {
        return false;
    }

    switch(type) {
    case PS_DATA_ARRAY:
        if (psMemCheckArray(ptr)) {
            return true;
        }
        break;
    case PS_DATA_BITS:
        if (psMemCheckBits(ptr)) {
            return true;
        }
        break;
    case PS_DATA_CUBE:
        if (psMemCheckCube(ptr)) {
            return true;
        }
        break;
    case PS_DATA_FITS:
        if (psMemCheckFits(ptr)) {
            return true;
        }
        break;
    case PS_DATA_HASH:
        if (psMemCheckHash(ptr)) {
            return true;
        }
        break;
    case PS_DATA_HISTOGRAM:
        if (psMemCheckHistogram(ptr)) {
            return true;
        }
        break;
    case PS_DATA_IMAGE:
        if (psMemCheckImage(ptr)) {
            return true;
        }
        break;
    case PS_DATA_KERNEL:
        if (psMemCheckKernel(ptr)) {
            return true;
        }
        break;
    case PS_DATA_LINE:
        if (psMemCheckLine(ptr)) {
            return true;
        }
        break;
    case PS_DATA_LIST:
        if (psMemCheckList(ptr)) {
            return true;
        }
        break;
    case PS_DATA_LOOKUPTABLE:
        if (psMemCheckLookupTable(ptr)) {
            return true;
        }
        break;
    case PS_DATA_METADATA:
        if (psMemCheckMetadata(ptr)) {
            return true;
        }
        break;
    case PS_DATA_METADATAITEM:
        if (psMemCheckMetadataItem(ptr)) {
            return true;
        }
        break;
    case PS_DATA_MINIMIZATION:
        if (psMemCheckMinimization(ptr)) {
            return true;
        }
        break;
    case PS_DATA_PIXELS:
        if (psMemCheckPixels(ptr)) {
            return true;
        }
        break;
    case PS_DATA_PLANE:
        if (psMemCheckPlane(ptr)) {
            return true;
        }
        break;
    case PS_DATA_PLANEDISTORT:
        if (psMemCheckPlaneDistort(ptr)) {
            return true;
        }
        break;
    case PS_DATA_PLANETRANSFORM:
        if (psMemCheckPlaneTransform(ptr)) {
            return true;
        }
        break;
    case PS_DATA_POLYNOMIAL1D:
        if (psMemCheckPolynomial1D(ptr)) {
            return true;
        }
        break;
    case PS_DATA_POLYNOMIAL2D:
        if (psMemCheckPolynomial2D(ptr)) {
            return true;
        }
        break;
    case PS_DATA_POLYNOMIAL3D:
        if (psMemCheckPolynomial3D(ptr)) {
            return true;
        }
        break;
    case PS_DATA_POLYNOMIAL4D:
        if (psMemCheckPolynomial4D(ptr)) {
            return true;
        }
        break;
    case PS_DATA_PROJECTION:
        if (psMemCheckProjection(ptr)) {
            return true;
        }
        break;
    case PS_DATA_REGION:
        if (psMemCheckRegion(ptr)) {
            return true;
        }
        break;
    case PS_DATA_SCALAR:
        if (psMemCheckScalar(ptr)) {
            return true;
        }
        break;
    case PS_DATA_SPHERE:
        if (psMemCheckSphere(ptr)) {
            return true;
        }
        break;
    case PS_DATA_SPHEREROT:
        if (psMemCheckSphereRot(ptr)) {
            return true;
        }
        break;
    case PS_DATA_SPLINE1D:
        if (psMemCheckSpline1D(ptr)) {
            return true;
        }
        break;
    case PS_DATA_STATS:
        if (psMemCheckStats(ptr)) {
            return true;
        }
        break;
    case PS_DATA_STRING:
        if (psMemCheckString(ptr)) {
            return true;
        }
        break;
    case PS_DATA_TIME:
        if (psMemCheckTime(ptr)) {
            return true;
        }
        break;
    case PS_DATA_VECTOR:
        if (psMemCheckVector(ptr)) {
            return true;
        }
        break;
    default:
        // error state -- fall through to returning false
        break;
    }

    return false;
}
