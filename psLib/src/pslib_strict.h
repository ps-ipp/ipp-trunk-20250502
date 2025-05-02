/** @file  pslib_strict.h
*
*  @brief Contains the complete list of header files for pslib while poisoning
*         the use of standard memory allocation routines.
*
*  This header file includes all the necessary header files for a user to
*  user all public functions within the pslib library.
*
*  @author Eric Van Alst, MHPCC
*
*  @version $Revision: 1.40 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-02-04 20:34:52 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifndef PS_LIB_STRICT_H
#define PS_LIB_STRICT_H

#ifdef PS_LIB_H  /* this is included from pslib.h, so don't poison anything */
#ifndef PS_ALLOW_MALLOC
#define PS_ALLOW_MALLOC
#endif // #ifndef PS_ALLOW_MALLOC
#else
#undef PS_ALLOW_MALLOC /* don't allow code to not poison malloc, i.e., strict poisioning */
#endif // #else

#include "psTime.h"
#include "psCoord.h"
#include "psSphereOps.h"
#include "psEarthOrientation.h"

#include "psDB.h"

#include "psFFT.h"
#include "psVectorFFT.h"
#include "psImageFFT.h"

#include "psFits.h"
#include "psFitsHeader.h"
#include "psFitsImage.h"
#include "psFitsTable.h"
#include "psFitsTableNew.h"
#include "psFitsFloat.h"
#include "psFitsFloatFile.h"
#include "psFitsScale.h"

//#include "psXML.h"

#include "psRegion.h"
#include "psImageInterpolate.h"
#include "psImageConvolve.h"
#include "psImageCovariance.h"
#include "psImageGeomManip.h"
#include "psImagePixelExtract.h"
#include "psImagePixelManip.h"
#include "psImagePixelInterpolate.h"
#include "psImageStats.h"
#include "psImageStructManip.h"
#include "psImageMaskOps.h"
#include "psImageBinning.h"
#include "psImageUnbin.h"
#include "psImageMap.h"
#include "psImageMapFit.h"

#include "psImageJpeg.h"

#include "psAssert.h"
#include "psBinaryOp.h"
#include "psCompare.h"
#include "psConstants.h"
#include "psExit.h"
#include "psMatrix.h"
#include "psMD5.h"
#include "psMinimizeLMM.h"
#include "psMinimizePowell.h"
#include "psMinimizePolyFit.h"
#include "psMixtureModels.h"
#include "psMutex.h"
#include "psRandom.h"
#include "psRegionForImage.h"
#include "psPolynomial.h"
#include "psPolynomialMetadata.h"
#include "psPolynomialUtils.h"
#include "psPolynomialMD.h"
#include "psSort.h"
#include "psSpline.h"
#include "psStats.h"
#include "psHistogram.h"
#include "psUnaryOp.h"
#include "psMathUtils.h"
#include "psImage.h"
#include "psScalar.h"
#include "psVector.h"
#include "psAbort.h"
#include "psConfigure.h"
#include "psError.h"
#include "psErrorCodes.h"
#include "psLogMsg.h"
#include "psMemory.h"
#include "psString.h"
#include "psLine.h"
#include "psTrace.h"
#include "psType.h"
#include "psArray.h"
#include "psBits.h"
#include "psHash.h"
#include "psList.h"
#include "psLookupTable.h"
#include "psMetadata.h"
#include "psMetadataConfig.h"
#include "psMetadataItemParse.h"
#include "psMetadataItemCompare.h"
#include "psMetadataHeader.h"
#include "psPixels.h"
#include "psArguments.h"
#include "psVectorSmooth.h"
#include "psImageBackground.h"
#include "psEllipse.h"
#include "psSparse.h"
#include "psSlurp.h"
#include "psTree.h"

#include "psThread.h"

#endif // #ifndef PS_LIB_STRICT_H
