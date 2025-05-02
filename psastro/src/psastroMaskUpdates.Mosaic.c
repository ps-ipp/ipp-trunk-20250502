# include "psastroInternal.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "I/O failure in psastroMaskUpdate"); \
  psFree (view); \
  return false; \
}
  
pmCell *pmCellInChip (pmChip *chip, float x, float y);
bool pmCellCoordsForChip (float *xCell, float *yCell, pmCell *cell, float xChip, float yChip);
bool psastroMaskCircle (psImage *mask, psImageMaskType value, float x0, float y0, float dX, float dY);
bool psastroMaskBox (psImage *mask, psImageMaskType value, float x0, float y0, float dL, float dW, float theta);
void psastroMaskLine (psImage *mask, psImageMaskType value, double x1, double y1, double x2, double y2, int dW);
void psastroMaskLineBresen (psImage *mask, psImageMaskType value, int X1, int Y1, int X2, int Y2, int dW, int swapcoords);
void psastroMaskRectangle (psImage *mask, psImageMaskType value, int x0, int y0, int x1, int y1);

// create a mask or mask regions based on the collection of reference stars that are 
// in the vicinity of each chip
bool psastroMaskUpdates (pmConfig *config) {

    bool status;
    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    float zeropt, exptime;

    psImageMaskType maskValue  = pmConfigMaskGet("GHOST", config); // Mask value for ghost pixels
    psImageMaskType maskBlank  = pmConfigMaskGet("BLANK", config); // Mask value for blank pixels

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
        return false;
    }

    // this step is optional
    bool REFSTAR_MASK                      = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK");
    if (!REFSTAR_MASK) return true;

    double REFSTAR_MASK_MAX_MAG            = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_MAX_MAG");
    double REFSTAR_MASK_SATSTAR_MAG_SLOPE  = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_MAG_SLOPE");
    double REFSTAR_MASK_SATSTAR_MAG_MAX    = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_MAG_MAX");
    double REFSTAR_MASK_SATSTAR_POS_ZERO   = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSTAR_POS_ZERO");
    double REFSTAR_MASK_SATSPIKE_MAG_SLOPE = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_MAG_SLOPE");
    double REFSTAR_MASK_SATSPIKE_MAG_MAX   = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_MAG_MAX");
    double REFSTAR_MASK_SATSPIKE_WIDTH     = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_SATSPIKE_WIDTH");

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
	return false;
    }
    pmFPA *fpa = input->fpa;

    // really error-out here?  or just skip?
    if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, NULL, fpa, recipe)) {
	psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
	return false;
    }

    // recipe values are given in instrumental magnitudes
    // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
    float MagOffset = zeropt + 2.5*log10(exptime);
    REFSTAR_MASK_MAX_MAG += MagOffset;
    REFSTAR_MASK_SATSTAR_MAG_MAX += MagOffset;
    REFSTAR_MASK_SATSPIKE_MAG_MAX += MagOffset;

    // select the input mask image :: we modify those pixels, the mosaic to chip mosaic format
    pmFPAfile *inMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT.MASK");
    if (!inMask) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find input mask");
	return false;
    }
    pmFPA *fpaMask = inMask->fpa;

    // select the output mask image :: we mosaic to chip mosaic format
    pmFPAfile *outMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.OUTPUT.MASK");
    if (!outMask) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find output mask");
	return false;
    }

    // select the reference mask fpa :: we use this to determine cell boundaries
    pmFPAfile *refMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.REFMASK");
    if (!refMask) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find mask reference");
	return false;
    }

    double POSANGLE = PM_RAD_DEG * psMetadataLookupF64 (&status, fpa->concepts, "FPA.POSANGLE"); 
    psAssert (status, "POSANGLE missing");

    // de-activate all files except PSASTRO.INPUT.MASK and PSASTRO.OUTPUT.MASK
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.INPUT.MASK");
    pmFPAfileActivate (config->files, true, "PSASTRO.OUTPUT.MASK");

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPAview *viewMask = pmFPAviewAlloc (0);

    // open/load files as needed
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
	if (!chip->fromFPA) { continue; }

	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

	char *filename = NULL;
	char *chipname = psMetadataLookupStr (&status, chip->concepts, "CHIP.NAME");
	psStringAppend (&filename, "refstars.mask.%s.dat", chipname);
	FILE *f = fopen (filename, "w");
	if (!f) {
	    psWarning ("cannot create refstar mask file %s\n", filename);
	    continue;
	}
	psFree (filename);

	pmChip *chipMask = pmFPAviewThisChip (view, fpaMask);

	// load sequence for mask corresponding to this chip (XXX this is needed if the input mask is not the same format as the astrometry file
	*viewMask = *view;
        while ((cell = pmFPAviewNextCell (viewMask, fpaMask, 1)) != NULL) {
            psTrace ("psastro", 4, "Mask Cell %d: %x %x\n", viewMask->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
	    if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_BEFORE)) ESCAPE;
	    
            while ((readout = pmFPAviewNextReadout (viewMask, fpaMask, 1)) != NULL) {
		if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_BEFORE)) ESCAPE;
                if (! readout->data_exists) { continue; }
	    }
	}

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            // XXX there can only be one readout per chip in astrometry, right?
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                // select the raw objects for this readout
                psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
                if (refstars == NULL) { continue; }

		// we need to generate the following masks regions:
		// 1) circle around the saturated stars (scaled by magnitude)
		// 2) diffraction spikes in direction ROT - ROTo
		// 3) bleed trail in the direction of the readout

		// XXX for the moment, let's just generate mana region-file objects in chip pixel space
		for (int i = 0; i < refstars->n; i++) {
		    pmAstromObj *ref = refstars->data[i];
		    if (ref->Mag > REFSTAR_MASK_MAX_MAG) continue;
		    
		    // convert x,y chip coordinates to cells in maskChip
		    pmCell *maskCell = pmCellInChip (chipMask, ref->chip->x, ref->chip->y);

		    // XXX convert ref->Mag to instrumental mags

		    // CIRCLE around the stars (scaled by magnitude)
		    float radius = REFSTAR_MASK_SATSTAR_MAG_SLOPE * (REFSTAR_MASK_SATSTAR_MAG_MAX - ref->Mag);
		    fprintf (f, "CIRCLE %f %f  %f %f\n", ref->chip->x, ref->chip->y, radius, radius);

		    if (maskCell) {
			float xCell = 0.0;
			float yCell = 0.0;
			pmCellCoordsForChip (&xCell, &yCell, maskCell, ref->chip->x, ref->chip->y);
			// XXX for now, assume cell binning is 1x1 relative to chip
			if (maskCell->readouts->n) {
			    pmReadout *readout = maskCell->readouts->data[0];
			    psastroMaskCircle (readout->mask, maskValue, xCell, yCell, radius, radius);
			}
		    }

		    // LINE for boundaries of the saturation spikes (scaled by magnitude)
		    float spikeLength = REFSTAR_MASK_SATSPIKE_MAG_SLOPE * (REFSTAR_MASK_SATSPIKE_MAG_MAX - ref->Mag);
		    float spikeWidth = REFSTAR_MASK_SATSPIKE_WIDTH;

		    for (float theta = 0.0; theta < 2*M_PI; theta += M_PI / 2.0) {
			float x0, y0, x1, y1, dx, dy;

			float Theta = POSANGLE - REFSTAR_MASK_SATSTAR_POS_ZERO + theta;

			// lower side 
			x0 = ref->chip->x + spikeWidth*sin(Theta);
			y0 = ref->chip->y - spikeWidth*cos(Theta);
			x1 = ref->chip->x + spikeLength*cos(Theta) + spikeWidth*sin(Theta);
			y1 = ref->chip->y + spikeLength*sin(Theta) - spikeWidth*cos(Theta);
			dx = x1 - x0;
			dy = y1 - y0;
			fprintf (f, "LINE %f %f  %f %f\n", x0, y0, dx, dy);

			// upper side 
			x0 = ref->chip->x - spikeWidth*sin(Theta);
			y0 = ref->chip->y + spikeWidth*cos(Theta);
			x1 = ref->chip->x + spikeLength*cos(Theta) - spikeWidth*sin(Theta);
			y1 = ref->chip->y + spikeLength*sin(Theta) + spikeWidth*cos(Theta);
			dx = x1 - x0;
			dy = y1 - y0;
			fprintf (f, "LINE %f %f  %f %f\n", x0, y0, dx, dy);

			if (maskCell) {
			    float xCell = 0.0;
			    float yCell = 0.0;
			    pmCellCoordsForChip (&xCell, &yCell, maskCell, ref->chip->x, ref->chip->y);
			    int xParityCell = psMetadataLookupS32(NULL, maskCell->concepts, "CELL.XPARITY");
			    int yParityCell = psMetadataLookupS32(NULL, maskCell->concepts, "CELL.YPARITY");
			    // XXX for now, assume cell binning is 1x1 relative to chip
			    // XXX for now, assume only a single readout
			    if (maskCell->readouts->n) {
				pmReadout *readout = maskCell->readouts->data[0];
				psastroMaskBox (readout->mask, maskValue, xCell, yCell, spikeLength, spikeWidth, Theta*xParityCell*yParityCell);
			    }
			}
		    }

		    // LINE for boundaries of the bleed lines
		    fprintf (f, "LINE %f %f  %f %f\n", ref->chip->x, ref->chip->y, 0.0, -100.0);
		    if (maskCell) {
			float xCell = 0.0;
			float yCell = 0.0;
			pmCellCoordsForChip (&xCell, &yCell, maskCell, ref->chip->x, ref->chip->y);
			// XXX for now, assume cell binning is 1x1 relative to chip
			if (maskCell->readouts->n) {
			    pmReadout *readout = maskCell->readouts->data[0];
			    psastroMaskRectangle (readout->mask, maskValue, (int) xCell-3, (int) yCell, (int) xCell+4, readout->mask->numRows);
			    // XXX this can be done more easily knowing the we mask to the end in y only
			}
		    }
		}
            }
        }

	pmChip *outChip = pmFPAviewThisChip(view, outMask->fpa);
	if (!outChip->hdu && !outChip->parent->hdu) {
	    const char *name = psMetadataLookupStr(&status, inMask->fpa->concepts, "FPA.OBS"); // Name of FPA
	    pmFPAAddSourceFromView(outMask->fpa, name, view, outMask->format);
	}

	psTrace("psastro", 5, "mosaic chip %s to %s (xbin,ybin: %d,%d to %d,%d)\n",
		inMask->name, outMask->name, inMask->xBin, inMask->yBin, outMask->xBin, outMask->yBin);

	// Mosaic the chip, making a deep copy.  This has the side effect of making the output
	// image products pure trimmed images, but also increases the memory footprint.
	status = pmChipMosaic(outChip, chipMask, true, maskBlank);

	// output sequence for mask corresponding to this chip
	*viewMask = *view;
        while ((cell = pmFPAviewNextCell (viewMask, outMask->fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Mask Cell %d: %x %x\n", viewMask->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
	    
            while ((readout = pmFPAviewNextReadout (viewMask, outMask->fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }
		if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_AFTER)) ESCAPE;
	    }
	    if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_AFTER)) ESCAPE;
	}

	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
	fclose (f);
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

    // deactivate all files
    pmFPAfileActivate (config->files, false, NULL);

    psFree (view);
    psFree (viewMask);
    return true;
}

// XXX this is going to be very slow...
pmCell *pmCellInChip (pmChip *chip, float x, float y) {

# if (0)
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
# endif

    for (int i = 0; i < chip->cells->n; i++) {

	pmCell *cell = chip->cells->data[i];
	psRegion *region = pmCellExtent (cell);

	if (x < region->x0) goto skip;
	if (x > region->x1) goto skip;
	if (y < region->y0) goto skip;
	if (y > region->y1) goto skip;

	psFree (region);
	return cell;

    skip:
	psFree (region);
    }
    return NULL;
}

// convert chip coords to cell coords, given known cell
bool pmCellCoordsForChip (float *xCell, float *yCell, pmCell *cell, float xChip, float yChip) {

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
    // ((pos)*(binning)*(cellParity) + (cell0))
    // XXX this is probably totally wrong now....
    // ((pos) - (cell0))*(cellParity)/(binning))
    *xCell = (xChip - x0Cell)*xParityCell/xBin;
    *yCell = (yChip - y0Cell)*yParityCell/yBin;

    return true;
}

// XXX should be doing an OR
bool psastroMaskCircle (psImage *mask, psImageMaskType value, float x0, float y0, float dX, float dY) {

    // XXX need to worry about row0, col0
    for (int ix = -dX; ix <= +dX; ix++) {
	int jx = ix + x0;
	if (jx < 0) continue;
	if (jx >= mask->numCols) continue;
	for (int iy = -dY; iy <= +dY; iy++) {
	    int jy = iy + y0;
	    if (jy < 0) continue;
	    if (jy >= mask->numRows) continue;

	    double r2 = PS_SQR(ix/dX) + PS_SQR(iy/dY);
	    if (r2 > 1.0) continue;
	    
	    mask->data.PS_TYPE_IMAGE_MASK_DATA[jy][jx] |= value;
	}
    }
    return true;
}

// XXX should be doing an OR
bool psastroMaskBox (psImage *mask, psImageMaskType value, float x0, float y0, float dL, float dW, float theta) {

    // draw a series of lines (from -0.5*dW to +0.5*dW) of length dL, starting at x0, y0, angle theta

    float xs = x0;
    float ys = y0;

    float xe = xs + dL*cos(theta);
    float ye = ys + dL*sin(theta);

    psastroMaskLine (mask, value, xs, ys, xe, ye, (int) dW);
    return true;
}

// identify the quadrant and draw the correct line
void psastroMaskLine (psImage *mask, psImageMaskType value, double x1, double y1, double x2, double y2, int dW) {

  int FlipDirect, FlipCoords;
  int X1, Y1, X2, Y2, dX, dY;

  /* rather than draw the line from float positions, we find the closest
     integer end-points and draw the line between those pixels */ 

  X1 = ROUND(x1);
  Y1 = ROUND(y1);
  X2 = ROUND(x2);
  Y2 = ROUND(y2);

  dX = X2 - X1;
  dY = Y2 - Y1;

  FlipCoords = (abs(dX) < abs(dY));
  FlipDirect = FlipCoords ? (y1 > y2) : (x1 > x2);

  if (!FlipDirect && !FlipCoords) psastroMaskLineBresen (mask, value, X1, Y1, X2, Y2, dW, FALSE);
  if ( FlipDirect && !FlipCoords) psastroMaskLineBresen (mask, value, X2, Y2, X1, Y1, dW, FALSE);
  if (!FlipDirect &&  FlipCoords) psastroMaskLineBresen (mask, value, Y1, X1, Y2, X2, dW, TRUE);
  if ( FlipDirect &&  FlipCoords) psastroMaskLineBresen (mask, value, Y2, X2, Y1, X1, dW, TRUE);

  return;
}

// use the Bresenham line drawing technique
// integer-only Bresenham line-draw version which is fast
void psastroMaskLineBresen (psImage *mask, psImageMaskType value, int X1, int Y1, int X2, int Y2, int dW, int swapcoords) {

    int X, Y, dX, dY;
    int e, e2;

    dX = X2 - X1;
    dY = Y2 - Y1;

    Y = Y1;
    e = 0;
    for (X = X1; X <= X2; X++) {
	if (X > 0) {
	    if (swapcoords) {
		if (X >= mask->numRows) continue;
		for (int y = Y - dW; y <= Y + dW; y++) {
		    if (y < 0) continue;
		    if (y >= mask->numCols) continue;
		    mask->data.PS_TYPE_IMAGE_MASK_DATA[X][y] |= value;
		}
	    } else {
		if (X >= mask->numCols) continue;
		for (int y = Y - dW; y <= Y + dW; y++) {
		    if (y < 0) continue;
		    if (y >= mask->numRows) continue;
		    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][X] |= value;
		}
	    }
	}
	e += dY;
	e2 = 2 * e;
	if (e2 > dX) {
	    Y++;
	    e -= dX;
	} 
	if (e2 < -dX) {
	    Y--;
	    e += dX;
	}
    }
    return;
}

void psastroMaskRectangle (psImage *mask, psImageMaskType value, int x0, int y0, int x1, int y1) {
    for (int iy = PS_MAX(0,y0); iy < PS_MIN(y1,mask->numRows); iy++) {
	for (int ix = PS_MAX(0,x0); ix < PS_MIN(x1,mask->numCols); ix++) {
	    mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= value;
	}
    }
}

