/** @file  pmSource.c
 *
 *  Functions to define and manipulate sources on images
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA: significant modifications.
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"

#include "pmSourceUtils.h"

/******************************************************************************
    pmSourceModelGuess(source, model, maskVal, markVal): This function allocates a new
    pmModel structure based on the given modelType specified in the argument list.  The
    corresponding pmModelGuess function is returned, and used to supply the values of the
    params array in the pmModel structure.
 
    XXX: Many parameters are based on the src->moments structure, which is in
    image, not subImage coords.  Therefore, the calls to the model evaluation
    functions will be in image, not subImage coords.  Remember this.
*****************************************************************************/
pmModel *pmSourceModelGuess(pmSource *source, pmModelType modelType, psImageMaskType maskVal, psImageMaskType markVal)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(source, NULL);
    PS_ASSERT_PTR_NON_NULL(source->moments, NULL);
    PS_ASSERT_PTR_NON_NULL(source->peak, NULL);

    pmModel *model = pmModelAlloc(modelType);

    if (!model->class->modelGuess(model, source, maskVal, markVal)) {
	psFree (model);
	return NULL;
    }

    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(model);
}

pmSource *pmSourceFromModel (pmModel *model, pmReadout *readout, float radius, 
                             pmSourceType type)
{
    PS_ASSERT_PTR_NON_NULL(model, NULL);
    PS_ASSERT_PTR_NON_NULL(readout, NULL);

    pmSource *source = pmSourceAlloc ();

    // use the model centroid for the peak
    switch (type) {
      case PM_SOURCE_TYPE_STAR:
	source->modelPSF = model;
	break;
      case PM_SOURCE_TYPE_EXTENDED:
	source->modelEXT = model;
	break;
      default:
	psAbort ("invalid source type");
    }
    source->type = type;

    pmCell *cell = readout->parent;

    float Io    = model->params->data.F32[PM_PAR_I0];
    float xChip = model->params->data.F32[PM_PAR_XPOS];
    float yChip = model->params->data.F32[PM_PAR_YPOS];

    source->peak = pmPeakAlloc (xChip, yChip, Io, PM_PEAK_LONE);

    float xReadout, yReadout;

    // if we have information about the chip & cell, adjust the coordinates chip->cell->readout
    // otherwise, assume 0,0 offset and 1,1 parity
    if (cell) {
      int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
      int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
      int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
      int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

      // XXX fix the binning : currently not selected from concepts
      // int xBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.XBIN"); // Binning in x and y
      // int yBin = psMetadataLookupS32(NULL, cell->concepts, "CELL.YBIN"); // Binning in x and y
      int xBin = 1;
      int yBin = 1;

      // Position on the cell 
      float xCell = PM_CHIP_TO_CELL(xChip, x0Cell, xParityCell, xBin);
      float yCell = PM_CHIP_TO_CELL(yChip, y0Cell, yParityCell, yBin);

      // Position on the readout
      // float xReadout = CELL_TO_READOUT(xCell, x0Readout);
      // float yReadout = CELL_TO_READOUT(yCell, y0Readout);
      xReadout = xCell;
      yReadout = yCell;
    } else {
      xReadout = xChip;
      yReadout = yChip;
    }
    
    pmSourceDefinePixels (source, readout, xReadout, yReadout, radius);

    return (source);
}

