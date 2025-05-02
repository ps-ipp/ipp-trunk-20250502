#ifndef PS_IMAGE_MAP_FIT_H
#define PS_IMAGE_MAP_FIT_H

#include <psType.h>
#include <psVector.h>
#include <psImageMap.h>


// fit the image map to a set of points
bool psImageMapFit(bool *pGoodFit, 
		   psImageMap *map,
                   const psVector *mask,
                   psVectorMaskType maskValue, // 
                   const psVector *x,
                   const psVector *y,
                   const psVector *f,
                   const psVector *df
    );

// fit the image map to a set of points
bool psImageMapClipFit(bool *pGoodFit, 
		       psImageMap *map,
                       psStats *stats,
                       psVector *mask,  // WARNING: Mask is modified!
                       psVectorMaskType maskValue,
                       const psVector *x,
                       const psVector *y,
                       const psVector *f,
                       const psVector *df
    );


// fit the image map to a set of points, using sparse matrix tools
bool psImageMapFitSparse(bool *pGoodFit, 
		   psImageMap *map,
                   const psVector *mask,
                   psVectorMaskType maskValue, // 
                   const psVector *x,
                   const psVector *y,
                   const psVector *f,
                   const psVector *df
    );

// fit the image map to a set of points using sparse matrix tools
bool psImageMapClipFitSparse(bool *pGoodFit, 
		       psImageMap *map,
                       psStats *stats,
                       psVector *mask,  // WARNING: Mask is modified!
                       psVectorMaskType maskValue,
                       const psVector *x,
                       const psVector *y,
                       const psVector *f,
                       const psVector *df
    );

bool psImageMapFit1DinY(bool *pGoodFit, 
			psImageMap *map,
                        const psVector *mask,
                        psVectorMaskType maskValue,
                        const psVector *x,
                        const psVector *y,
                        const psVector *f,
                        const psVector *df
    );

bool psImageMapFit1DinX(bool *pGoodFit, 
			psImageMap *map,
                        const psVector *mask,
                        psVectorMaskType maskValue,
                        const psVector *x,
                        const psVector *y,
                        const psVector *f,
                        const psVector *df
    );

bool psImageMapRepair (psImage *image);

#endif
