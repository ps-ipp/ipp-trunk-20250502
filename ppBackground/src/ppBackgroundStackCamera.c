#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppBackgroundStack.h"

/// Add a single filename to the arguments as an array, so that it can be used with pmFPAfileBindFromArgs, etc
static void fileArguments(const char *file, // The symbolic name for the file
                          const char *name, // The name of the file
                          const char *comment, // Description of the file
                          pmConfig *config // Configuration
    )
{
    psArray *files = psArrayAlloc(1); // Array with file names
    files->data[0] = psStringCopy(name);
    if (psMetadataLookup(config->arguments, file)) {
        psMetadataRemoveKey(config->arguments, file);
    }
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, file, 0, comment, files);
    psFree(files);
    return;
}


bool ppBackgroundStackCamera(ppBackgroundStackData *data // Run-time data
    )
{
    bool status;                        // Status of file definition
    pmConfig *config = data->config;    // Because I'm reusing code.
    int u,v;
    //    size_t A,P;
    double RR = -9999.0 ,DD = -99999.0,rr = 9999.0,dd = 99999.0;
    // FIX Figure out what stacks we have to deal with.
    psString stackName = data->stacks->data[0];
    fileArguments("IMAGE", stackName, "Input image", data->config);
    psFree(stackName);
    pmFPAfile *stack = pmFPAfileDefineFromArgs(&status, data->config,
					       "PPBACKGROUND.STACK", "IMAGE");
    if (!status || !stack) {
      psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.STACK");
      return false;
    }

    if (!pmAstromReadBilevelMosaic(stack->fpa,stack->fpa->hdu->header)) {
      psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for input stack.");
      return false;
    }
    psArrayAdd(data->stack_data,data->stack_data->n,psMemIncrRefCounter(stack));
    pmFPAfileActivate(config->files, false, NULL);

    // You know what? Let's just fucking lie to config->files.
    psMetadata *config_files = config->files;
    config->files = NULL;
    config->files = psMetadataAlloc();
    
    // Read over the input background models.    
    psMetadataIterator *iter = psMetadataIteratorAlloc(data->contents, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item; // Item from iteration
    int i = 0;
    while ((item = psMetadataGetAndIncrement(iter))) {
      i++;
      if (item->type != PS_DATA_METADATA) {
	psError(PPBACKGROUND_ERR_ARGUMENTS, true,
		"Component %s of the input metadata is not of type METADATA", item->name);
	psFree(iter);
	return(false);
      }
      
      // Pull out the information for this exposure
      psMetadata *input = item->data.md; // the input metadata of interest
      psString smfFileName = psMetadataLookupStr(NULL, input, "astrom");
      psMetadata *modelContent = psMetadataLookupMetadata(NULL, input, "models");

      // Allocate the model metadata object
      psMetadata *Bmodel = psMetadataAlloc();
      
      // Read the smf file from this item
      fileArguments("astrom",smfFileName,"",config);
      pmFPAfile *smfFile = pmFPAfileDefineFromArgs(&status, config, "PSWARP.ASTROM","astrom");
      
      // taking from pswarpLoadAstrometry.c
      smfFile->type = PM_FPA_FILE_WCS;
      // Read the SMF data
      pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy

      if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	return NULL;
      }
      if (!pmFPAfileRead(smfFile,view,config)) {
	psError(PS_ERR_IO, false, "failed READ at FPA %s", smfFile->name);
	psFree(view);
	return false;
      }
      
      printf("CZW: Item %d\n",i);
      // find the FPA phu
      bool bilevelAstrometry = false;
      pmHDU *phu = pmFPAviewThisPHU(view, smfFile->fpa);
      if (phu) {
	char *ctype = psMetadataLookupStr(NULL, phu->header, "CTYPE1");
	if (ctype) {
	  bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
	}
      }
      if (bilevelAstrometry) {
	if (!pmAstromReadBilevelMosaic(smfFile->fpa, phu->header)) {
	  psError(psErrorCodeLast(), false, "Unable to read bilevel mosaic astrometry for input FPA.");
	  psFree(view);
	  return false;
	}
      }
      
      psF32 exptime = 1.0;
      exptime = psMetadataLookupF32(NULL, phu->header, "EXPTIME");

      pmChip *chip;                       // Chip from FPA
      while ((chip = pmFPAviewNextChip(view, smfFile->fpa, 1))) {
	if (!chip->process || !chip->file_exists) {
	  continue;
	}
	const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip
	if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	  psError(psErrorCodeLast(), false, "Error loading data from files.");
	  return false;
	}
	if (chip->cells->n != 1) {
	  psWarning("More than one cell present for chip %d", view->chip);
	}

/* 	psMemStats(0,&A,&P); */
/* 	fprintf(stderr,"chip %ld %ld\n",A,P); */

	// read WCS data from the corresponding header
	pmHDU *hdu = pmFPAviewThisHDU (view, smfFile->fpa);
	if (bilevelAstrometry) {
	  if (!pmAstromReadBilevelChip (chip, hdu->header)) {
	    psWarning("Unable to read bilevel chip astrometry for chip %s.", chipName);
	    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	      psError(psErrorCodeLast(), false, "Error saving data to files.");
	      return false;
	    }
	    continue;
	  }
	} else {
	  // we use a default FPA pixel scale of 1.0
	  psWarning("Reading WCS astrometry for chip %s.", chipName);
	  if (!pmAstromReadWCS(smfFile->fpa, chip, hdu->header, 1.0)) {
	    psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for input FPA.");
	    psFree(view);
	    return false;
	  }
	}

	//  Load model data for this chip
	psString modelFileName = pmConfigConvertFilename(psMetadataLookupStr(NULL, modelContent, chipName),
							 config, PM_FPA_MODE_READ, false);
	psFits *modelFits = psFitsOpen(modelFileName,"r");
	psImage *image = psFitsReadImage(modelFits,psRegionSet(0,0,0,0),0);
	psMetadata *header = psFitsReadHeader(NULL,modelFits);
	
	// Allocate the data structures for this chip
	psImage *raim = psImageAlloc(image->numCols,image->numRows,PS_TYPE_F32);
	psImage *decim = psImageAlloc(image->numCols,image->numRows,PS_TYPE_F32);
	psImage *model = psImageAlloc(image->numCols,image->numRows,PS_TYPE_F32);

	// Read header and construct original positions
	psS32 naxis1 = psMetadataLookupS32(NULL, header, "NAXIS1");
	psS32 naxis2 = psMetadataLookupS32(NULL, header, "NAXIS2");
	psS32 imnaxis1 = psMetadataLookupS32(NULL, header, "IMNAXIS1");
	psS32 imnaxis2 = psMetadataLookupS32(NULL, header, "IMNAXIS2");
	psString ccdsum = psMetadataLookupStr(NULL, header, "CCDSUM");
	psS32 xbin = atoi(strtok(ccdsum," "));
	psS32 ybin = atoi(strtok(NULL, " "));
	psS32 xoffset = (naxis1 * xbin - imnaxis1) / (2 * xbin);
	psS32 yoffset = (naxis2 * ybin - imnaxis2) / (2 * ybin);

	psPlane *pix = psPlaneAlloc();   // Pixel coordinates on chip
	psPlane *fp = psPlaneAlloc();    // Focal plane coordinates
	psPlane *tp = psPlaneAlloc();    // Tangent plane coordinates
	psSphere *sky = psSphereAlloc(); // Sky coordinates
	
	for (v = 0; v < image->numRows; v++) {
	  pix->y = (v - yoffset) * ybin;
	  for (u = 0; u < image->numCols; u++) {
	    pix->x = (u - xoffset) * xbin;
	    psPlaneTransformApply(fp, chip->toFPA, pix);
	    psPlaneTransformApply(tp, smfFile->fpa->toTPA, fp);
	    psDeproject(sky, tp, smfFile->fpa->toSky);

	    psProject(tp,sky,stack->fpa->toSky);

	    raim->data.F32[v][u] = tp->x;
	    decim->data.F32[v][u] = tp->y;
	    model->data.F32[v][u] = 0.0;
	    image->data.F32[v][u] /= exptime;
	    // Check the bounds so we'll know how large of an area to model in the map
	    if (tp->x < data->ra_min) { data->ra_min = tp->x; }
	    else if (tp->x > data->ra_max) { data->ra_max = tp->x; }
	    if (tp->y < data->dec_min) { data->dec_min = tp->y; }
	    else if (tp->y > data->dec_max) { data->dec_max = tp->y; }

	    if (sky->r < rr) { rr = sky->r; }
	    else if (sky->r > RR) { RR = sky->r; }
	    if (sky->r < dd) { dd = sky->d; }
	    else if (sky->d > DD) { DD = sky->d; }
	    
	  }
	}

	// Allocate the model data for this chip
	psMetadata *this_model = psMetadataAlloc();
		
	// Attach vectors to teh structure of the chip
	psMetadataAddImage(this_model,PS_LIST_TAIL,
			   "bkg image", 0,
			   "ota space X vector", image);
	psMetadataAddImage(this_model,PS_LIST_TAIL,
			   "bkg ra", 0,
			   "ota space ra vector", raim);
	psMetadataAddImage(this_model,PS_LIST_TAIL,
			   "bkg dec", 0,
			   "ota space dec vector", decim);
	psMetadataAddImage(this_model,PS_LIST_TAIL,
			   "bkg calibrated data", 0,
			   "ota space corrected data", model);

	// Define default background model parameters, using the assumption:
	// observed = camera + offset + scale * astrophysical
	psMetadataAddF32(this_model,PS_LIST_TAIL,
			 "bkg offset", PS_META_REPLACE,
			 "background offset for this exposure/ota pair", 0.0);
	psMetadataAddF32(this_model,PS_LIST_TAIL,
			 "bkg scale", PS_META_REPLACE,
			 "background scale parameter for this exposure/ota pair", 1.0);
	// Add this model to the current model
	psMetadataAddMetadata(Bmodel,PS_LIST_TAIL,
			      chipName, PS_META_REPLACE,
			      "model data for this exposure/ota", this_model);
	// Free model data for this chip
	psFree(modelFileName);
	psFree(modelFits);
	psFree(header);
	psFree(pix);
	psFree(fp);
	psFree(tp);
	psFree(sky);
	psFree(image);
	psFree(raim);
	psFree(decim);
	psFree(model);
	psFree(this_model);
	
	// Check to see if we've loaded or allocated an OTA solution container for this chip
	if (!psMetadataLookupPtr(NULL,data->OTA_solutions,chipName)) { // No solution metadata entry exists for this chipName
	  // FIX this should try to find the imagefile. 
	  if (data->fit_OTAS) { // We are fitting OTAs, so allocate a new image
	    psImage *solution = psImageAlloc(image->numCols,image->numRows,PS_TYPE_F32);
	    psMetadataAddPtr(data->OTA_solutions,PS_LIST_TAIL,
			     chipName, PS_DATA_UNKNOWN | PS_META_REPLACE,
			     "OTA solution element", solution);
	  }
	  else { // We are not fitting OTAs, so read the one that should be saved on OTApath.
	    psString solutionFileName = psStringCopy(data->OTApath);
	    psStringAppend(&solutionFileName, ".%s.fits",chipName);
	    psString resolvedFileName = pmConfigConvertFilename(solutionFileName,config,false,false);
	    psFits *solutionFits = psFitsOpen(resolvedFileName,"r");
	    psImage *solution = psFitsReadImage(solutionFits,psRegionSet(0,0,0,0),0);
	    psMetadataAddImage(data->OTA_solutions,PS_LIST_TAIL,
			       chipName, 0,
			       "OTA solution element", solution);
	    psFree(solutionFits);
	    psFree(solutionFileName);
	    psFree(solution);
	  }
	}
      } // end chip loop

      // Add the set of models to the datastructure
      psMetadataAddMetadata(data->models, PS_LIST_TAIL,
			    smfFile->origname, PS_META_REPLACE,
			    "model data for this exposure", Bmodel);
      psMetadataAddS16(data->models,PS_LIST_TAIL,
		       "N", PS_META_REPLACE,
		       "counter", i);
      
      if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
	psError(psErrorCodeLast(), false, "Error saving data to files.");
	return false;
      }
      pmFPAfileActivate(config->files, false, NULL);
      psFree(modelContent);
      //       psFree(smfFile);       // This gets freed when we trash the config->files structure.  I think.  
      psFree(smfFileName);
      psFree(view);

      psFree(config->files);
      config->files = psMetadataAlloc();
      // 

      //
    } // end smf loop

    // Unlie now.
    psFree(config->files);
    config->files = config_files;
    
    // remove the {ra|dec}_min values from everything so we don't confuse the binning code.
/*     psMetadataIterator *expIter = psMetadataIteratorAlloc(data->models, PS_LIST_HEAD, NULL); */
/*     psMetadataItem *expItem; */
/*     while ((expItem = psMetadataGetAndIncrement(expIter))) { */
/*       if (expItem->type != PS_DATA_METADATA) { */
/* 	continue; // This is the N counter */
/*       } */
/*       psMetadataIterator *chipIter = psMetadataIteratorAlloc(expItem->data.md, PS_LIST_HEAD, NULL); */
/*       psMetadataItem *chipItem; */
/*       while ((chipItem = psMetadataGetAndIncrement(chipIter))) { */
/* 	psImage *ra    = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg ra"); */
/* 	psImage *dec   = psMetadataLookupPtr(NULL, chipItem->data.md, "bkg dec"); */
/* 	for (v = 0; v < ra->numRows; v++) { */
/* 	  for (u = 0; u < ra->numCols; u++) { */
/* 	    ra->data.F32[v][u] -= data->ra_min; */
/* 	    dec->data.F32[v][u] -= data->dec_min; */
/* 	  } */
/* 	} */
/*       } // End chip */
/*       psFree(chipIter); */
/*     } // End smfs */
/*     psFree(expIter); */
/*     // And from the structure objects. */
/*     data->x_min -= data->ra_min; */
/*     data->y_min -= data->dec_min; */
/*     data->x_max -= data->ra_min; */
/*     data->y_max -= data->dec_min; */
/* /\*     data->ra_max -= data->ra_min; *\/ */
/* /\*     data->dec_max -= data->dec_min; *\/ */
/* /\*     data->ra_min = 0.0; *\/ */
/* /\*     data->dec_min = 0.0; *\/ */

    
    return true;
}
