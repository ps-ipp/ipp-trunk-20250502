/** @file pswarpMatchRange.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

# define TEST_MINMAX \
    psPlaneTransformApply(destFP, chipDest->toFPA, destPix); \
    psPlaneTransformApply (destTP, fpaDest->toTPA, destFP); \
    psDeproject (sky, destTP, fpaDest->toSky); \
    psProject (srcTP, sky, fpaSrc->toSky); \
    psPlaneTransformApply (srcFP, fpaSrc->fromTPA, srcTP); \
    psPlaneTransformApply (srcPix, chipSrc->fromFPA, srcFP); \
    *minX = PS_MIN (*minX, srcPix->x); \
    *minY = PS_MIN (*minY, srcPix->y); \
    *maxX = PS_MAX (*maxX, srcPix->x); \
    *maxY = PS_MAX (*maxY, srcPix->y);

/** we are warping from src to dest.  find the max overlapping pixels in the INPUT (src)
 * coordinate frame.  NOTE: these are in the parent pixel frame since the astrometric
 * transformation refers to the parent frame
 */
bool pswarpMatchRange (int *minX, int *minY, int *maxX, int *maxY, pmReadout *dest, pmReadout *src) {

    // transform input corners and edge centers to output coords
    // find max overlapping region of output image
    
    pmCell *cell = NULL;

    cell = src->parent;
    pmChip *chipSrc = cell->parent;
    pmFPA *fpaSrc = chipSrc->parent;

    cell = dest->parent;
    pmChip *chipDest = cell->parent;
    pmFPA *fpaDest = chipDest->parent;

    *minX = src->image->numCols + src->image->col0 - 1;
    *minY = src->image->numRows + src->image->row0 - 1;
    *maxX = src->image->col0;
    *maxY = src->image->row0;

    // XXX save these as static for speed?
    psPlane *srcPix = psPlaneAlloc();
    psPlane *srcFP  = psPlaneAlloc();
    psPlane *srcTP  = psPlaneAlloc();

    psPlane *destPix = psPlaneAlloc();
    psPlane *destFP  = psPlaneAlloc();
    psPlane *destTP  = psPlaneAlloc();

    psSphere *sky = psSphereAlloc();

    destPix->x = dest->image->col0;
    destPix->y = dest->image->row0;
    TEST_MINMAX;
    
    destPix->x = dest->image->col0 + dest->image->numCols;
    destPix->y = dest->image->row0;
    TEST_MINMAX;
    
    destPix->x = dest->image->col0 + dest->image->numCols;
    destPix->y = dest->image->row0 + dest->image->numRows;
    TEST_MINMAX;
    
    destPix->x = dest->image->col0;
    destPix->y = dest->image->row0 + dest->image->numRows;
    TEST_MINMAX;
    
    destPix->x = dest->image->col0 + 0.5*dest->image->numCols;
    destPix->y = dest->image->row0;
    TEST_MINMAX;
    
    destPix->x = dest->image->col0;
    destPix->y = dest->image->row0 + 0.5*dest->image->numRows;
    TEST_MINMAX;
    
    destPix->x = dest->image->col0 + 0.5*dest->image->numCols;
    destPix->y = dest->image->row0;
    TEST_MINMAX;

    destPix->x = dest->image->col0;
    destPix->y = dest->image->row0 + 0.5*dest->image->numRows;
    TEST_MINMAX;

    *minX = PS_MAX (*minX, src->image->col0);
    *minY = PS_MAX (*minY, src->image->row0);
    *maxX = PS_MIN (*maxX, src->image->numCols + src->image->col0);
    *maxY = PS_MIN (*maxY, src->image->numRows + src->image->row0);

    // demo forward and backward transformation
# if (0)
    srcPix->x = *minX;
    srcPix->y = *minY;
    psPlaneTransformApply(srcFP, chipSrc->toFPA, srcPix); 
    psPlaneTransformApply (srcTP, fpaSrc->toTPA, srcFP); 
    psDeproject (sky, srcTP, fpaSrc->toSky); 
    psProject (destTP, sky, fpaDest->toSky); 
    psPlaneTransformApply (destFP, fpaDest->fromTPA, destTP); 
    psPlaneTransformApply (destPix, chipDest->fromFPA, destFP); 
    fprintf (stderr, "%f,%f -> %f,%f ", srcPix->x, srcPix->y, destPix->x, destPix->y);

    psPlaneTransformApply(destFP, chipDest->toFPA, destPix); 
    psPlaneTransformApply (destTP, fpaDest->toTPA, destFP); 
    psDeproject (sky, destTP, fpaDest->toSky); 
    psProject (srcTP, sky, fpaSrc->toSky); 
    psPlaneTransformApply (srcFP, fpaSrc->fromTPA, srcTP); 
    psPlaneTransformApply (srcPix, chipSrc->fromFPA, srcFP); 
    fprintf (stderr, "-> %f,%f\n", srcPix->x, srcPix->y);
# endif

    psFree (srcPix);
    psFree (srcFP);
    psFree (srcTP);

    psFree (destPix);
    psFree (destFP);
    psFree (destTP);

    psFree (sky);

    return true;
}

