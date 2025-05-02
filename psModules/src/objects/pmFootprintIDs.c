/* @file  pmFootprintIDs.c
 * functions to manipulate footprint IDs 
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-08 02:51:14 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"

/*
 * Worker routine for the pmSetFootprintArrayIDs/pmSetFootprintID (and pmMergeFootprintArrays)
 */
static void
set_footprint_id(psImage *idImage,	// the image to set
		 const pmFootprint *fp, // the footprint to insert
		 const int id) {	// the desired ID
   const int col0 = fp->region.x0;
   const int row0 = fp->region.y0;

   for (int j = 0; j < fp->nspans; j++) {
       const pmSpan *span = fp->spans->data[j];
       psS32 *imgRow = idImage->data.S32[span->y - row0];
       for(int k = span->x0 - col0; k <= span->x1 - col0; k++) {
	   imgRow[k] += id;
       }
   }
}

void pmSetFootprintArrayIDsForImage(psImage *idImage,
				    const psArray *footprints, // the footprints to insert
				    const bool relativeIDs) { // show IDs starting at 0, not pmFootprint->id
    int id = 0;				// first index will be 1
    for (int i = 0; i < footprints->n; i++) {
	const pmFootprint *fp = footprints->data[i];
	if (relativeIDs) {
	    id++;
	} else {
	    id = fp->id;
	}
       
	set_footprint_id(idImage, fp, id);
    }
}

/*
 * Set an image to the value of footprint's ID whever they may fall
 */
psImage *pmSetFootprintArrayIDs(const psArray *footprints, // the footprints to insert
				const bool relativeIDs) { // show IDs starting at 1, not pmFootprint->id
   assert (footprints != NULL);

   if (footprints->n == 0) {
       return NULL;
   }
   const pmFootprint *fp = footprints->data[0];
   assert(pmFootprintTest((const psPtr)fp));
   const int numCols = fp->region.x1 - fp->region.x0 + 1;
   const int numRows = fp->region.y1 - fp->region.y0 + 1;
   const int col0 = fp->region.x0;
   const int row0 = fp->region.y0;
   assert (numCols >= 0 && numRows >= 0);
   
   psImage *idImage = psImageAlloc(numCols, numRows, PS_TYPE_S32);
   P_PSIMAGE_SET_ROW0(idImage, row0);
   P_PSIMAGE_SET_COL0(idImage, col0);
   psImageInit(idImage, 0);
   /*
    * do the work
    */
   pmSetFootprintArrayIDsForImage(idImage, footprints, relativeIDs);

   return idImage;
   
}

/*
 * Set an image to the value of footprint's ID whever they may fall
 */
psImage *pmSetFootprintID(psImage *idImage,
			  const pmFootprint *fp, // the footprint to insert
			  const int id) {	// the desired ID
   assert(fp != NULL && pmFootprintTest((const psPtr)fp));
   const int numCols = fp->region.x1 - fp->region.x0 + 1;
   const int numRows = fp->region.y1 - fp->region.y0 + 1;
   const int col0 = fp->region.x0;
   const int row0 = fp->region.y0;
   assert (numCols >= 0 && numRows >= 0);
   
   if (idImage == NULL) {
       idImage = psImageAlloc(numCols, numRows, PS_TYPE_S32);
   } else {
       assert (idImage->numCols == numCols);
       assert (idImage->numRows == numRows);
       // XXX assert on type (S32)
   }
   P_PSIMAGE_SET_ROW0(idImage, row0);
   P_PSIMAGE_SET_COL0(idImage, col0);
   psImageInit(idImage, 0);
   /*
    * do the work
    */
   set_footprint_id(idImage, fp, id);

   return idImage;
   
}

