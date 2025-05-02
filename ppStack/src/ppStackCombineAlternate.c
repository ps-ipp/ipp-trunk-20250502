#include "ppStack.h"

// This is the doomsday switch.
// #define TESTING                         // Enable test output
bool ppStackCombineMedian(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config)
{
  psAssert(stack, "Require stack");
  psAssert(options, "Require options");
  psAssert(config, "Require configuration");

  psTimerStart("PPSTACK_BKGMED");


  pmReadout *outRO = options->outRO;
  
  psArray *inputs  = psArrayAlloc(options->num);
  for (int i = 0; i < options->num; i++) {
    ppStackFileActivationSingle(config, PPSTACK_FILES_MEDIAN_IN, true, i);
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i);
    pmFPAview *view = ppStackFilesIterateDown(config);
    pmReadout *ro = pmFPAviewThisReadout(view,file->fpa);
    inputs->data[i] = ro;
    pmFPAfileClose(file,view);
  }

  // Scaling block
  if (!ppStackLinearScale(inputs, config)) {
    psFree(inputs);
    return(false);
  }

  if (!pmStackSimpleMedianCombine(outRO,inputs)) {
    psFree(inputs);
    return(false);
  }

#if 0
  if (!ppStackWriteImage("/tmp/test_forced.median.fits",
			 outRO->parent->parent->parent->hdu->header,
			 outRO->image,
			 config)) {
    fprintf(stderr,"Failed to write image because fail.\n");
  }
#endif
  for (int i = 0; i < options->num; i++) {
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i);
    pmFPAview *view = ppStackFilesIterateDown(config);
    bool success = pmFPAfileClose(file,view);
    if (!success) {
      psTrace("ppStack",5,"I failed at closing a file.\n");
    }
	      
    psFitsClose(file->fits);
    file->fits = NULL;
    file->header = NULL;
    file->state = PM_FPA_STATE_CLOSED;
    file->wrote_phu = false;
    ppStackFileActivationSingle(config, PPSTACK_FILES_MEDIAN_IN, false, i);
  }
  psFree(inputs);
  outRO->data_exists = true;
  outRO->parent->data_exists = true;
  outRO->parent->parent->data_exists = true;
  
  return(true);
}
  

bool ppStackCombineBackground(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config)
{
  psAssert(stack, "Require stack");
  psAssert(options, "Require options");
  psAssert(config, "Require configuration");

  psTimerStart("PPSTACK_BKGMED");


  pmReadout *bkgRO = options->bkgRO;
  
  psArray *inputs  = psArrayAlloc(options->num);
  for (int i = 0; i < options->num; i++) {
    ppStackFileActivationSingle(config, PPSTACK_FILES_BKG, true, i);
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.BKGMODEL", i);
    pmFPAview *view = ppStackFilesIterateDown(config);
    pmReadout *ro = pmFPAviewThisReadout(view,file->fpa);
    inputs->data[i] = ro;
    pmFPAfileClose(file,view);
  }

  if (!ppStackLinearScale(inputs, config)) {
    psFree(inputs);
    return(false);
  }

  // Do combination
  if (!pmStackSimpleMedianCombine(bkgRO,inputs)) {
    psFree(inputs);
    return(false);
  }
#if 0
  if (!ppStackWriteImage("/tmp/test_forced.bkgmdl.fits",
			 bkgRO->parent->parent->parent->hdu->header,
			 bkgRO->image,
			 config)) {
    fprintf(stderr,"Failed to write image because fail.\n");
  }
#endif
  for (int i = 0; i < options->num; i++) {
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.BKGMODEL", i);
    pmFPAview *view = ppStackFilesIterateDown(config);
    bool success = pmFPAfileClose(file,view);
    if (!success) {
      psTrace("ppStack",5,"I failed at closing a file.\n");
    }
	      
    psFitsClose(file->fits);
    file->fits = NULL;
    file->header = NULL;
    file->state = PM_FPA_STATE_CLOSED;
    file->wrote_phu = false;
    ppStackFileActivationSingle(config, PPSTACK_FILES_BKG, false, i);
  }
  psFree(inputs);
  bkgRO->data_exists = true;
  bkgRO->parent->data_exists = true;
  bkgRO->parent->parent->data_exists = true;
  
  return(true);
}

bool ppStackLinearScale (psArray *inputs, pmConfig *config)  {
  psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE);
  bool doLinearScaling = psMetadataLookupBool(NULL, recipe, "DO.LINEAR.INPUT.SCALING");
  if (doLinearScaling) {
    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image
    
    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize,inputs)) {
      psError(psErrorCodeLast(), false, "Input stack is not valid.");
      psFree(inputs);
      return false;
    }
    
    // Determine best input.
    int ref = 0;
    int Nref = 0;
    double refScale = 0;
    
    for (int i = 0; i < inputs->n; i++) {
      pmReadout *ro = inputs->data[i];
      psImage *roImage = ro->image;
      int Nvalid = 0;
      double scale = 0;

      for (int y = minInputRows; y < maxInputRows; y++) {
	for (int x = minInputCols; x < maxInputCols; x++) {
	  if ((isfinite(roImage->data.F32[y][x]))) {
	    Nvalid += 1;
	    scale += roImage->data.F32[y][x];
	  }
	}
      }
      if ((scale > refScale)&&(Nvalid > 0.9 * Nref)) {
	ref = i;
	refScale = scale;
	Nref = Nvalid;
      }
    }
    fprintf(stderr,"ref: %d %d %f\n",ref,Nref,refScale);
    // Calculate scaling factors
    pmReadout *refReadout = inputs->data[ref];
    psImage *refImage     = refReadout->image;
    for (int i = 0; i < inputs->n; i++) {
      pmReadout *ro = inputs->data[i];
      psImage *roImage = ro->image;
      double S = 0.0;
      double Sx = 0.0;
      double Sy = 0.0;
      double Sxx = 0.0;
      double Syy = 0.0;
      double Sxy = 0.0;
      double D = 0.0;
      double offset = 0.0;
      double scale  = 0.0;
      
      for (int y = minInputRows; y < maxInputRows; y++) {
	for (int x = minInputCols; x < maxInputCols; x++) {
	  if ((isfinite(refImage->data.F32[y][x]))&&
	      (isfinite(roImage->data.F32[y][x]))) {
	    S += 1.0;
	    Sx += roImage->data.F32[y][x];
	    Sy += refImage->data.F32[y][x];
	    
	    Sxx += pow(roImage->data.F32[y][x],2);
	    Syy += pow(refImage->data.F32[y][x],2);
	    Sxy += roImage->data.F32[y][x] * refImage->data.F32[y][x];
	  }
	}
      }
      
      D = S * Sxx - Sx * Sx;
      offset = (Sy * Sxx - Sx * Sxy) / D;
      scale  = (S * Sxy - Sx * Sy) / D;
      fprintf(stderr,"Scales: %d %g %g %g %g %g %g %g %g\n",i,offset,scale,D,Sx,Sy,Sxx,Syy,Sxy);
      // Apply scaling factors
      for (int y = minInputRows; y < maxInputRows; y++) {
	for (int x = minInputCols; x < maxInputCols; x++) {
	  roImage->data.F32[y][x] = offset + scale * roImage->data.F32[y][x];
	}
      }
    }      
  }
  return(true);
}


