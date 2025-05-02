#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

#include "ppBackgroundStack.h"

#define WCS_TOLERANCE 0.001             // Tolerance for WCS


bool ppBackgroundStackLoop(ppBackgroundStackData *data // Run-time data
    )
{
  pmConfig *config = data->config;                                        // Configuration data
    int i;
    //
    // Solve the data into a consistent model.
    { // This block does the initialization
      // If we didn't load the OTA solution from an external source, we need to build one.
      if (data->fit_OTAS) {
	printf("Fitting OTAs!\n");
	if (!ppBackgroundStackModelFitOTASolution(data)) {
	  // Currently can't fail.
	  psError(psErrorCodeLast(), false, "Error calculating the OTA solution");
	  return(false);
	}

	if (data->OTApath) {
	  // This will write the solutions out.
	  psMetadataIterator *iter = psMetadataIteratorAlloc(data->OTA_solutions, PS_LIST_HEAD, NULL); // Iterator
	  psMetadataItem *item; // Item from iteration
	  int i = 0;
	  while ((item = psMetadataGetAndIncrement(iter))) {
	    i++;
	    if (item->type != PS_DATA_IMAGE) {
	      psString chipName = item->name;
	      psImage  *image    = item->data.V;
	      psMetadata *header = psMetadataAlloc();
	      psString solutionFileName = psStringCopy(data->OTApath);
	      psStringAppend(&solutionFileName, ".%s.fits",chipName);
	      psString resolved = pmConfigConvertFilename(solutionFileName,config,true,true);
	      psFits *solutionFits = psFitsOpen(resolved,"w");
	      if (!solutionFits) {
		psError(2, false, "Unable to open FITS file %s to write model %s.", resolved, chipName);
		psFitsClose(solutionFits);
		psFree(resolved);
		return(false);
	      }
	      if (!psFitsWriteImage(solutionFits, header, image, 0, NULL)) {
		psError(2, false, "Unable to write FITS image %s.", resolved);
		psFitsClose(solutionFits);
		psFree(resolved);
		return false;
	      }
	      if (!psFitsClose(solutionFits)) {
		psError(2, false, "Unable to close FITS image %s.", resolved);
		psFree(resolved);
		return false;
	      }
	      psFree(resolved);
	      psFree(solutionFileName);
	    }
	  }
	  psFree(item);
	  psFree(iter);
	}
	
      }
    } // End initialization block.      


    // Loop over the input images, and apply the models to construct the restored versions.
    for (i = 0; i < data->stack_data->n; i++) {
      pmFPAfile *stack = data->stack_data->data[i];
      pmFPAview *view = pmFPAviewAlloc(0);

      //      pmHDU *phu = pmFPAviewThisPHU(view, stack->fpa);
      psF32 exptime = 1.0;
      exptime = psMetadataLookupF32(NULL, stack->fpa->hdu->header, "EXPTIME");
      
      // PART 1:
      // Determine the extent of the model map for this stack
      data->x_min = 99e99; data->x_max = -99e99;
      data->y_min = 99e99; data->y_max = -99e99;
      // Allocate the modelMap for the region we're covering.
      psPlane *pix = psPlaneAlloc();   // Pixel coordinates on chip
      psPlane *tp = psPlaneAlloc();    // Focal plane coordinates
      
      pix->x = 0; pix->y = 0;
      psPlaneTransformApply(tp, stack->fpa->toTPA, pix);
      printf("%f %f -> %f %f\n",pix->x,pix->y,tp->x,tp->y);
      if (tp->x < data->x_min) { data->x_min = tp->x; }
      if (tp->x > data->x_max) { data->x_max = tp->x; }
      if (tp->y < data->y_min) { data->y_min = tp->y; }
      if (tp->y > data->y_max) { data->y_max = tp->y; }

      pix->x = 6240; pix->y = 0;
      psPlaneTransformApply(tp, stack->fpa->toTPA, pix);
      printf("%f %f -> %f %f\n",pix->x,pix->y,tp->x,tp->y);
      if (tp->x < data->x_min) { data->x_min = tp->x; }
      if (tp->x > data->x_max) { data->x_max = tp->x; }
      if (tp->y < data->y_min) { data->y_min = tp->y; }
      if (tp->y > data->y_max) { data->y_max = tp->y; }

      pix->x = 6240; pix->y = 6243;
      psPlaneTransformApply(tp, stack->fpa->toTPA, pix);
      printf("%f %f -> %f %f\n",pix->x,pix->y,tp->x,tp->y);
      if (tp->x < data->x_min) { data->x_min = tp->x; }
      if (tp->x > data->x_max) { data->x_max = tp->x; }
      if (tp->y < data->y_min) { data->y_min = tp->y; }
      if (tp->y > data->y_max) { data->y_max = tp->y; }

      pix->x = 0; pix->y = 6243;
      psPlaneTransformApply(tp, stack->fpa->toTPA, pix);
      printf("%f %f -> %f %f\n",pix->x,pix->y,tp->x,tp->y);
      if (tp->x < data->x_min) { data->x_min = tp->x; }
      if (tp->x > data->x_max) { data->x_max = tp->x; }
      if (tp->y < data->y_min) { data->y_min = tp->y; }
      if (tp->y > data->y_max) { data->y_max = tp->y; }

      psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
      psImageBinning *binning = psImageBinningAlloc();
      binning->nXruff = 13; // Number of samples
      binning->nYruff = 13; 
      binning->nXfine = ceil(data->x_max - data->x_min) + 1;
      binning->nYfine = ceil(data->y_max - data->y_min) + 1;
      binning->nXskip = floor(data->x_min - data->ra_min) + 1;
      binning->nYskip = floor(data->y_min - data->dec_min) + 1;
      psImageBinningSetScale(binning,PS_IMAGE_BINNING_CENTER);
      printf("Sizes: sky: %f %f -> %f %f :: tp: %f %f -> %f %f :: map: %d %d\n",
	     0.0,0.0,0.0,0.0,data->ra_min,data->dec_min,data->ra_max,data->dec_max,binning->nXfine,binning->nYfine);
      printf("Sizes: corners: %f %f -> %f %f map: %d %d\n",
	     data->x_min,data->y_min,data->x_max,data->y_max,
	     binning->nXfine,binning->nYfine);
      psImage *sizeImage = psImageAlloc(binning->nXfine,binning->nYfine,PS_TYPE_F32);
      P_PSIMAGE_SET_COL0(sizeImage, data->x_min);
      P_PSIMAGE_SET_ROW0(sizeImage, data->y_min);
      data->modelMap = psImageMapAlloc(sizeImage,binning,stats);
      
      // PART 2:
      // Solve the data into a model for this region of the sky.

      // This seems wrong, but I need a blank modelMap object, so we fit the zero-data we've stored in the calib objects
      printf("Determining blank modelMap!\n");
      if (!ppBackgroundStackModelFit(data)) {
	psError(psErrorCodeLast(), false, "Error determining the blank modelMap object.");
	return(false);
      }
      
      // Apply OTA solution
      printf("Calib apply!\n");
      if (!ppBackgroundStackCalibApply(data)) {
	psError(psErrorCodeLast(), false, "Error applying the calibration models.");
	return(false);
      }

      // This is where an iterative solution loop would likely start.
      for (int iterator = 0; iterator < 4; iterator++) {
	// Construct the offset information
	printf("Model fit!\n");
	if (!ppBackgroundStackDataModelFit(data)) {
	  psError(psErrorCodeLast(), false, "Error determining the exposure/OTA scaling.");
	  return(false);
	}
	
	// Apply full correction
	printf("Calib apply!\n");
	if (!ppBackgroundStackCalibApply(data)) {
	  psError(psErrorCodeLast(), false, "Error applying the calibration models.");
	  return(false);
	}      
	
	// Fit the new model
	printf("Determining model!\n");
	if (!ppBackgroundStackModelFit(data)) {
	  psError(psErrorCodeLast(), false, "Error determining the modelMap object.");
	  return(false);
	}
      } // End loop

      // PART 3:
      // Define output products
      // Define output image.  Why is this always so hard to do?
      pmFPA *tmp_fpa1,*tmp_fpa2;
      tmp_fpa1 = pmFPAConstruct(config->camera,config->cameraName);
      tmp_fpa2 = pmFPAConstruct(config->camera,config->cameraName);
      
      pmFPAfile *stack_model = pmFPAfileDefineOutput(config,tmp_fpa1,"PPBACKGROUND.STACK.MODEL");
      
      if (!stack_model) {
	psError(psErrorCodeLast(), false, "Unable to generate output model");
	return (false);
      }
      
      pmFPAfile *stack_corr  = pmFPAfileDefineOutput(config,tmp_fpa2,"PPBACKGROUND.STACK.OUTPUT");
      if (!stack_corr) {
	psError(psErrorCodeLast(), false, "Unable to generate output result");
	return (false);
      }
      stack_model->save = true;
      stack_corr->save = true;
      
      printf("I'm about to loop over the parts of this stack: %d\n",i);
      // Iterate over the images.
      pmFPAfileActivate(config->files,true,"PPBACKGROUND.STACK");
      pmFPAfileActivate(config->files,true,"PPBACKGROUND.STACK.MODEL");
      pmFPAfileActivate(config->files,true,"PPBACKGROUND.STACK.OUTPUT");
      if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psError(psErrorCodeLast(), false, "load failure for Chip");
	return(false);
      }
      
      pmChip *chip;
      while ((chip = pmFPAviewNextChip(view, stack->fpa, 1))) {
	if (!chip->process || !chip->file_exists) {
	  continue;
	}
	
	if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	  psError(psErrorCodeLast(), false, "load failure for Chip");
	  return(false);
	}
	printf("  I'm in a chip\n");
	pmCell *cell;
	
	while ((cell = pmFPAviewNextCell(view, stack->fpa, 1)) != NULL) {
	  psLogMsg ("ppImageLoop", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
	  if (!cell->process || !cell->file_exists) {
	    continue;
	  }
	  if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	    psError(psErrorCodeLast(), false, "load failure for Cell");
	    return(false);
	  }
	  if (cell->readouts->n > 1) {
	    psWarning ("Skipping Video Cell for ppImageDetrendReadout");
	    continue;
	  }
	  printf("    I'm in a cell\n");


	  // process each of the readouts
	  pmReadout *readout;         // Readout from cell
	  while ((readout = pmFPAviewNextReadout (view, stack->fpa, 1)) != NULL) {
	    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	      	psError(psErrorCodeLast(), false, "load failure for Readout");
		return(false);
	    }
	    if (!readout->data_exists) {
	      continue;
	    }
	    printf("      I'm in a readout\n");

	    // Futz with things to get an acceptable output product.
	    pmCell *model_cell = pmFPAviewThisCell(view,stack_model->fpa);
	    pmCell *corr_cell  = pmFPAviewThisCell(view,stack_corr->fpa);
	    
	    pmReadout *model = pmReadoutAlloc(model_cell);
	    pmReadout *corr  = pmReadoutAlloc(corr_cell);
	    model->data_exists = true;
	    corr->data_exists = true;
	    model->parent->data_exists = true;
	    corr->parent->data_exists = true;
	    model->parent->parent->data_exists = true;
	    corr->parent->parent->data_exists = true;
	    model->image = psImageAlloc(readout->image->numCols,readout->image->numRows,PS_TYPE_F32);
	    corr->image = psImageAlloc(readout->image->numCols,readout->image->numRows,PS_TYPE_F32);

	    model_cell->concepts = psMemIncrRefCounter(cell->concepts);
	    model_cell->conceptsRead = cell->conceptsRead;
	    corr_cell->concepts = psMemIncrRefCounter(cell->concepts);
	    corr_cell->conceptsRead = cell->conceptsRead;
	    
	    psPlane *pix = psPlaneAlloc();   // Pixel coordinates on chip
	    psPlane *fp = psPlaneAlloc();    // Focal plane coordinates
	    psPlane *tp = psPlaneAlloc();    // Tangent plane coordinates
	    
	    int x,y;
	    for (y = 0; y < readout->image->numRows; y++) {
	      pix->y = y;
	      for (x = 0; x < readout->image->numCols; x++) {
		pix->x = x;
		// Calculate model for each pixel of output
		//		psPlaneTransformApply(fp, chip->toFPA, pix);
		psPlaneTransformApply(tp, stack->fpa->toTPA, pix);
		
		model->image->data.F32[y][x] = exptime * psImageMapEval(data->modelMap,tp->x,tp->y);
		corr->image->data.F32[y][x] = readout->image->data.F32[y][x] + model->image->data.F32[y][x];
		
	      }
	    }
	    psFree(pix);
	    psFree(fp);
	    psFree(tp);

	    // Copy WCS (from ppStackUpdateHeader)
	    pmHDU *inHDU = pmHDUFromCell(readout->parent);
	    model->parent->hdu = pmHDUAlloc(NULL);
	    corr->parent->hdu = pmHDUAlloc(NULL);
	    pmHDU *modHDU= pmHDUFromCell(model->parent);
	    pmHDU *corHDU= pmHDUFromCell(corr->parent);

	    if (!modHDU || !inHDU) {
	      psWarning("Unable to find HDU at FPA level to copy wcs!");
	    }
	    else {
	      if (!pmAstromReadWCS(stack_model->fpa,model_cell->parent,inHDU->header,1.0)) {
		psErrorClear();
		psWarning("Unable to read WCS astrometry from input FPA!");
	      }
	      else {
		if (!modHDU->header) {
		  modHDU->header = psMetadataAlloc();
		}
		if (!pmAstromWriteWCS(modHDU->header, stack_model->fpa,model_cell->parent, WCS_TOLERANCE)) {
		  psErrorClear();
		  psWarning("Unable to read WCS astrometry from input FPA!");
		}
	      }
	      if (!pmAstromReadWCS(stack_corr->fpa,corr_cell->parent,inHDU->header,1.0)) {
		psErrorClear();
		psWarning("Unable to read WCS astrometry from input FPA!");
	      }
	      else {
		if (!corHDU->header) {
		  corHDU->header = psMetadataAlloc();
		}
		if (!pmAstromWriteWCS(corHDU->header, stack_corr->fpa,corr_cell->parent, WCS_TOLERANCE)) {
		  psErrorClear();
		  psWarning("Unable to read WCS astrometry from input FPA!");
		}
	      }
	    } // End WCS saving.

	    
	  } // Close readout
	  printf("    I'm done with that readout\n");
	  // Close output image
	} // Close Cell
	// Close cells (XXX shouldn't pmFPAfileClose iterate down as needed?)
	view->cell = -1;
	while ((cell = pmFPAviewNextCell(view, stack->fpa, 1)) != NULL) {
	  if (!cell->process || !cell->file_exists) {
	    continue;
	  }
	  if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	    psError(psErrorCodeLast(), false, "save failure for Cell");
	    return(false);
	  }
	}

        // Close chip
	if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	  psError(psErrorCodeLast(), false, "save failure for Chip");
	  return(false);
	}
      } // Close chip.
      // Output and Close FPA
      if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psError(psErrorCodeLast(), false, "save failure for FPA");
	return(false);
      }
      psFree(view);
      psFree(data->modelMap);
      psFree(sizeImage);
    }
		
    
    return(true);
}



