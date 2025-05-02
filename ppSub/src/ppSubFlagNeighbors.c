/** @file ppSubFlagNeighbors.c
 *
 *  @ingroup ppSub
 *  @author EAM, IfA
 *  @date $Date: 2009-02-18 00:31:20 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"

# define MIN_ERROR 0.02 /* padding for self-match flux test */
# define MAX_OFFSET 5.0 /* delta mag greater than this is not a self match */

// match the diff sources to the detections from the positive or negative input images
// matchRef : false if this is the standard diff image, true if this is the inverse (in-ref vs ref-in)
// this is needed to choose which value is positive and which is negative relative to the difference
bool ppSubFlagNeighbors(pmConfig *config, pmFPAview *view, psArray *sources, bool subInverse) {

    bool status;

    psAssert(config, "Require config");
    psAssert(view, "Require config");
    psAssert (sources, "missing sources?");

    // Look up recipe values
    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSUB_RECIPE); // should this be psphot?
    psAssert(recipe, "We checked this earlier, so it should be here.");

    float radius = psMetadataLookupF32(&status, recipe, "INPUT.MATCH.RADIUS");
    psAssert (status, "cannot find INPUT.MATCH.RADIUS");

    float minSN  = psMetadataLookupF32(&status, recipe, "INPUT.MATCH.MIN.SN");
    psAssert (status, "cannot find INPUT.MATCH.MIN.SN");

    // Input sources
    pmReadout *inSourceRO = pmFPAfileThisReadout(config->files, view, "PPSUB.INPUT.SOURCES");
    pmDetections *inDetections = inSourceRO ? psMetadataLookupPtr(NULL, inSourceRO->analysis, "PSPHOT.DETECTIONS") : NULL; // Input sources
    if (!inDetections) {
        psWarning("Unable to filter detections based on image A.");
    }

    pmReadout *refSourceRO = pmFPAfileThisReadout(config->files, view, "PPSUB.REF.SOURCES");
    pmDetections *refDetections = refSourceRO ? psMetadataLookupPtr(NULL, refSourceRO->analysis, "PSPHOT.DETECTIONS") : NULL; // Ref sources
    if (!refDetections) {
        psWarning("Unable to filter detections based on image B.");
    }
    if (!inDetections && !refDetections) {
	return true;
    }

    psArray *inSources = inDetections->allSources;
    psAssert (inSources, "missing inSources?");

    psArray *refSources = refDetections->allSources;
    psAssert (refSources, "missing refSources?");

    ppSubSetSourceImageIDs(sources, 0);
    ppSubSetSourceImageIDs(inSources, 1);
    ppSubSetSourceImageIDs(refSources, 2);

    // define the objects from the diff sources (1-to-1)
    psArray *objects = psArrayAllocEmpty(sources->n);
    for (int i = 0; i < sources->n; i++) {
        pmSource *src = sources->data[i]; 
	pmPhotObj *obj = pmPhotObjAlloc();
	pmPhotObjAddSource(obj, src);
	psArrayAdd (objects, 100, obj);
	psFree (obj);
    }

    // Match the sources to both inSources and refSources.  We want to find the closest source
    // in the positive source lists to each source in the diff source list.
    // XXX we need to either (a) apply S/N and other filters before we pass the in/ref sources
    // to this function, or (b) add those filters in the function
    ppSubMatchSources (objects, inSources, radius, minSN);
    ppSubMatchSources (objects, refSources, radius, minSN);
    
    // now mark or flag the sources with matches that meet some criterion
    for (int i = 0; i < objects->n; i++) {
        pmPhotObj *obj = objects->data[i]; 

	// the first entry should be from image 0 (the diff image).
	// we should have only 1 of 4 possible results:
	// a) only one entry -- the diff source (no positive matches)
	// b) 2 matches -- diff and a source from image 1 
	// c) 2 matches -- diff and a source from image 2
	// d) 3 matches -- diff and a source from image 1 & 2

	switch (obj->sources->n) {
	  case 1: {
	      pmSource *source = obj->sources->data[0];
	      psAssert (source->imageID == 0, "error in pmPhotObj construction: first entry is not image ID 0");
	      break;
	  }
	  case 2: {
	      pmSource *source = obj->sources->data[0];
	      psAssert (source->imageID == 0, "error in pmPhotObj construction: first entry not 0 (case 2)");

	      pmSource *sourceM1 = obj->sources->data[1];
	      psAssert ((sourceM1->imageID == 1) || (sourceM1->imageID == 2), "error in pmPhotObj construction (second entry is not from 1 or 2");

	      // which image gives the match, the positive or negative one?
	      bool positive = false;
	      if (subInverse) {
		  positive = (sourceM1->imageID == 2);
	      } else {
		  positive = (sourceM1->imageID == 1);
	      }

	      // check if source and sourceM1 are likely to be the same flux (ie, isolated detection in one image)
	      if (positive && isfinite(sourceM1->psfMag) && isfinite(source->psfMag)) {
		  float dMag = sourceM1->psfMag - source->psfMag;
		  float nSig = fabs(dMag) / hypot(MIN_ERROR, source->psfMagErr);
		  if (nSig < MAX_OFFSET) {
		      source->mode2 |= PM_SOURCE_MODE2_DIFF_SELF_MATCH;
		  }
	      }

	      // only one match.  set a flag?
	      source->mode2 |= PM_SOURCE_MODE2_DIFF_WITH_SINGLE;
	      if (source->diffStats) {
		  float SN1 = isfinite(sourceM1->psfMagErr) ? 1.0 / sourceM1->psfMagErr : NAN;
		  if (positive) {
		      source->diffStats->SNp = SN1;
		      source->diffStats->Rp  = hypot(source->peak->xf - sourceM1->peak->xf, source->peak->yf - sourceM1->peak->yf);
		  } else {
		      source->diffStats->SNm = SN1;
		      source->diffStats->Rm  = hypot(source->peak->xf - sourceM1->peak->xf, source->peak->yf - sourceM1->peak->yf);
		  }
	      }
	      break;
	  }
	  case 3: {
	      pmSource *source = obj->sources->data[0];
	      psAssert (source->imageID == 0, "error in pmPhotObj construction: first entry not 0 (case 3)");

	      pmSource *sourceM1 = obj->sources->data[1];
	      pmSource *sourceM2 = obj->sources->data[2];
	      psAssert ((sourceM1->imageID == 1) || (sourceM1->imageID == 2), "error in pmPhotObj construction (case 3.1)");
	      psAssert ((sourceM2->imageID == 1) || (sourceM2->imageID == 2), "error in pmPhotObj construction (case 3.2)");
	      psAssert (sourceM1->imageID != sourceM2->imageID, "error in pmPhotObj construction (case 3.3)");

	      // we don't know which matched source (sourceM1 or sourceM2) is the positive detection,
	      // and the answer depends on both the ID and the subtraction order (in-ref vs ref-in)
	      // positive is true if sourceM1 is the positive detection
	      bool positive = false;
	      if (subInverse) {
		  positive = (sourceM1->imageID == 2);
	      } else {
		  positive = (sourceM1->imageID == 1);
	      }

	      // check for self-detection
	      float magPSF1 = positive ? sourceM1->psfMag : sourceM2->psfMag;
	      if (isfinite(magPSF1) && isfinite(source->psfMag)) {
		  float dMag = magPSF1 - source->psfMag;
		  float nSig = fabs(dMag) / hypot(MIN_ERROR, source->psfMagErr);
		  if (nSig < MAX_OFFSET) {
		      source->mode2 |= PM_SOURCE_MODE2_DIFF_SELF_MATCH;
		  }
	      }

	      source->mode2 |= PM_SOURCE_MODE2_DIFF_WITH_DOUBLE;
	      if (source->diffStats) {
		  float SN1 = isfinite(sourceM1->psfMagErr) ? 1.0 / sourceM1->psfMagErr : NAN;
		  float SN2 = isfinite(sourceM2->psfMagErr) ? 1.0 / sourceM2->psfMagErr : NAN;
		  if (positive) {
		      source->diffStats->SNp = SN1;
		      source->diffStats->Rp  = hypot(source->peak->xf - sourceM1->peak->xf, source->peak->yf - sourceM1->peak->yf);
		      source->diffStats->SNm = SN2;
		      source->diffStats->Rm  = hypot(source->peak->xf - sourceM2->peak->xf, source->peak->yf - sourceM2->peak->yf);
		  } else {
		      source->diffStats->SNp = SN2;
		      source->diffStats->Rp  = hypot(source->peak->xf - sourceM2->peak->xf, source->peak->yf - sourceM2->peak->yf);
		      source->diffStats->SNm = SN1;
		      source->diffStats->Rm  = hypot(source->peak->xf - sourceM1->peak->xf, source->peak->yf - sourceM1->peak->yf);
		  }
	      }
	      break;
	  }
	  default:
	    psAbort ("error in pmPhotObj construction (unexpected matches)");
	}
    }
    psFree (objects);
    return true;
}

// XXX based on psphotMatchSourcesToObjects
# define NEXT1 { i++; continue; } 
# define NEXT2 { j++; continue; } 
bool ppSubMatchSources (psArray *objects, psArray *sources, float RADIUS, float MIN_SN) { 
 
    float dx, dy; 
 
    float RADIUS2 = RADIUS*RADIUS;

    // sort the source list by X 
    sources = psArraySort (sources, pmSourceSortByX); 
    objects = psArraySort (objects, pmPhotObjSortByX); 
 
    // match sources to existing objects

    psLogMsg ("psphot", PS_LOG_DETAIL, "attempt to match sources (%ld vs %ld) (SN > %f)", sources->n, objects->n, MIN_SN);

    int nMatch = 0;
    int i = 0;
    int j = 0;
    for (i = j = 0; (i < sources->n) && (j < objects->n); ) { 
 
        pmSource  *src = sources->data[i]; 
        pmPhotObj *obj = objects->data[j]; 
 
	// fprintf (stderr, "%f, %f vs %f, %f\n", src->peak->xf, src->peak->yf, obj->x, obj->y);

        if (!src) NEXT1; 
        if (!src->peak) NEXT1; 
        if (!isfinite(src->peak->xf)) NEXT1; 
        if (!isfinite(src->peak->yf)) NEXT1; 
        if (!isfinite(src->peak->detValue)) NEXT1; 
        if (sqrt(src->peak->detValue) < MIN_SN) NEXT1; 
 
        if (!obj) NEXT2; 
        if (!isfinite(obj->x)) NEXT2; 
        if (!isfinite(obj->y)) NEXT2; 
 
        dx = src->peak->xf - obj->x; 
        if (dx < -1.02*RADIUS) NEXT1; 
        if (dx > +1.02*RADIUS) NEXT2; 
 
        // we are within match range, look for closest match to object j
	int Imin = -1;
	float Rmin = RADIUS2;
        for (int I = i; (dx < +1.02*RADIUS) && (I < sources->n); I++) { 
 
	    src = sources->data[I]; 
	    
	    if (!src) continue; 
	    if (!src->peak) continue; 
	    if (!isfinite(src->peak->xf)) continue; 
	    if (!isfinite(src->peak->yf)) continue; 
	    if (!isfinite(src->peak->detValue)) continue; 
	    if (sqrt(src->peak->detValue) < MIN_SN) continue; 

	    dx = src->peak->xf - obj->x; 
            dy = src->peak->yf - obj->y; 
 
            float dr = dx*dx + dy*dy; 
            if (dr > RADIUS2) continue; 
	    if (dr > Rmin) continue;
	    Rmin = dr;
	    Imin  = I;
	    // fprintf (stderr, "j: %d, I: %d, Imin: %d, dr: %f, dx: %f, dy: %f\n", j, I, Imin, dr, dx, dy);
	}

	// no match, try next object
	if (Imin == -1) {
	    // fprintf (stderr, "*** missed j: %d, Imin: %d, obj x,y: %f, %f\n", j, Imin, obj->x, obj->y);
	    j++;
	    continue;
	}
	src = sources->data[Imin]; 

	// fprintf (stderr, "j: %d, Imin: %d, obj x,y: %f, %f  src x,y: %f, %f, SN: %f, ID: %d\n", j, Imin, obj->x, obj->y, src->peak->xf, src->peak->yf, sqrt(src->peak->detValue), src->id);

	// add source to object
	pmPhotObjAddSource (obj, src);
	nMatch ++;
        j++; 
    } 

    psLogMsg ("psphot", PS_LOG_DETAIL, "matched sources (%d matches)", nMatch);
    return true;
} 

bool ppSubSetSourceImageIDs (psArray *sources, int imageID) {

    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	source->imageID = imageID;
    }
    return true;
}
