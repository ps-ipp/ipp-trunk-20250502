#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppCoord.h"

typedef struct {
  psPlane pix;
  psPlane fp;
  psPlane tp;
  psSphere sky;
} ppCoordSet;

// Convert sky coordinates to chip coordinates
static void coordSky2Chip(ppCoordSet *src, // Sky coordinates
                          bool radians,          // Coordinates are in radians?
                          const pmFPA *astromFPA,  // Astrometry FPA
                          const pmChip *astromChip // Astrometry chip
    )
{
    // convert from src->sky to src->pix
    if (!radians) {
        src->sky.r *= M_PI / 180.0;
        src->sky.d *= M_PI / 180.0;
    }

    psProject(&src->tp, &src->sky, astromFPA->toSky);
    psTrace("ppCoord",2,"sky2pix tr1: %f %f\n", src->tp.x, src->tp.y);
    psPlaneTransformApply(&src->fp, astromFPA->fromTPA, &src->tp);
    psTrace("ppCoord",2,"sky2pix tr2: %f %f\n", src->fp.x, src->fp.y);
    psPlaneTransformApply(&src->pix, astromChip->fromFPA, &src->fp);
    psTrace("ppCoord",2,"sky2pix tr3: %f %f\n", src->pix.x, src->pix.y);

    return;
}

// Convert chip coordinates to cell coordinates
static void coordChip2Cell(psString *cellName,       // Cell name (output)
                           float *xCell, float *yCell, // Pixel coordinates (output)
                           float xChip, float yChip, // Sky coordinates
                           const psArray *cellNames, // Names of cells
                           const psArray *cellBounds,                                // Bounds of cells
                           const psVector *cellX0, const psVector *cellY0, // Cell offsets
                           const psVector *cellParityX, const psVector *cellParityY, // Cell parities
                           const psVector *cellBinX, const psVector *cellBinY        // Cell binnings


    )
{
    int numCells = cellNames->n; // Number of cells
    for (int i = 0; i < numCells && !*cellName; i++) {
        psRegion *region = cellBounds->data[i]; // Bounds of cell
        if (xChip >= region->x0 && xChip < region->x1 && yChip >= region->y0 && yChip < region->y1) {
            *cellName = psStringCopy(cellNames->data[i]);
            // Transform chip coordinates to cell coordinates
            *xCell = (xChip - cellX0->data.S32[i]) / (float)(cellParityX->data.S32[i] * cellBinX->data.S32[i]);
            *yCell = (yChip - cellY0->data.S32[i]) / (float)(cellParityY->data.S32[i] * cellBinY->data.S32[i]);
        }
    }

    return;
}

bool ppCoordLoop(ppCoordData *data // Run-time data
    )
{
    pmConfig *config = data->config;                                        // Configuration data
    pmFPAfile *astromFile = pmFPAfileSelectSingle(config->files, "PPCOORD.ASTROM", 0); // File with astrometry

    if (astromFile->fpa->chips->n > 0 && data->pixelsName && !data->chipName) {
        psWarning("Pixel coordinates supplied, but no chip name provided.");
    }

    pmFPAfile *rawFile = data->rawName ? pmFPAfileSelectSingle(config->files, "PPCOORD.RAW", 0) :
        NULL; // File with raw image

    psArray *pixels = NULL, *radec = NULL, *streaks = NULL, *clusters = NULL; // Array of coordinate vectors
    psArray *radecOut = NULL, *streaksOut = NULL, *clustersOut = NULL;        // Output for sky coordinates
    if (data->pixelsName) {
        pixels = psVectorsReadFromFile(data->pixelsName, "%f %f");
        if (!pixels || pixels->n != 2) {
            psError(psErrorCodeLast(), false, "Unable to read pixel coordinates");
            return false;
        }
    }
    if (data->radecName) {
        radec = psVectorsReadFromFile(data->radecName, "%lf %lf");
        if (!radec || radec->n != 2) {
            psError(psErrorCodeLast(), false, "Unable to read sky coordinates");
            return false;
        }
        psVector *ra = radec->data[0];  // RA coordinates
        long num = ra->n;               // Number of coordinates
        radecOut = psArrayAlloc(8);
        radecOut->data[0] = psArrayAlloc(num);
        radecOut->data[1] = psVectorAlloc(num, PS_TYPE_F32);
        radecOut->data[2] = psVectorAlloc(num, PS_TYPE_F32);
        radecOut->data[3] = psArrayAlloc(num);
        radecOut->data[4] = psVectorAlloc(num, PS_TYPE_F32); // FP x
        radecOut->data[5] = psVectorAlloc(num, PS_TYPE_F32); // FP y
        radecOut->data[6] = psVectorAlloc(num, PS_TYPE_F32); // TP x
        radecOut->data[7] = psVectorAlloc(num, PS_TYPE_F32); // TP y
        psVectorInit(radecOut->data[1], NAN);
        psVectorInit(radecOut->data[2], NAN);
    }

    if (data->streaksName) {
        FILE *streaksFile = fopen(data->streaksName, "r"); // File handle for streaks
        if (!streaksFile) {
            psError(PS_ERR_IO, true, "Unable to open streaks file %s", data->streaksName);
            return false;
        }
        int numStreaks = 0;             // Number of streaks
        if (fscanf(streaksFile, "%d", &numStreaks) != 1) {
            psError(PS_ERR_IO, true, "Unable to read number of streaks from %s", data->streaksName);
            return false;
        }

        streaks = psArrayAlloc(4);
        psVector *ra1 = streaks->data[0] = psVectorAlloc(numStreaks, PS_TYPE_F64);
        psVector *dec1 = streaks->data[1] = psVectorAlloc(numStreaks, PS_TYPE_F64);
        psVector *ra2 = streaks->data[2] = psVectorAlloc(numStreaks, PS_TYPE_F64);
        psVector *dec2 = streaks->data[3] = psVectorAlloc(numStreaks, PS_TYPE_F64);

        for (int i = 0; i < numStreaks; i++) {
            if (fscanf(streaksFile, "%lf %lf %lf %lf %*d",
                       &ra1->data.F64[i], &dec1->data.F64[i], &ra2->data.F64[i], &dec2->data.F64[i]) != 4) {
                psError(PS_ERR_IO, true, "Unable to read streak %d of %d from %s",
                        i, numStreaks, data->streaksName);
                return false;
            }
        }
        streaksOut = psArrayAlloc(8);
        streaksOut->data[0] = psArrayAlloc(numStreaks);
        streaksOut->data[1] = psArrayAlloc(numStreaks);
        streaksOut->data[2] = psVectorAlloc(numStreaks, PS_TYPE_F32);
        streaksOut->data[3] = psVectorAlloc(numStreaks, PS_TYPE_F32);
        streaksOut->data[4] = psArrayAlloc(numStreaks);
        streaksOut->data[5] = psArrayAlloc(numStreaks);
        streaksOut->data[6] = psVectorAlloc(numStreaks, PS_TYPE_F32);
        streaksOut->data[7] = psVectorAlloc(numStreaks, PS_TYPE_F32);
        psVectorInit(streaksOut->data[2], NAN);
        psVectorInit(streaksOut->data[3], NAN);
        psVectorInit(streaksOut->data[6], NAN);
        psVectorInit(streaksOut->data[7], NAN);
    }

    if (data->clustersName) {
        psString file = psSlurpFilename(data->clustersName); // Contents of clusters file
        if (!file) {
            psError(psErrorCodeLast(), false, "Unable to read clusters file %s", data->clustersName);
            return false;
        }
        psArray *lines = psStringSplitArray(file, "\n", false); // Lines of clusters
        psFree(file);

        int num = lines->n - 1;         // Number of clusters
        clusters = psArrayAlloc(2);
        psVector *ra = clusters->data[0] = psVectorAlloc(num, PS_TYPE_F64);
        psVector *dec = clusters->data[1] = psVectorAlloc(num, PS_TYPE_F64);

        // Skip the first line
        for (int i = 1; i < lines->n; i++) {
            const char *line = lines->data[i]; // Line of interest
            if (sscanf(line, "%lf %lf", &ra->data.F64[i-1], &dec->data.F64[i-1]) != 2) {
                psError(PS_ERR_IO, true, "Unable to read line %d of %s", i, data->clustersName);
                return false;
            }
        }
        psFree(lines);

        clustersOut = psArrayAlloc(4);
        clustersOut->data[0] = psArrayAlloc(num);
        clustersOut->data[1] = psVectorAlloc(num, PS_TYPE_F32);
        clustersOut->data[2] = psVectorAlloc(num, PS_TYPE_F32);
        clustersOut->data[3] = psArrayAlloc(num);
        psVectorInit(clustersOut->data[1], NAN);
        psVectorInit(clustersOut->data[2], NAN);
    }

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return NULL;
    }

    // find the FPA phu
    bool bilevelAstrometry = false;
    pmHDU *phu = pmFPAviewThisPHU(view, astromFile->fpa);
    if (phu) {
        char *ctype = psMetadataLookupStr(NULL, phu->header, "CTYPE1");
        if (ctype) {
            bilevelAstrometry = !strcmp (&ctype[4], "-DIS");
        }
    }
    if (bilevelAstrometry) {
        if (!pmAstromReadBilevelMosaic(astromFile->fpa, phu->header)) {
            psError(psErrorCodeLast(), false, "Unable to read bilevel mosaic astrometry for input FPA.");
            psFree(view);
            return false;
        }
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, astromFile->fpa, 1))) {
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (rawFile) {
            pmChip *rawChip = pmFPAviewThisChip(view, rawFile->fpa);                   // Chip with raw data
            if (!rawChip || !rawChip->file_exists) {
                continue;
            }
        }
        const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip
        if (data->chipName && strcmp(chipName, data->chipName) != 0) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            psError(psErrorCodeLast(), false, "Error loading data from files.");
            return false;
        }

        if (chip->cells->n != 1) {
            psWarning("More than one cell present for chip %d", view->chip);
        }

	// fprintf (stderr, "chip: %s\n", chipName);

        // read WCS data from the corresponding header
        pmHDU *hdu = pmFPAviewThisHDU (view, astromFile->fpa);
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
            if (!pmAstromReadWCS(astromFile->fpa, chip, hdu->header, 1.0)) {
                psError(psErrorCodeLast(), false, "Unable to read WCS astrometry for input FPA.");
                psFree(view);
                return false;
            }
        }

        if (pixels) {
            psVector *x = pixels->data[0], *y = pixels->data[1]; // Pixel coordinates
            long num = x->n;                                     // Number of coordinates

            psPlane *pix = psPlaneAlloc();   // Pixel coordinates on chip
            psPlane *fp = psPlaneAlloc();    // Focal plane coordinates
            psPlane *tp = psPlaneAlloc();    // Tangent plane coordinates
            psSphere *sky = psSphereAlloc(); // Sky coordinates

            for (long i = 0; i < num; i++) {
                pix->x = x->data.F32[i];
                pix->y = y->data.F32[i];

                psPlaneTransformApply(fp, chip->toFPA, pix);
		psTrace("ppCoord",2,"pix2sky tr1: %f %f\n",fp->x,fp->y);
                psPlaneTransformApply(tp, astromFile->fpa->toTPA, fp);
		psTrace("ppCoord",2,"pix2sky tr2: %f %f\n",tp->x,tp->y);
                psDeproject(sky, tp, astromFile->fpa->toSky);
		psTrace("ppCoord",2,"pix2sky tr3: %f %f\n",sky->r,sky->d);
                if (!data->radians) {
		  sky->r *= 180.0 / M_PI;
		  sky->d *= 180.0 / M_PI;
                }

                fprintf(stdout, "%s %10.3f %10.3f --> %14.10lf %14.10lf : %11.4f %11.4f : %11.4f %11.4f\n", chipName, pix->x, pix->y, sky->r, sky->d, tp->x, tp->y, fp->x, fp->y);
            }
            psFree(pix);
            psFree(fp);
            psFree(tp);
            psFree(sky);
        }


        pmChip *rawChip = rawFile ? pmFPAviewThisChip(view, rawFile->fpa) : NULL; // Chip with raw
        psArray *cellBounds = NULL;                                               // Bounds of each cell
        psArray *cellNames = NULL;                                                // Names of each cell
        psVector *cellParityX = NULL, *cellParityY = NULL;                        // Parity for each cell
        psVector *cellX0 = NULL, *cellY0 = NULL;                                  // Offset for each cell
        psVector *cellBinX = NULL, *cellBinY = NULL;                              // Binning for each cell
        if (rawChip) {
            if (!rawChip->data_exists) {
                // Not interested in this chip
                continue;
            }
            int numCells = rawChip->cells->n; // Number of cells

            cellBounds = psArrayAlloc(numCells);
            cellNames = psArrayAlloc(numCells);
            cellParityX = psVectorAlloc(numCells, PS_TYPE_S32);
            cellParityY = psVectorAlloc(numCells, PS_TYPE_S32);
            cellX0 = psVectorAlloc(numCells, PS_TYPE_S32);
            cellY0 = psVectorAlloc(numCells, PS_TYPE_S32);
            cellBinX = psVectorAlloc(numCells, PS_TYPE_S32);
            cellBinY = psVectorAlloc(numCells, PS_TYPE_S32);

            pmCell *rawCell;        // Cell with raw data
            while ((rawCell = pmFPAviewNextCell(view, rawFile->fpa, 1))) {
                if (!rawCell->data_exists) {
                    continue;
                }
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    psError(psErrorCodeLast(), false, "Error loading data from files.");
                    return false;
                }
                psRegion *region = pmCellExtent(rawCell); // Bounds of cell
                if (!region) {
                    psError(psErrorCodeLast(), false, "Unable to determine extent of cell.");
                    return false;
                }
                cellBounds->data[view->cell] = region;
                cellNames->data[view->cell] = psMemIncrRefCounter(psMetadataLookupStr(NULL, rawCell->concepts, "CELL.NAME"));
                cellParityX->data.S32[view->cell] = psMetadataLookupS32(NULL, rawCell->concepts, "CELL.XPARITY");
                cellParityY->data.S32[view->cell] = psMetadataLookupS32(NULL, rawCell->concepts, "CELL.YPARITY");
                cellX0->data.S32[view->cell] = psMetadataLookupS32(NULL, rawCell->concepts, "CELL.X0");
                cellY0->data.S32[view->cell] = psMetadataLookupS32(NULL, rawCell->concepts, "CELL.Y0");
                cellBinX->data.S32[view->cell] = psMetadataLookupS32(NULL, rawCell->concepts, "CELL.XBIN");
                cellBinY->data.S32[view->cell] = psMetadataLookupS32(NULL, rawCell->concepts, "CELL.YBIN");
                if (cellParityX->data.S32[view->cell] == 0 || cellParityY->data.S32[view->cell] == 0 ||
                    cellBinX->data.S32[view->cell] == 0 || cellBinY->data.S32[view->cell] == 0) {
                    psError(PM_ERR_CONCEPTS, true, "Concepts aren't set: %d %d %d %d %d %d\n",
                            cellParityX->data.S32[view->cell], cellParityY->data.S32[view->cell],
                            cellX0->data.S32[view->cell], cellY0->data.S32[view->cell],
                            cellBinX->data.S32[view->cell], cellBinY->data.S32[view->cell]);
                    return false;
                }

                if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                    psError(psErrorCodeLast(), false, "Error freeing data from files.");
                    return false;
                }
            }
        }

        if (radec) {
            psVector *ra = radec->data[0], *dec = radec->data[1]; // Pixel coordinates
            long num = ra->n;                                     // Number of coordinates

            int numCols = psMetadataLookupS32(NULL, hdu->header, "IMNAXIS1"); // Number of columns
            int numRows = psMetadataLookupS32(NULL, hdu->header, "IMNAXIS2"); // Number of rows
            if (numCols <= 0 || numRows <= 0) {
                psError(psErrorCodeLast(), false, "Unable to read size of chip.");
                return false;
            }

            psArray *chipPix = radecOut->data[0]; // Chip for pixels
            psVector *xPix = radecOut->data[1];   // x coordinate for pixels
            psVector *yPix = radecOut->data[2];   // y coordinate for pixels
            psArray *cellPix = radecOut->data[3]; // Cell for pixels
            psVector *xFP = radecOut->data[4];   // x coordinate for FP
            psVector *yFP = radecOut->data[5];   // y coordinate for FP
            psVector *xTP = radecOut->data[6];   // x coordinate for TP
            psVector *yTP = radecOut->data[7];   // y coordinate for TP

            for (long i = 0; i < num; i++) {
		ppCoordSet src;
		src.sky.r = ra->data.F64[i];
		src.sky.d = dec->data.F64[i];
                coordSky2Chip(&src, data->radians, astromFile->fpa, chip);
		// fprintf (stderr, "x,y: %.2f %.2f\n", src.pix.x, src.pix.y);
                if ((src.pix.x < 0 || src.pix.x > numCols || src.pix.y < 0 || src.pix.y > numRows)) {
                    // Not on this chip
                    continue;
                }

                if (rawChip) {
		    float x, y;
                    psString cellName = NULL; // Name of cell
                    coordChip2Cell(&cellName, &x, &y, src.pix.x, src.pix.y, cellNames, cellBounds,
                                   cellX0, cellY0, cellParityX, cellParityY, cellBinX, cellBinY);
                    cellPix->data[i] = cellName;
		    xPix->data.F32[i] = x;
		    yPix->data.F32[i] = y;
                } else {
		    xPix->data.F32[i] = src.pix.x;
		    yPix->data.F32[i] = src.pix.y;
		}
                chipPix->data[i] = psStringCopy(chipName);
		xFP->data.F32[i] = src.fp.x;
		yFP->data.F32[i] = src.fp.y;
		xTP->data.F32[i] = src.tp.x;
		yTP->data.F32[i] = src.tp.y;
            }
        }

        if (streaks) {
            psArray *chipPix1 = streaksOut->data[0]; // Chip for point 1
            psArray *cellPix1 = streaksOut->data[1]; // Cell for point 1
            psVector *xPix1 = streaksOut->data[2]; // x coordinate for point 1
            psVector *yPix1 = streaksOut->data[3]; // y coordinate for point 1

            psArray *chipPix2 = streaksOut->data[4]; // Chip for point 2
            psArray *cellPix2 = streaksOut->data[5]; // Cell for point 2
            psVector *xPix2 = streaksOut->data[6]; // x coordinate for point 2
            psVector *yPix2 = streaksOut->data[7]; // y coordinate for point 2

            psVector *ra1 = streaks->data[0]; // RA coordinate for point 1
            psVector *dec1 = streaks->data[1]; // Dec coordinate for point 1
            psVector *ra2 = streaks->data[2]; // RA coordinate for point 2
            psVector *dec2 = streaks->data[3]; // Dec coordinate for point 2

            int numCols = psMetadataLookupS32(NULL, hdu->header, "IMNAXIS1"); // Number of columns
            int numRows = psMetadataLookupS32(NULL, hdu->header, "IMNAXIS2"); // Number of rows
            if (numCols <= 0 || numRows <= 0) {
                psError(psErrorCodeLast(), false, "Unable to read size of chip.");
                return false;
            }

            for (long i = 0; i < xPix1->n; i++) {
                float x1, y1;   // Coordinates of point 1
		ppCoordSet src;
		src.sky.r = ra1->data.F64[i];
		src.sky.d = dec1->data.F64[i];
                coordSky2Chip(&src, true, astromFile->fpa, chip);
		x1 = src.pix.x;
		y1 = src.pix.y;
                if ((x1 >= 0 && x1 < numCols && y1 >= 0 && y1 < numRows)) {
                    if (rawChip) {
                        psString cellName = NULL; // Name of cell
                        coordChip2Cell(&cellName, &x1, &y1, x1, y1, cellNames, cellBounds,
                                       cellX0, cellY0, cellParityX, cellParityY, cellBinX, cellBinY);
                        cellPix1->data[i] = cellName;
                    }
                    chipPix1->data[i] = psStringCopy(chipName);
                    xPix1->data.F32[i] = x1;
                    yPix1->data.F32[i] = y1;
                }

                float x2, y2;   // Coordinates of point 2
		src.sky.r = ra2->data.F64[i];
		src.sky.d = dec2->data.F64[i];
                coordSky2Chip(&src, true, astromFile->fpa, chip);
		x2 = src.pix.x;
		y2 = src.pix.y;
                if ((x2 >= 0 && x2 < numCols && y2 >= 0 && y2 < numRows)) {
                    if (rawChip) {
                        psString cellName = NULL; // Name of cell
                        coordChip2Cell(&cellName, &x2, &y2, x2, y2, cellNames, cellBounds,
                                       cellX0, cellY0, cellParityX, cellParityY, cellBinX, cellBinY);
                        cellPix2->data[i] = cellName;
                    }
                    chipPix2->data[i] = psStringCopy(chipName);
                    xPix2->data.F32[i] = x2;
                    yPix2->data.F32[i] = y2;
                }
            }
        }

        if (clusters) {
            psVector *ra = clusters->data[0], *dec = clusters->data[1]; // Pixel coordinates
            long num = ra->n;                                     // Number of coordinates

            int numCols = psMetadataLookupS32(NULL, hdu->header, "IMNAXIS1"); // Number of columns
            int numRows = psMetadataLookupS32(NULL, hdu->header, "IMNAXIS2"); // Number of rows
            if (numCols <= 0 || numRows <= 0) {
                psError(psErrorCodeLast(), false, "Unable to read size of chip.");
                return false;
            }

            psArray *chipPix = clustersOut->data[0]; // Chip for pixels
            psVector *xPix = clustersOut->data[1];   // x coordinate for pixels
            psVector *yPix = clustersOut->data[2];   // y coordinate for pixels
            psArray *cellPix = clustersOut->data[3]; // Cell for pixels

            for (long i = 0; i < num; i++) {
                float x, y;             // Pixel coordinates
		ppCoordSet src;
		src.sky.r = ra->data.F64[i];
		src.sky.d = dec->data.F64[i];
                coordSky2Chip(&src, false, astromFile->fpa, chip);
		x = src.pix.x;
		y = src.pix.y;
                if ((x < 0 || x > numCols || y < 0 || y > numRows)) {
                    // Not on this chip
                    continue;
                }

                if (rawChip) {
                    psString cellName = NULL; // Name of cell
                    coordChip2Cell(&cellName, &x, &y, x, y, cellNames, cellBounds,
                                   cellX0, cellY0, cellParityX, cellParityY, cellBinX, cellBinY);
                    cellPix->data[i] = cellName;
                }

                chipPix->data[i] = psStringCopy(chipName);
                xPix->data.F32[i] = x;
                yPix->data.F32[i] = y;
            }
        }

        psFree(cellNames);
        psFree(cellBounds);
        psFree(cellParityX);
        psFree(cellParityY);
        psFree(cellBinX);
        psFree(cellBinY);
        psFree(cellX0);
        psFree(cellY0);

        // Chip
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            psError(psErrorCodeLast(), false, "Error saving data to files.");
            return false;
        }
    }
    // FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        psError(psErrorCodeLast(), false, "Error saving data to files.");
        return false;
    }

    if (radecOut) {
        psArray *chipPix = radecOut->data[0]; // Chip for pixels
        psVector *xPix = radecOut->data[1];   // x coordinate for pixels
        psVector *yPix = radecOut->data[2];   // y coordinate for pixels
        psArray *cellPix = radecOut->data[3]; // Cell for pixels
        psVector *ra = radec->data[0];        // RA coordinate
        psVector *dec = radec->data[1];       // Dec coordinate
	psVector *xFP = radecOut->data[4];   // x coordinate for FP
	psVector *yFP = radecOut->data[5];   // y coordinate for FP
	psVector *xTP = radecOut->data[6];   // x coordinate for TP
	psVector *yTP = radecOut->data[7];   // y coordinate for TP

        for (long i = 0; i < chipPix->n; i++) {
            const char *chipName = chipPix->data[i]; // Name of chip
            const char *cellName = cellPix->data[i]; // Name of cell, or NULL
            if (!data->all && (!isfinite(xPix->data.F32[i]) || !isfinite(yPix->data.F32[i]) ||
                               !chipName || (rawFile && !cellName))) {
                continue;
            }
            if (!rawFile && data->ds9) {
                // Region file is only appropriate if we're not mapping all the way back to cell coordinates
                fprintf(data->ds9, "image;circle(%f,%f,%f) # color=%s\n",
                        xPix->data.F32[i], yPix->data.F32[i], data->ds9radius, data->ds9color);
            } else {
                fprintf(stdout, "%14.10lf %14.10lf --> %10.3f %10.3f %s%s%s : %11.4f %11.4f : %11.4f %11.4f\n",
                        ra->data.F64[i], dec->data.F64[i], xPix->data.F32[i], yPix->data.F32[i],
                        chipName ? chipName : "UNKNOWN",
                        rawFile ? " " : "",
                        rawFile && cellName ? cellName : (rawFile ? "UNKNOWN" : ""),
			xFP->data.F32[i], yFP->data.F32[i], xTP->data.F32[i], yTP->data.F32[i] );
            }
        }
    }

    if (streaksOut) {
        psArray *chipPix1 = streaksOut->data[0]; // Chip for point 1
        psArray *cellPix1 = streaksOut->data[1]; // Cell for point 1
        psVector *xPix1 = streaksOut->data[2]; // x coordinate for point 1
        psVector *yPix1 = streaksOut->data[3]; // y coordinate for point 1
        psArray *chipPix2 = streaksOut->data[4]; // Chip for point 2
        psArray *cellPix2 = streaksOut->data[5]; // Cell for point 2
        psVector *xPix2 = streaksOut->data[6]; // x coordinate for point 2
        psVector *yPix2 = streaksOut->data[7]; // y coordinate for point 2

        psVector *ra1 = streaks->data[0]; // RA coordinate for point 1
        psVector *dec1 = streaks->data[1]; // Dec coordinate for point 1
        psVector *ra2 = streaks->data[2]; // RA coordinate for point 2
        psVector *dec2 = streaks->data[3]; // Dec coordinate for point 2

        for (long i = 0; i < xPix1->n; i++) {
            const char *chipName1 = chipPix1->data[i]; // Name of chip for point 1
            const char *cellName1 = cellPix1->data[i]; // Name of cell for point 1
            const char *chipName2 = chipPix2->data[i]; // Name of chip for point 2
            const char *cellName2 = cellPix2->data[i]; // Name of cell for point 2
            if (!data->all &&
                (!isfinite(xPix1->data.F32[i]) || !isfinite(yPix1->data.F32[i]) ||
                 !chipName1 || (rawFile && !cellName1) ||
                 !isfinite(xPix2->data.F32[i]) || !isfinite(yPix2->data.F32[i]) ||
                 !chipName2 || (rawFile && !cellName2))) {
                continue;
            }
            if (!rawFile && data->ds9) {
                // Region file is only appropriate if we're not mapping all the way back to cell coordinates
                fprintf(data->ds9, "image;line(%f,%f,%f,%f) # line= 0 0 color=%s\n",
                        xPix1->data.F32[i], yPix1->data.F32[i], xPix2->data.F32[i], yPix2->data.F32[i],
                        data->ds9color);
            } else {
                fprintf(stdout, "Streak %.10lf %.10lf %.10lf %.10lf --> %.3f %.3f %s%s%s %.3f %.3f %s%s%s\n",
                        ra1->data.F64[i], dec1->data.F64[i],
                        ra2->data.F64[i], dec2->data.F64[i],
                        xPix1->data.F32[i], yPix1->data.F32[i],
                        chipName1 ? chipName1 : "UNKNOWN",
                        rawFile ? " " : "",
                        rawFile && cellName1 ? cellName1 : (rawFile ? "UNKNOWN" : ""),
                        xPix2->data.F32[i], yPix2->data.F32[i],
                        chipName2 ? chipName2 : "UNKNOWN",
                        rawFile ? " " : "",
                        rawFile && cellName2 ? cellName2 : (rawFile ? "UNKNOWN" : "")
                    );
            }
        }
    }

    if (clustersOut) {
        psArray *chipPix = clustersOut->data[0]; // Chip for pixels
        psVector *xPix = clustersOut->data[1];   // x coordinate for pixels
        psVector *yPix = clustersOut->data[2];   // y coordinate for pixels
        psArray *cellPix = clustersOut->data[3]; // Cell for pixels
        psVector *ra = clusters->data[0];        // RA coordinate
        psVector *dec = clusters->data[1];       // Dec coordinate

        for (long i = 0; i < chipPix->n; i++) {
            const char *chipName = chipPix->data[i]; // Name of chip
            const char *cellName = cellPix->data[i]; // Name of cell, or NULL
            if (!data->all && (!isfinite(xPix->data.F32[i]) || !isfinite(yPix->data.F32[i]) ||
                               !chipName || (rawFile && !cellName))) {
                continue;
            }
            if (!rawFile && data->ds9) {
                // Region file is only appropriate if we're not mapping all the way back to cell coordinates
                fprintf(data->ds9, "image;circle(%f,%f,%f) # color=%s\n",
                        xPix->data.F32[i], yPix->data.F32[i], data->ds9radius, data->ds9color);
            } else {
                fprintf(stdout, "Cluster %.10lf %.10lf --> %.3f %.3f %s%s%s\n",
                        ra->data.F64[i], dec->data.F64[i], xPix->data.F32[i], yPix->data.F32[i],
                        chipName ? chipName : "UNKNOWN",
                        rawFile ? " " : "",
                        rawFile && cellName ? cellName : (rawFile ? "UNKNOWN" : ""));
            }
        }
    }

    psFree(pixels);
    psFree(radec);
    psFree(radecOut);
    psFree(streaks);
    psFree(streaksOut);
    psFree(clusters);
    psFree(clustersOut);

    return true;
}
