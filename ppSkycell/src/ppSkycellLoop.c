#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSkycell.h"

#define BUFFER 16                       // Size of buffer for projections


// Activate/deactivate a single element for a list
void fileActivationSingle(pmConfig *config, // Configuration
                          const char **files, // Files to turn on/off
                          bool state,   // Activation state
                          int num // Number of file in sequence
                          )
{
    assert(config);
    for (int i = 0; files[i] != NULL; i++) {
        pmFPAfileActivateSingle(config->files, state, files[i], num); // Activated file
    }
    return;
}

// Iterate down the hierarchy, loading files; we can get away with this because we're working on skycells
static pmFPAview *filesIterateDown(pmConfig *config // Configuration
                                   )
{
    assert(config);

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }
    view->chip = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }
    view->cell = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }
    view->readout = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }
    return view;
}

// Iterate up the hierarchy, writing files; we can get away with this because we're working on skycells
static bool filesIterateUp(pmConfig *config // Configuration
                           )
{
    assert(config);

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    view->chip = view->cell = view->readout = 0;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        return false;
    }
    view->readout = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        return false;
    }
    view->cell = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        return false;
    }
    view->chip = -1;
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        return false;
    }
    psFree(view);
    return true;
}


bool ppSkycellLoop(ppSkycellData *data // Run-time data
    )
{
    psVector *crval1 = psVectorAllocEmpty(BUFFER, PS_TYPE_F64); // CRVAL1 values
    psVector *crval2 = psVectorAllocEmpty(BUFFER, PS_TYPE_F64); // CRVAL2 values
    psVector *cdelt1 = psVectorAllocEmpty(BUFFER, PS_TYPE_F64); // CDELT1 values
    psVector *cdelt2 = psVectorAllocEmpty(BUFFER, PS_TYPE_F64); // CDELT2 values
    psArray *projRegions = psArrayAllocEmpty(BUFFER); // Region for projection
    int numProj = 0;                    // Number of projections

    psVector *target = psVectorAlloc(data->numInputs, PS_TYPE_S32); // Target for each input
    psArray *imageRegions = psArrayAlloc(data->numInputs); // Region for image
    psArray *regionHDUs = psArrayAlloc(data->numInputs);

    psVector *exptimes = psVectorAlloc(data->numInputs, PS_TYPE_F64);
    
    // Determine which projection cells we have to deal with.
    for (int i = 0; i < data->numInputs; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.IMAGE", i); // File to examine
        // Header in the FPA should have been read as a part of defining the file...
        pmHDU *hdu = file->fpa->hdu;    // Header of interest

        int numCols = psMetadataLookupS32(NULL, hdu->header, "NAXIS1"); // Number of columns
        int numRows = psMetadataLookupS32(NULL, hdu->header, "NAXIS2"); // Number of rows

        pmAstromWCS *wcs = pmAstromWCSfromHeader(hdu->header); // World Coordinate System
        if (!wcs) {
	  if (data->wcsrefName) {
	    pmFPAfile *wcsref = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.WCSREF", i);
	    wcs = pmAstromWCSfromHeader(wcsref->fpa->hdu->header);
	  }
	  if (!wcs) {	  
            psError(psErrorCodeLast(), false, "Unable to read WCS for image %d", i);
            return false;
	  }
        }

	psRegion *region = psRegionAlloc(-1.0 * wcs->crpix1,numCols - wcs->crpix1,
					 -1.0 * wcs->crpix2,numRows - wcs->crpix2);
	imageRegions->data[i] = region;
        psTrace("ppSkycell", 5, "Image region %d is: [%.0f:%.0f,%.0f:%.0f]\n",
                i, region->x0, region->x1, region->y0, region->y1);

        bool found = false;             // Found a projection?
        for (int j = 0; j < numProj && !found; j++) {
            if (wcs->crval1 == crval1->data.F64[j] && wcs->crval2 == crval2->data.F64[j] &&
                wcs->cdelt1 == cdelt1->data.F64[j] && wcs->cdelt2 == cdelt2->data.F64[j]) {
	      //                regionMinMax(projRegions->data[j], region);
	      psRegion *proj = projRegions->data[j];
	      proj->x0 = PS_MIN(region->x0,proj->x0);
	      proj->x1 = PS_MAX(region->x1,proj->x1);
	      proj->y0 = PS_MIN(region->y0,proj->y0);
	      proj->y1 = PS_MAX(region->y1,proj->y1);
                target->data.S32[i] = j;
                found = true;
                psTrace("ppSkycell", 3, "Image %d uses projection %d\n", i, j);
            }
        }
	
        if (!found) { // Add new projection cell if we didn't find one.
            psVectorAppend(crval1, wcs->crval1);
            psVectorAppend(crval2, wcs->crval2);
            psVectorAppend(cdelt1, wcs->cdelt1);
            psVectorAppend(cdelt2, wcs->cdelt2);

            psRegion *projRegion = psRegionAlloc(region->x0, region->x1, region->y0, region->y1);
            psArrayAdd(projRegions, projRegions->n, projRegion);
            psFree(projRegion);
            target->data.S32[i] = numProj;
            psTrace("ppSkycell", 3, "Image %d uses new projection\n", i);
	    psArrayAdd(regionHDUs,1,hdu);
            numProj++;

        }

	// Save exptime
	exptimes->data.F64[i] = psMetadataLookupF64(NULL, hdu->header, "EXPTIME");
    }

    float exptime_target = 1.0;

    //    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);
    //    psVectorStats(stats,exptimes,NULL, NULL, 0);
    //    exptime_target = stats->robustMedian;
    
    pmFPAfileActivate(data->config->files, false, NULL);
    // Loop over projections
    for (int i = 0; i < numProj; i++) {
        psRegion *projRegion = projRegions->data[i]; // Region for skycell projection
        psTrace("ppSkycell", 2, "Projection %d: [%.0f:%.0f,%.0f:%.0f]\n",
                i, projRegion->x0, projRegion->x1, projRegion->y0, projRegion->y1);

	// Size of unbinned image
        int xSize = projRegion->x1 - projRegion->x0 + 1; 
        int ySize = projRegion->y1 - projRegion->y0 + 1; 

        // Size of binned images
        int numCols1 = xSize / (float)data->bin1 + 1.5, numRows1 = ySize / (float)data->bin1 + 1.5;
        int numCols2 = numCols1 / (float)data->bin2 + 1.5, numRows2 = numRows1 / (float)data->bin2 + 1.5;

	// Binned image containers
        psImage *image1 = psImageAlloc(numCols1, numRows1, PS_TYPE_F32); 
        psImage *image2 = psImageAlloc(numCols2, numRows2, PS_TYPE_F32); 
        psImageInit(image1,NAN);
        psImageInit(image2,NAN);

	// Binned image radius values.  Used to determine primacy.
	psImage *radius1 = psImageAlloc(numCols1, numRows1, PS_TYPE_F32);
	psImage *radius2 = psImageAlloc(numCols2, numRows2, PS_TYPE_F32);
	psImageInit(radius1,pow(numCols1 + numRows1,2)); // These values can be anything, just need to be larger than a binned radius.
	psImageInit(radius2,pow(numCols1 + numRows1,2));
	
	// HDU containing the WCS we plan on using
	pmHDU *projhdu = NULL;

	// Do we need to modify the WCS?  This flips to zero after we've done it once.
	int modify_wcs1 = 1;
	int modify_wcs2 = 1;

	// Because we may have holes, we need to ensure that we set the CRPIX in the binned image correction.
	// Find the minimum/maximum, so we know where the zero is.
	float maxCRPIX1   = -99e99;
	float maxCRPIX2   = -99e99;
	// Loop over inputs to this projection.
        for (int j = 0; j < data->numInputs; j++) {
            if (target->data.S32[j] != i) {
                continue;
            }
            pmFPAfileActivateSingle(data->config->files, true, "PPSKYCELL.IMAGE", j);

            pmFPAview *view = filesIterateDown(data->config); // View to readout
            if (!view) {
                psError(psErrorCodeLast(), false, "Unable to iterate down.");
                // XXX Cleanup
                return false;
            }

	    // Read the HDU/WCS information from the first entry, and use that as the reference.
            pmFPAfile *file = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.IMAGE", j);
	    pmAstromWCS *wcs = pmAstromWCSfromHeader(file->fpa->hdu->header); // World Coordinate System
	    if (!wcs) {
	      if (data->wcsrefName) {
		pmFPAfile *wcsref = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.WCSREF", j);
		wcs = pmAstromWCSfromHeader(wcsref->fpa->hdu->header);
		if (!projhdu) {
		  projhdu = wcsref->fpa->hdu;
		}
	      }
	      if (!wcs) {	  
		psError(psErrorCodeLast(), false, "Unable to read WCS for image %d", j);
		return false;
	      }
	    }
	    if (!projhdu) {
	      projhdu = file->fpa->hdu;
	    }

	    // However, we need to check to see if we've found the maximum CRPIX
	    if (wcs->crpix1 > maxCRPIX1) {
	      maxCRPIX1 = wcs->crpix1;
	    }
	    if (wcs->crpix2 > maxCRPIX2) {
	      maxCRPIX2 = wcs->crpix2;
	    }

	    // Actuall do the binning.
            pmReadout *inRO = pmFPAviewThisReadout(view, file->fpa); // Readout with input
            psFree(view);

            pmReadout *bin1RO = pmReadoutAlloc(NULL), *bin2RO = pmReadoutAlloc(NULL); // Binned readouts
            if (!pmReadoutRebin(bin1RO, inRO, data->maskVal, data->bin1, data->bin1)) {
                psError(psErrorCodeLast(), false, "Unable to rebin image");
                // XXX Cleanup
                return false;
            }
            if (!pmReadoutRebin(bin2RO, bin1RO, data->maskVal, data->bin2, data->bin2)) {
                psError(psErrorCodeLast(), false, "Unable to rebin image");
                // XXX Cleanup
                return false;
            }

	    // Scale by exposure time
	    double exptime_factor = pow(exptimes->data.F64[j]/exptime_target,data->exptimeOrder);
	    if ((isfinite(exptime_factor))&&(exptime_factor != 0.0)) {
	      psBinaryOp(bin1RO->image,bin1RO->image,"/",
			 psScalarAlloc(exptime_factor,  PS_TYPE_F32));
	      psBinaryOp(bin2RO->image,bin2RO->image,"/",
			 psScalarAlloc(exptime_factor,  PS_TYPE_F32));
	    }
	    // End scaling

            // Offsets for image on this projection cell are just differences in CRPIX positions.
	    int xOffset1 = (-1 * wcs->crpix1 - projRegion->x0) / (float)(data->bin1);
	    int yOffset1 = (-1 * wcs->crpix2 - projRegion->y0) / (float)(data->bin1);
	    
            int xOffset2 = xOffset1 / (float)data->bin2;
	    int yOffset2 = yOffset1 / (float)data->bin2;
	    psTrace("ppSkycell",5,"Offsets: %d %d : %d %d",
		    xOffset1,yOffset1,xOffset2,yOffset2);

	    // Check each pixel for primacy.  A pixel is primary
	    // if it is closer to the central pixel of its skycell
	    // than any other pixel is to theirs.

	    // Let's just do the overlay here, instead of doing math, then handing it
	    // off to another function.  That seems silly.

	    int u,v,x,y;

	    double u0 = xOffset1 + (bin1RO->image->numCols) / 2.0;
	    double v0 = yOffset1 + (bin1RO->image->numRows) / 2.0;
	    for (x = 0; x < bin1RO->image->numCols; x++) {
	      for (y = 0; y < bin1RO->image->numRows; y++) {
		if (!isfinite(bin1RO->image->data.F32[y][x])) {
		  continue;
		}
		u = x + xOffset1;
		v = y + yOffset1;
		double R2 = pow(u - u0,2) + pow(v - v0,2);
		
		if (R2 < radius1->data.F32[v][u]) {
		  radius1->data.F32[v][u] = R2;
		  image1->data.F32[v][u] = bin1RO->image->data.F32[y][x];
		}
	      }
	    }

	    u0 = xOffset2 + (bin2RO->image->numCols) / 2.0;
	    v0 = yOffset2 + (bin2RO->image->numRows) / 2.0;
	    for (x = 0; x < bin2RO->image->numCols; x++) {
	      for (y = 0; y < bin2RO->image->numRows; y++) {
		if (!isfinite(bin2RO->image->data.F32[y][x])) {
		  continue;
		}
		u = x + xOffset2;
		v = y + yOffset2;
		double R2 = pow(u - u0,2) + pow(v - v0,2);
		if (R2 < radius2->data.F32[v][u]) {
		  radius2->data.F32[v][u] = R2;
		  image2->data.F32[v][u] = bin2RO->image->data.F32[y][x];
		}
	      }
	    }
	    
		
		
	    
	    // Overlay the data onto the appropriate pixels in the final outputs
            // XXX Completely neglecting rotations
            // The skycells are divided up neatly with them all having the same orientation
		//	    psImageOverlaySection(image1, bin1RO->image, xOffset1, yOffset1, "E");
	    //	    psImageOverlaySection(image2, bin2RO->image, xOffset2, yOffset2, "E");

	    // Cleanup on input loop.
            psFree(bin1RO);
            psFree(bin2RO);
            filesIterateUp(data->config);
            psFree(file->fpa);
            file->fpa = NULL;
            pmFPAfileActivate(data->config->files, false, NULL);
        }

        pmFPAfileActivate(data->config->files, true, "PPSKYCELL.JPEG1");
        pmFPAfileActivate(data->config->files, true, "PPSKYCELL.JPEG2");
	if (data->doFits) {
	  pmFPAfileActivate(data->config->files, true, "PPSKYCELL.BIN1");
	  pmFPAfileActivate(data->config->files, true, "PPSKYCELL.BIN2");
	}
	
        pmFPAview *view = filesIterateDown(data->config); // View to readout

	
        pmCell *cell1 = pmFPAfileThisCell(data->config->files, view, "PPSKYCELL.JPEG1"); // Rebinned cell 1
        pmCell *cell2 = pmFPAfileThisCell(data->config->files, view, "PPSKYCELL.JPEG2"); // Rebinned cell 2

        pmReadout *ro1 = pmReadoutAlloc(cell1), *ro2 = pmReadoutAlloc(cell2); // Binned readouts

        ro1->image = image1;
        ro2->image = image2;

        ro1->data_exists = cell1->data_exists = cell1->parent->data_exists = true;
        ro2->data_exists = cell2->data_exists = cell2->parent->data_exists = true;
	
        pmFPAfile *file1 = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.JPEG1", 0);
        file1->save = true;
        file1->fileIndex = i;
        pmFPAfile *file2 = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.JPEG2", 0);
        file2->save = true;
        file2->fileIndex = i;

	if (data->doFits) {

	  pmFPAfile *fits1 = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.BIN1", 0);
	  // Copy header from projection root hdu
	  fits1->fpa->hdu = pmHDUAlloc(NULL);
	  fits1->fpa->hdu->header = psMetadataAlloc();
	  psMetadataCopy(fits1->fpa->hdu->header,projhdu->header);
	  
	  // Change wcs here
	  if (modify_wcs1) {
	    pmAstromWCS *WCS = pmAstromWCSfromHeader(fits1->fpa->hdu->header);
	    double cd1f = 1.0 * data->bin1;
	    double cd2f = 1.0 * data->bin1;

	    WCS->cdelt1 *= cd1f;
	    WCS->cdelt2 *= cd2f;
	    // Fudge the CRPIX incase we have missing corners
	    if (maxCRPIX1 > WCS->crpix1) {
	      WCS->crpix1 = maxCRPIX1 / cd1f;
	    }
	    else {
	      WCS->crpix1 = WCS->crpix1 / cd1f;
	    }
	    if (maxCRPIX2 > WCS->crpix2) {
	      WCS->crpix2 = maxCRPIX2 / cd2f;
	    }
	    else {
	      WCS->crpix2 = WCS->crpix2 / cd2f;
	    }

	    for (int q = 0; q <= WCS->trans->x->nX; q++) {
	      for (int r = 0; r <= WCS->trans->x->nY; r++) {
		WCS->trans->x->coeff[q][r] *= pow(cd1f,q) * pow(cd2f,r);
	      }
	    }
	    for (int q = 0; q <= WCS->trans->y->nX; q++) {
	      for (int r = 0; r <= WCS->trans->y->nY; r++) {
		WCS->trans->y->coeff[q][r] *= pow(cd1f,q) * pow(cd2f,r);
	      }
	    }
	    pmAstromWCStoHeader (fits1->fpa->hdu->header,WCS);
	    modify_wcs1 = 0;
	  }

	  
	  pmChip *Fchip1 = pmFPAfileThisChip(data->config->files, view, "PPSKYCELL.BIN1");
	  psMetadataAddS32(Fchip1->concepts,PS_LIST_TAIL,"CHIP.XPARITY", PS_META_REPLACE,"",1);
	  psMetadataAddS32(Fchip1->concepts,PS_LIST_TAIL,"CHIP.YPARITY", PS_META_REPLACE,"",1);

	  pmCell *Fcell1 = pmFPAfileThisCell(data->config->files, view, "PPSKYCELL.BIN1"); // Rebinned cell 1
 	  // This is a hack to get a functioning header created so the fits images can be written out. 
	  psMetadataAddS32(Fcell1->concepts,PS_LIST_TAIL,"CELL.XPARITY", PS_META_REPLACE,"",1);
	  psMetadataAddS32(Fcell1->concepts,PS_LIST_TAIL,"CELL.YPARITY", PS_META_REPLACE,"",1);
	  psMetadataAddS32(Fcell1->concepts,PS_LIST_TAIL,"CELL.READDIR", PS_META_REPLACE,"",1);

	  // I am baffled that this is the way to get the exposure time updated correctly.
	  psMetadataItem *item1;
	  item1 = psMetadataLookup(Fcell1->concepts,"CELL.EXPOSURE");
	  item1->data.F32 = exptime_target;

	  pmReadout *Fro1 = pmReadoutAlloc(Fcell1);
	  Fro1->image = image1;
	  Fro1->data_exists = Fcell1->data_exists = Fcell1->parent->data_exists = true;
	  
	  fits1->save = true;
	  fits1->fileIndex = i;

	  // Repeat with second binned image
	  pmFPAfile *fits2 = pmFPAfileSelectSingle(data->config->files, "PPSKYCELL.BIN2", 0);
	  fits2->fpa->hdu = pmHDUAlloc(NULL);
	  fits2->fpa->hdu->header = psMetadataAlloc();
	  psMetadataCopy(fits2->fpa->hdu->header,projhdu->header);
	  
	  if (modify_wcs2) {
	    pmAstromWCS *WCS = pmAstromWCSfromHeader(fits2->fpa->hdu->header);
	    double cd1f = 1.0 * data->bin2 * data->bin1;
	    double cd2f = 1.0 * data->bin2 * data->bin1;
	    
	    WCS->cdelt1 *= cd1f;
	    WCS->cdelt2 *= cd2f;
	    WCS->crpix1 = WCS->crpix1 / cd1f;
	    WCS->crpix2 = WCS->crpix2 / cd2f;
	    
	    for (int q = 0; q <= WCS->trans->x->nX; q++) {
	      for (int r = 0; r <= WCS->trans->x->nY; r++) {
		WCS->trans->x->coeff[q][r] *= pow(cd1f,q) * pow(cd2f,r);
	      }
	    }
	    for (int q = 0; q <= WCS->trans->y->nX; q++) {
	      for (int r = 0; r <= WCS->trans->y->nY; r++) {
		WCS->trans->y->coeff[q][r] *= pow(cd1f,q) * pow(cd2f,r);
	      }
	    }
	    pmAstromWCStoHeader (fits2->fpa->hdu->header,WCS);
	    modify_wcs2 = 0;
	  }
	  
	  pmChip *Fchip2 = pmFPAfileThisChip(data->config->files, view, "PPSKYCELL.BIN2");
	  psMetadataAddS32(Fchip2->concepts,PS_LIST_TAIL,"CHIP.XPARITY", PS_META_REPLACE,"",1);
	  psMetadataAddS32(Fchip2->concepts,PS_LIST_TAIL,"CHIP.YPARITY", PS_META_REPLACE,"",1);

	  
	  pmCell *Fcell2 = pmFPAfileThisCell(data->config->files, view, "PPSKYCELL.BIN2"); // Rebinned cell 2
	  psMetadataAddS32(Fcell2->concepts,PS_LIST_TAIL,"CELL.XPARITY", PS_META_REPLACE,"",1);
	  psMetadataAddS32(Fcell2->concepts,PS_LIST_TAIL,"CELL.YPARITY", PS_META_REPLACE,"",1);
	  psMetadataAddS32(Fcell2->concepts,PS_LIST_TAIL,"CELL.READDIR", PS_META_REPLACE,"",1);

	  psMetadataItem *item2 = psMetadataLookup(Fcell2->concepts,"CELL.EXPOSURE");
	  item2->data.F32 = exptime_target;
	  
	  pmReadout *Fro2 = pmReadoutAlloc(Fcell2); 
	  Fro2->image = image2;
	  Fro2->data_exists = Fcell2->data_exists = Fcell2->parent->data_exists = true;

	  fits2->save = true;
	  fits2->fileIndex = i;

	  
	}

        psFree(view);

        filesIterateUp(data->config);
    }

    // XXX Cleanup
    return true;
}
