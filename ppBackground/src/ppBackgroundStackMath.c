/** @file ppBackgroundStackArguments.c
 *
 *  @brief
 *
 *  @ingroup ppBackgroundStack
 *
 *  @author Paul Price
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppBackgroundStack.h"

// data->models->name->XY__->{image/ra/dec/calibrated/offset/scale}
// data->models->counter->XY__->{image/ra/dec/calibrated/offset/scale}

//
// Determine the best OTA-to-OTA solution for this set of exposures/chips.
// This is done by finding the median value of the set of (ota,u,v) values for
// all exposures.
// solution_{OTA}(u,v) = < data_{OTA}(u,v) >_{exposures}
bool ppBackgroundStackModelFitOTASolution(ppBackgroundStackData *data) {
  int u,v;
  psMetadataIterator *iter = psMetadataIteratorAlloc(data->OTA_solutions, PS_LIST_HEAD, NULL); // Iterate over all chips.
  psMetadataItem *item;
  psStats *stats = psStatsAlloc( PS_STAT_ROBUST_MEDIAN );
  psS16 N = psMetadataLookupS16(NULL, data->models, "N");
  
  while ((item = psMetadataGetAndIncrement(iter))) {
    if (item->type != PS_DATA_UNKNOWN) {
      psWarning("This seems like I have a not-data, when I expect a data.");
      continue;
    }
    psString workingChip = item->name;
    psImage  *solution    = item->data.V;

    for (v = 0; v < solution->numRows; v++) {
      for (u = 0; u < solution->numCols; u++) {

	psVector *tmp = psVectorAllocEmpty(N,PS_TYPE_F32);	

	psMetadataIterator *expIter = psMetadataIteratorAlloc(data->models, PS_LIST_HEAD, NULL);
	psMetadataItem *expItem;
	while ((expItem = psMetadataGetAndIncrement(expIter))) {
	  if (expItem->type != PS_DATA_METADATA) {
	    continue; // This is the N counter
	  }
	  psMetadata *chipData = psMetadataLookupPtr(NULL, expItem->data.md, workingChip);
	  if (!chipData) { continue; }
	  psImage *image      = psMetadataLookupPtr(NULL, chipData, "bkg image");
	  if (!image) { continue; }
	  psVectorAppend(tmp,image->data.F32[v][u]);
	} // End loop over exposures
	psStatsInit(stats);
	psVectorStats(stats,tmp,NULL,NULL,0);
	solution->data.F32[v][u] = stats->robustMedian;

	psFree(expIter);
	psFree(tmp);
      } // End u
    } // End v
    // Remove the median value from this data.  We just want the tilts, not the offsets
    psStatsInit(stats);
    psImageStats(stats,solution,NULL,0);
    for (v = 0; v < solution->numRows; v++) {
      for (u = 0; u < solution->numCols; u++) {
	solution->data.F32[v][u] -= stats->robustMedian;
      }
    }
    
    
  } // End working chip scan

  psFree(iter);
  psFree(stats);
  return(true);
}

//
// Determine the relative offset of this exposure/ota relative to the current estimate of the sky at this point.
// Note: the initial assumption of the model is a flat grid of zero, "the sky contains nothing real."
// SOLVE: model(r,d) = scale * data(r,d) + offset + solution_{OTA}(u,v)
// FOR: scale, offset
bool ppBackgroundStackDataModelFit(ppBackgroundStackData *data) {
  int u,v;

  psMetadataIterator *expIter = psMetadataIteratorAlloc(data->models, PS_LIST_HEAD, NULL);
  psMetadataItem *expItem;
  while ((expItem = psMetadataGetAndIncrement(expIter))) {
    if (expItem->type != PS_DATA_METADATA) {
      continue; // This is the N counter
    }
    
    psMetadataIterator *chipIter = psMetadataIteratorAlloc(expItem->data.md, PS_LIST_HEAD, NULL);
    psMetadataItem *chipItem;
    while ((chipItem = psMetadataGetAndIncrement(chipIter))) {
      //      const char *chipName = chipItem->name;
      
      psImage *image = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg image");
      psImage *ra    = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg ra");
      psImage *dec   = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg dec");
      //      psImage *camera= psMetadataLookupPtr(NULL, data->OTA_solutions, chipName);      
      psVector *obs  = psVectorAllocEmpty(image->numRows, PS_TYPE_F32);
      psVector *model= psVectorAllocEmpty(image->numRows, PS_TYPE_F32);

      int j = 0;
      int used = 0;
      for (v = 0; v < image->numRows; v++) {
	for (u = 0; u < image->numCols; u++) {
	  if ((ra->data.F32[v][u] < data->x_min)||(ra->data.F32[v][u] > data->x_max)||
	      (dec->data.F32[v][u] < data->y_min)||(dec->data.F32[v][u] > data->y_max)) { j++; continue; }
	  psVectorAppend(obs,image->data.F32[v][u]);// - camera->data.F32[v][u]);
	  psVectorAppend(model, psImageMapEval(data->modelMap,ra->data.F32[v][u],dec->data.F32[v][u]));
	  j++;
	  used++;
	}
      }

      if (used > 0) {

	psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD,1);
	int status = psVectorFitPolynomial1D(poly,NULL,0,model,NULL,obs);
	printf("in model fit loop: %d %d %d %f %f\n",status,j,used,poly->coeff[0],poly->coeff[1]);
	if (status && (poly->coeff[1] != 0.0)) {
	  psMetadataAddF32(chipItem->data.md,PS_LIST_TAIL,"bkg offset", PS_META_REPLACE, "background offset for this exposure/ota pair", poly->coeff[0]);
	  psMetadataAddF32(chipItem->data.md,PS_LIST_TAIL,"bkg scale", PS_META_REPLACE, "background scale for this exposure/ota pair", poly->coeff[1]);
	}
	psFree(poly);
      }
    } // End OTA loop
    psFree(chipIter);
  } // End smf/exp loop
  psFree(expIter);
  return(true);
}


//
// Apply the corrections to attempt to put all exposures, and all OTAs onto a common level,
//  such that they should all match the true sky
// calib_{exposure,OTA}(r,d) = scale * data_{exposure,OTA}(r,d) + offset + solution_{OTA}(u,v)
bool ppBackgroundStackCalibApply(ppBackgroundStackData *data) {
  int u,v;

  psMetadataIterator *expIter = psMetadataIteratorAlloc(data->models, PS_LIST_HEAD, NULL);
  psMetadataItem *expItem;
  while ((expItem = psMetadataGetAndIncrement(expIter))) {
    if (expItem->type != PS_DATA_METADATA) {
      continue; // This is the N counter
    }
    psMetadataIterator *chipIter = psMetadataIteratorAlloc(expItem->data.md, PS_LIST_HEAD, NULL);
    psMetadataItem *chipItem;
    while ((chipItem = psMetadataGetAndIncrement(chipIter))) {
      const char *chipName = chipItem->name;
      
      psImage *image = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg image");
      psImage *model = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg calibrated data");
      psImage *camera= psMetadataLookupPtr(NULL, data->OTA_solutions, chipName);
      psImage *ra    = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg ra");
      psImage *dec   = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg dec");
      
      psF32 offset = psMetadataLookupF32(NULL,chipItem->data.md,"bkg offset");
      psF32 scale  = psMetadataLookupF32(NULL,chipItem->data.md,"bkg scale");

      for (v = 0; v < image->numRows; v++) {
	for (u = 0; u < image->numCols; u++) {
	  if ((ra->data.F32[v][u] < data->x_min)||(ra->data.F32[v][u] > data->x_max)||
	      (dec->data.F32[v][u] < data->y_min)||(dec->data.F32[v][u] > data->y_max)) { continue; }

	  model->data.F32[v][u] = scale * image->data.F32[v][u] + offset - camera->data.F32[v][u];
	}
      }
    } // End chip
    psFree(chipIter);
  } // End smf
  psFree(expIter);
  return(true);
}

// 
// Determine the best estimate of the true sky from the ensemble of calibrated samples:
//  model(r,d) = < calib(r,d) >_{exposures,OTAs}
// This "averaging" is done using the psImageMapClipFit.
bool ppBackgroundStackModelFit(ppBackgroundStackData *data) {
  long j = 0;
  int u,v;

  long used = 0;
  psS16 N = psMetadataLookupS16(NULL, data->models, "N");
  psVector *X = psVectorAllocEmpty(N * 13 * 13,PS_TYPE_F32);
  psVector *Y = psVectorAllocEmpty(N * 13 * 13,PS_TYPE_F32);
  psVector *Z = psVectorAllocEmpty(N * 13 * 13,PS_TYPE_F32);
  psVector *E = psVectorAllocEmpty(N * 13 * 13,PS_TYPE_F32);
  psVector *mask = psVectorAllocEmpty(N * 13 * 13,PS_TYPE_VECTOR_MASK);
  j = 0;
  
  pmFPAview *view    = pmFPAviewAlloc(0);
  psStats *stats     = psStatsAlloc( PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV );

  psMetadataIterator *expIter = psMetadataIteratorAlloc(data->models, PS_LIST_HEAD, NULL);
  psMetadataItem *expItem;
  while ((expItem = psMetadataGetAndIncrement(expIter))) {
    if (expItem->type != PS_DATA_METADATA) {
      continue; // This is the N counter
    }
    psMetadataIterator *chipIter = psMetadataIteratorAlloc(expItem->data.md, PS_LIST_HEAD, NULL);
    psMetadataItem *chipItem;
    while ((chipItem = psMetadataGetAndIncrement(chipIter))) {
      psImage *calib = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg calibrated data");
      psImage *ra    = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg ra");
      psImage *dec   = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg dec");
#define DUMP_DATA 0
#if DUMP_DATA
      psImage *model = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg image");

      psF32 offset = psMetadataLookupF32(NULL,chipItem->data.md,"bkg offset");
      psF32 scale  = psMetadataLookupF32(NULL,chipItem->data.md,"bkg scale");

#endif
      for (v = 0; v < calib->numRows; v++) {
	for (u = 0; u < calib->numCols; u++) {
	  if ((ra->data.F32[v][u] < data->x_min)||(ra->data.F32[v][u] > data->x_max)||
	      (dec->data.F32[v][u] < data->y_min)||(dec->data.F32[v][u] > data->y_max)) {
	    j++;	    
	    continue; }
	  psVectorAppend(X,ra->data.F32[v][u]);
	  psVectorAppend(Y,dec->data.F32[v][u]);
	  psVectorAppend(Z,calib->data.F32[v][u]);
	  psVectorAppend(E,1.0);
	  psVectorAppend(mask,0);
/* 	  X->data.F32[j] = ra->data.F32[v][u]; */
/* 	  Y->data.F32[j] = dec->data.F32[v][u]; */
/* 	  Z->data.F32[j] = calib->data.F32[v][u]; */
/* 	  E->data.F32[j] = 1.0; */
/* 	  mask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 0; */
	  used++;
	  j++;
#if DUMP_DATA
	  printf("DATA %d %ld %ld %f %f %f %f %f %f\n",
		 data->model_iteration,j,used,
		 offset,scale,
		 ra->data.F32[v][u],
		 dec->data.F32[v][u],
		 calib->data.F32[v][u],
		 model->data.F32[v][u]);
#endif 
	}
      }
    } // End chip
    psFree(chipIter);
  } // End smfs
  printf("%ld %ld\n", j,used);
  bool fitStatus;
  bool status = psImageMapClipFit(&fitStatus,data->modelMap,stats, mask, 1, X, Y, Z, E);
  data->model_iteration++;
  psFree(expIter);
  psFree(X);
  psFree(Y);
  psFree(Z);
  psFree(E);
  psFree(mask);
  psFree(stats);
  psFree(view);
  return(status);
}
