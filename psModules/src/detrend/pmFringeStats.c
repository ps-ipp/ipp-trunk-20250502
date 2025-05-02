#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFringeStats.h"

#include "psPolynomialMD.h"
#include "psMinimizePolyFit.h"
#include "psVector.h"


// Future optimisations for speed:
//
// 1. Clipping --- don't re-do the matrix setup again, but carry matrix and vector around, subtract
// contributions from clipped data points.
// 2. Faster psImageStats (use memcpy?)



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeRegions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void fringeRegionsFree(pmFringeRegions *fringe)
{
    psFree(fringe->x);
    psFree(fringe->y);
    psFree(fringe->mask);
    return;
}

pmFringeRegions *pmFringeRegionsAlloc(int nPts, int dX, int dY, int nX, int nY)
{
    pmFringeRegions *fringe = psAlloc(sizeof(pmFringeRegions));
    (void)psMemSetDeallocator(fringe, (psFreeFunc)fringeRegionsFree);

    fringe->x = NULL;
    fringe->y = NULL;
    fringe->mask = NULL;

    fringe->nRequested = nPts;
    fringe->nAccepted = 0;

    fringe->dX = dX;
    fringe->dY = dY;
    fringe->nX = nX;
    fringe->nY = nY;

    return fringe;
}

bool pmFringeRegionsCreatePoints(pmFringeRegions *fringe, const psImage *image, psRandom *random)
{
    PS_ASSERT_PTR_NON_NULL(fringe, false);
    PS_ASSERT_PTR_NON_NULL(image, false);
    PS_ASSERT_IMAGE_NON_EMPTY(image, false);

    double frnd;
    // create fringe->nRequested

    psRandom *rng;
    if (random) {
        rng = psMemIncrRefCounter(random);
    } else {
        rng = psRandomAlloc(PS_RANDOM_TAUS);
    }

    fringe->x = psVectorRecycle(fringe->x, fringe->nRequested, PS_TYPE_F32);
    fringe->y = psVectorRecycle(fringe->y, fringe->nRequested, PS_TYPE_F32);
    fringe->mask = psVectorRecycle(fringe->mask, fringe->nRequested, PS_TYPE_VECTOR_MASK);
    fringe->x->n = fringe->y->n = fringe->mask->n = fringe->nRequested;
    psVectorInit(fringe->mask, 0);

    int nX = image->numCols;
    int nY = image->numRows;

    psF32 *xPt = fringe->x->data.F32;
    psF32 *yPt = fringe->y->data.F32;

    int dX = fringe->dX;
    int dY = fringe->dY;

    // generate random points located within image bounds
    for (int i = 0; i < fringe->nRequested; i++) {
        frnd = psRandomUniform(rng);
        xPt[i] = (nX - 2*dX)* frnd + dX;
        frnd = psRandomUniform(rng);
        yPt[i] = (nY - 2*dY)* frnd + dY;
    }

    psFree(rng);

    return true;
}

bool pmFringeRegionsWriteFits(psFits *fits, psMetadata *header,
                              const pmFringeRegions *regions, const char *extname)
{
    // Make sure the input is well-behaved
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(regions, false);
    psVector *x = regions->x;           // The x positions
    psVector *y = regions->y;           // The y positions
    psVector *mask = regions->mask;     // The region mask
    int numRows = regions->nRequested;  // Number of rows in the table
    PS_ASSERT_INT_POSITIVE(numRows, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_SIZE(x, (long)numRows, false);
    PS_ASSERT_VECTOR_SIZE(y, (long)numRows, false);
    if (mask) {
        PS_ASSERT_VECTOR_NON_NULL(mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
        PS_ASSERT_VECTOR_SIZE(mask, (long)numRows, false);
    }

    // We need to write:
    // Scalars: dX, dY, nX, nY
    // Vectors: x, y, mask

    psMetadata *scalars = psMemIncrRefCounter(header); // Metadata to hold the scalars; will be the header
    if (!scalars) {
        scalars = psMetadataAlloc();
    }
    psMetadataAddS32(scalars, PS_LIST_TAIL, "PSFRNGDX", PS_META_REPLACE, "Median box half-width",
                     regions->dX);
    psMetadataAddS32(scalars, PS_LIST_TAIL, "PSFRNGDY", PS_META_REPLACE, "Median box half-height",
                     regions->dY);
    psMetadataAddS32(scalars, PS_LIST_TAIL, "PSFRNGNX", PS_META_REPLACE, "Large-scale smoothing in x",
                     regions->nX);
    psMetadataAddS32(scalars, PS_LIST_TAIL, "PSFRNGNY", PS_META_REPLACE, "Large-scale smoothing in y",
                     regions->nY);

    psArray *table = psArrayAlloc(numRows); // The table
    // Translate the vectors into the required format for psFitsWriteTable()
    for (long i = 0; i < numRows; i++) {
        psMetadata *row = psMetadataAlloc();
        psMetadataAddF32(row, PS_LIST_TAIL, "x", PS_META_REPLACE, "Fringe position in x", x->data.F32[i]);
        psMetadataAddF32(row, PS_LIST_TAIL, "y", PS_META_REPLACE, "Fringe position in y", y->data.F32[i]);
        psVectorMaskType maskValue = 0;
        if (mask && mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            maskValue = 0xff;
        }
        psMetadataAddVectorMask(row, PS_LIST_TAIL, "mask", PS_META_REPLACE, "Mask", maskValue);
        table->data[i] = row;
    }

    bool success;                       // Success of operation
    if (!(success = psFitsWriteTable(fits, scalars, table, extname))) {
        psError(PS_ERR_IO, false, "Unable to write fringe data to extension %s\n", extname);
    }
    psFree(scalars);
    psFree(table);

    return success;
}

pmFringeRegions *pmFringeRegionsReadFits(psMetadata *header, const psFits *fits, const char *extname)
{
    PS_ASSERT_PTR_NON_NULL(fits, NULL);

    if (extname && strlen(extname) > 0) {
        if (!psFitsMoveExtName(fits, extname)) {
            psError(PS_ERR_IO, false, "Unable to move to extension %s\n", extname);
            return NULL;
        }
    } else if (!psFitsMoveExtNum(fits, 0, false)) {
        psError(PS_ERR_IO, false, "Unable to move to PHU\n");
        return NULL;
    }

    psMetadata *headerCopy = psMemIncrRefCounter(header); // Copy of the header, or NULL

    headerCopy = psFitsReadHeader(headerCopy, fits); // The FITS header
    if (!headerCopy) {
        psError(PS_ERR_IO, false, "Unable to read header for extension %s\n", extname);
        psFree(header);
        return NULL;
    }

    // Read the scalars from the header
    #define READ_SCALAR(SCALAR, NAME) \
    int SCALAR = psMetadataLookupS32(&mdok, headerCopy, NAME); \
    if (!mdok || SCALAR <= 0) { \
        psError(PS_ERR_IO, true, "Unable to find " NAME " in header of extension %s.\n", extname); \
        psFree(headerCopy); \
        return NULL; \
    }

    // Need to retrieve the scalars: dX, dY, nX, nY
    bool mdok = true;                   // Status of MD lookup
    READ_SCALAR(dX, "PSFRNGDX");
    READ_SCALAR(dY, "PSFRNGDY");
    READ_SCALAR(nX, "PSFRNGNX");
    READ_SCALAR(nY, "PSFRNGNY");
    psFree(headerCopy);

    // Now the vectors: x, y, mask
    psArray *table = psFitsReadTable(fits); // The table
    long numRows = table->n;            // Number of rows

    pmFringeRegions *regions = pmFringeRegionsAlloc(numRows, dX, dY, nX, nY); // The fringe regions
    psVector *x = psVectorAlloc(numRows, PS_TYPE_F32); // x position
    psVector *y = psVectorAlloc(numRows, PS_TYPE_F32); // y position
    psVector *mask = psVectorAlloc(numRows, PS_TYPE_VECTOR_MASK); // mask
    regions->x = x;
    regions->y = y;
    regions->mask = mask;

    #define READ_REGIONS_ROW(VECTOR, TYPE, DATATYPE, NAME, DESCRIPTION) \
    VECTOR->data.DATATYPE[i] = psMetadataLookup##TYPE(&mdok, row, NAME); \
    if (!mdok) { \
        psError(PS_ERR_IO, true, "Unable to find " #DESCRIPTION " .\n"); \
        psFree(table); \
        psFree(regions); \
        return NULL; \
    }

    // Translate the table into vectors
    for (long i = 0; i < numRows; i++) {
        psMetadata *row = table->data[i]; // Table row
        READ_REGIONS_ROW(x, F32, F32, "x", "x position");
        READ_REGIONS_ROW(y, F32, F32, "y", "y position");
        READ_REGIONS_ROW(mask, VectorMask, PS_TYPE_VECTOR_MASK_DATA, "mask", "mask");
    }
    psFree(table);

    return regions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeStats
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void fringeStatsFree(pmFringeStats *stats)
{
    psFree(stats->regions);
    psFree(stats->f);
    psFree(stats->df);
}

pmFringeStats *pmFringeStatsAlloc(pmFringeRegions *regions)
{
    PS_ASSERT_PTR_NON_NULL(regions, false);

    pmFringeStats *stats = psAlloc(sizeof(pmFringeStats));
    (void)psMemSetDeallocator(stats, (psFreeFunc)fringeStatsFree);

    int numRegions = regions->nRequested; // Number of regions
    stats->regions = psMemIncrRefCounter(regions);
    stats->f = psVectorAlloc(numRegions, PS_TYPE_F32);
    stats->df = psVectorAlloc(numRegions, PS_TYPE_F32);

    return stats;
}

pmFringeStats *pmFringeStatsMeasure(pmFringeRegions *fringe, const pmReadout *readout, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(fringe, NULL);
    PS_ASSERT_PTR_NON_NULL(readout, NULL);
    PS_ASSERT_PTR_NON_NULL(readout->image, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(readout->image, NULL);

    if (!fringe->x || !fringe->y) {
        // create the fringe vectors for this image
        pmFringeRegionsCreatePoints(fringe, readout->image, NULL);
    }

    PS_ASSERT_PTR_NON_NULL(fringe->x, false);
    PS_ASSERT_PTR_NON_NULL(fringe->y, false);

    pmFringeStats *measurements = pmFringeStatsAlloc(fringe);

    psF32 *xPt = fringe->x->data.F32;
    psF32 *yPt = fringe->y->data.F32;
    psF32 *fPt = measurements->f->data.F32;
    psF32 *dfPt = measurements->df->data.F32;

    int dX = fringe->dX;
    int dY = fringe->dY;

    psImage *image = readout->image;
    psImage *mask  = readout->mask;

    psStats *median = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN); // Median statistics only
    psStats *medianSd = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV); // Median and SD

    // Measure the sky in each smoothing box
    psImage *sky = psImageAlloc(fringe->nX, fringe->nY, PS_TYPE_F32);
    for (int i = 0; i < fringe->nY; i++) {
        int y0 = image->row0 + (float)i * (float)image->numRows / (float)fringe->nY;
        int y1 = image->row0 + (float)(i + 1) * (float)image->numRows / (float)fringe->nY;
        for (int j = 0; j < fringe->nX; j++) {
            int x0 = image->col0 + (float)j * (float)image->numCols / (float)fringe->nX;
            int x1 = image->col0 + (float)(j + 1) * (float)image->numCols / (float)fringe->nX;
            psRegion region = psRegionSet(x0, x1, y0, y1);
            psImage *subImage = psImageSubset(image, region); // Subimage of the sky region
            psImage *subMask = NULL;
            if (mask) {
                subMask = psImageSubset(mask, region); // Subimage of the sky region
            }
            psImageStats(median, subImage, subMask, maskVal);
            sky->data.F32[i][j] = median->sampleMedian;
            psFree(subImage);
            psFree(subMask);
        }
    }

    for (int i = 0; i < fringe->x->n; i++) {
        psRegion region = psRegionSet(image->col0 + xPt[i] - dX,
                                      image->col0 + xPt[i] + dX + 1,
                                      image->row0 + yPt[i] - dY,
                                      image->row0 + yPt[i] + dY + 1);
        psImage *subImage = psImageSubset(image, region);
        psImage *subMask = NULL;
        if (mask) {
            subMask = psImageSubset(mask, region);
        }
        psImageStats(medianSd, subImage, subMask, maskVal);
        psFree(subImage);
        psFree(subMask);

        int xSky = xPt[i] / (float)image->numCols * (float)sky->numCols;
        int ySky = yPt[i] / (float)image->numRows * (float)sky->numRows;

        fPt[i] = medianSd->sampleMedian - sky->data.F32[ySky][xSky];
        dfPt[i] = 1.0 / medianSd->sampleStdev;

	if (readout->parent->hdu) {
	  psTrace("psModules.detrend", 7, "[%d:%d,%d:%d]: %f %f : %s\n", (int)region.x0, (int)region.x1,
		  (int)region.y0, (int)region.y1, fPt[i], dfPt[i], readout->parent->hdu->extname);
	}
	else {
	  psTrace("psModules.detrend", 7, "[%d:%d,%d:%d]: %f %f : THIS_IS_A_SPOOKY_GHOST_CELL\n", (int)region.x0, (int)region.x1,
		  (int)region.y0, (int)region.y1, fPt[i], dfPt[i]);
	}
    }
    psFree(sky);
    psFree(median);
    psFree(medianSd);

    return measurements;
}

bool pmFringeStatsWriteFits(psFits *fits,
                            psMetadata *header,
                            const pmFringeStats *fringe,
                            const char *extname
                           )
{
    // Make sure the input is well-behaved
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(fringe, false);
    pmFringeRegions *regions = fringe->regions; // The fringe regions
    PS_ASSERT_PTR_NON_NULL(regions, false);
    int numRows = regions->nRequested;  // Number of rows in the table
    PS_ASSERT_INT_POSITIVE(numRows, false);
    psVector *f = fringe->f;            // The fringe measurements
    psVector *df = fringe->df;      // The fringe standard deviatiations
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_NON_NULL(df, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_TYPE(df, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_SIZE(f, (long)numRows, false);
    PS_ASSERT_VECTOR_SIZE(df, (long)numRows, false);

    // We need to write:
    // Vectors: f, df
    psArray *table = psArrayAlloc(numRows); // The table
    // Translate the vectors into the required format for psFitsWriteTable()
    for (long i = 0; i < numRows; i++) {
        psMetadata *row = psMetadataAlloc();
        psMetadataAddF32(row, PS_LIST_TAIL, "f", PS_META_REPLACE, "Fringe measurement", f->data.F32[i]);
        psMetadataAddF32(row, PS_LIST_TAIL, "df", PS_META_REPLACE, "Fringe stdev", df->data.F32[i]);
        table->data[i] = row;
    }

    if (!psFitsWriteTable(fits, header, table, extname)) {
        psError(PS_ERR_IO, false, "Unable to write fringe data to extension %s\n", extname);
        psFree(table);
        return false;
    }

    psFree(table);
    return true;
}

pmFringeStats *pmFringeStatsReadFits(psMetadata *header, const psFits *fits, const char *extname,
                                     pmFringeRegions *regions)
{
    PS_ASSERT_PTR_NON_NULL(fits, NULL);
    PS_ASSERT_PTR_NON_NULL(regions, NULL);
    PS_ASSERT_INT_POSITIVE(regions->nRequested, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(regions->x, regions->y, NULL);
    PS_ASSERT_VECTOR_SIZE(regions->x, (long)regions->nRequested, NULL);

    if (extname && strlen(extname) > 0) {
        if (!psFitsMoveExtName(fits, extname)) {
            psError(PS_ERR_IO, false, "Unable to move to extension %s\n", extname);
            return NULL;
        }
    } else if (!psFitsMoveExtNum(fits, 0, false)) {
        psError(PS_ERR_IO, false, "Unable to move to PHU\n");
        return NULL;
    }

    psMetadata *headerCopy = psMemIncrRefCounter(header); // Copy of the header, or NULL

    headerCopy = psFitsReadHeader(headerCopy, fits); // The FITS header
    if (!headerCopy) {
        psError(PS_ERR_IO, false, "Unable to read header for extension %s\n", extname);
        psFree(headerCopy);
        return NULL;
    }
    psFree(headerCopy);

    // Now the vectors: f, df
    psArray *table = psFitsReadTable(fits); // The table
    long numRows = table->n;            // Number of rows

    pmFringeStats *fringes = pmFringeStatsAlloc(regions); // The fringe measurements
    psVector *f = fringes->f;           // fringe measurement
    psVector *df = fringes->df;         // fringe stdev

    #define READ_STATS_ROW(VECTOR, TYPE, NAME, DESCRIPTION) \
    VECTOR->data.TYPE[i] = psMetadataLookup##TYPE(&mdok, row, NAME); \
    if (!mdok) { \
        psError(PS_ERR_IO, true, "Unable to find " #DESCRIPTION " .\n"); \
        psFree(table); \
        psFree(fringes); \
        return NULL; \
    }

    // Translate the table into vectors
    bool mdok;                          // Status of MD lookup
    for (long i = 0; i < numRows; i++) {
        psMetadata *row = table->data[i]; // Table row
        READ_STATS_ROW(f, F32, "f", "fringe measurement");
        READ_STATS_ROW(df, F32, "df", "fringe standard deviation");
    }
    psFree(table);

    return fringes;
}


pmFringeStats *pmFringeStatsConcatenate(const psArray *fringes, const psVector *x0, const psVector *y0)
{
    PS_ASSERT_PTR_NON_NULL(fringes, NULL);
    PS_ASSERT_PTR_NON_NULL(fringes->data, NULL);
    PS_ASSERT_INT_POSITIVE(fringes->n, NULL);
    if (x0 && y0) {
        PS_ASSERT_VECTOR_NON_NULL(x0, NULL);
        PS_ASSERT_VECTOR_NON_NULL(y0, NULL);
        PS_ASSERT_VECTOR_TYPE(x0, PS_TYPE_S32, NULL);
        PS_ASSERT_VECTOR_TYPE(y0, PS_TYPE_S32, NULL);
        PS_ASSERT_VECTORS_SIZE_EQUAL(x0, y0, NULL);
        PS_ASSERT_VECTOR_SIZE(x0, fringes->n, NULL);
        PS_ASSERT_VECTOR_SIZE(y0, fringes->n, NULL);
    }

    // Get the measurement parameters, and check they are consistent
    int numPoints = 0;                  // Number of fringe points
    int dX = 0, dY = 0;                 // Half-width and -height of fringe boxes
    int nX = 0, nY = 0;                 // Smoothing scales
    for (long i = 0; i < fringes->n; i++) {
        pmFringeStats *fringe = fringes->data[i]; // The fringe of interest
	if (!fringe) {
	    psWarning ("skipping empty fringe stats for entry %ld -- video cell?\n", i);
	    continue;
	}
        pmFringeRegions *regions = fringe->regions; // The fringe regions
        if (numPoints == 0) {
            dX = regions->dX;
            dY = regions->dY;
            nX = regions->nX;
            nY = regions->nY;
        } else if (regions->dX != dX || regions->dY != dY || regions->nX != nX || regions->nY != nY) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Fringe %ld has different parameters (%d,%d,%d,%d) "
                    "from the first (%d,%d,%d,%d).\n", i,
                    regions->dX, regions->dY, regions->nX, regions->nY, dX, dY, nX, nY);
            return NULL;
        }
        int num = regions->nRequested;  // Number of fringe points
        if (regions->x->n != num || regions->y->n != num || regions->mask->n != num ||
                fringe->f->n != num || fringe->df->n != num) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Length of region (%ld,%ld,%ld) and fringe vectors "
                    "do not match (%ld,%ld) with the official value (%d).\n", regions->x->n, regions->y->n,
                    regions->mask->n, fringe->f->n, fringe->df->n, num);
            return NULL;
        }
        numPoints += regions->nRequested;
    }

    pmFringeRegions *newRegions = pmFringeRegionsAlloc(numPoints, dX, dY, nX, nY); // The new list of regions
    newRegions->x = psVectorAlloc(numPoints, PS_TYPE_F32);
    newRegions->y = psVectorAlloc(numPoints, PS_TYPE_F32);
    newRegions->mask = psVectorAlloc(numPoints, PS_TYPE_VECTOR_MASK);
    pmFringeStats *newStats = pmFringeStatsAlloc(newRegions); // The new list of statistics

    long offset = 0;                    // Offset from start of the list
    for (long i = 0; i < fringes->n; i++) {
        pmFringeStats *fringe = fringes->data[i]; // The fringe of interest
	if (!fringe) {
	    psWarning ("skipping empty fringe stats for entry %ld -- video cell?\n", i);
	    continue;
	}
        pmFringeRegions *regions = fringe->regions; // The fringe regions
        // Copy the data over
        memcpy(&newRegions->x->data.F32[offset], regions->x->data.F32, regions->x->n * sizeof(psF32));
        memcpy(&newRegions->y->data.F32[offset], regions->y->data.F32, regions->y->n * sizeof(psF32));
        memcpy(&newRegions->mask->data.PS_TYPE_VECTOR_MASK_DATA[offset], regions->mask->data.PS_TYPE_VECTOR_MASK_DATA, regions->mask->n * sizeof(psVectorMaskType));
        memcpy(&newStats->f->data.F32[offset], fringe->f->data.F32, fringe->f->n * sizeof(psF32));
        memcpy(&newStats->df->data.F32[offset], fringe->df->data.F32, fringe->df->n * sizeof(psF32));
        if (x0 && y0) {
            for (long j = offset; j < offset + regions->x->n; j++) {
                newRegions->x->data.F32[j] += x0->data.S32[i];
                newRegions->y->data.F32[j] += y0->data.S32[i];
            }
        }
        offset += regions->nRequested;
    }

    psFree(newRegions);                 // Drop reference
    return newStats;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeIO
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

psArray *pmFringesFormatTable(psMetadata *header, const psArray *fringes)
{
    PS_ASSERT_PTR_NON_NULL(header, false);
    PS_ASSERT_ARRAY_NON_NULL(fringes, false);

    // Check the regions are all identical
    pmFringeRegions *regions = ((pmFringeStats*)fringes->data[0])->regions; // First region
    for (int i = 1; i < fringes->n; i++) {
        pmFringeStats *stats = fringes->data[i];
        if (stats->regions != regions) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Regions for fringe statistics are not identical.\n");
            return NULL;
        }
    }

    // Ensure the region is legit
    psVector *x = regions->x;           // The x positions
    psVector *y = regions->y;           // The y positions
    psVector *mask = regions->mask;     // The region mask
    int numRows = regions->nRequested;  // Number of rows in the table
    PS_ASSERT_INT_POSITIVE(numRows, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_SIZE(x, (long)numRows, false);
    PS_ASSERT_VECTOR_SIZE(y, (long)numRows, false);
    if (mask) {
        PS_ASSERT_VECTOR_NON_NULL(mask, false);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, false);
        PS_ASSERT_VECTOR_SIZE(mask, (long)numRows, false);
    }

    // We need to write:
    // Scalars: dX, dY, nX, nY
    // Vectors: x, y, mask, f, df

    psMetadataAddS32(header, PS_LIST_TAIL, "PSFRNGDX", PS_META_REPLACE, "Median box half-width", regions->dX);
    psMetadataAddS32(header, PS_LIST_TAIL, "PSFRNGDY", PS_META_REPLACE, "Median box half-height", regions->dY);
    psMetadataAddS32(header, PS_LIST_TAIL, "PSFRNGNX", PS_META_REPLACE, "Large-scale smoothing in x", regions->nX);
    psMetadataAddS32(header, PS_LIST_TAIL, "PSFRNGNY", PS_META_REPLACE, "Large-scale smoothing in y", regions->nY);

    psArray *table = psArrayAlloc(numRows); // The table
    // Translate the vectors into the required format for psFitsWriteTable()
    for (long i = 0; i < numRows; i++) {
        psMetadata *row = psMetadataAlloc();
        psMetadataAddF32(row, PS_LIST_TAIL, "x", PS_META_REPLACE, "Fringe position in x", x->data.F32[i]);
        psMetadataAddF32(row, PS_LIST_TAIL, "y", PS_META_REPLACE, "Fringe position in y", y->data.F32[i]);
        psVectorMaskType maskValue = 0;             // Mask value
        if (mask && mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            maskValue = 0xff;
        }

        psVector *f = psVectorAlloc(fringes->n, PS_TYPE_F32); // Measurements for each fringe component
        psVector *df = psVectorAlloc(fringes->n, PS_TYPE_F32); // Errors in measurements
        for (long j = 0; j < fringes->n; j++) {
            pmFringeStats *stats = fringes->data[j]; // Fringe statistics of interest
            f->data.F32[j] = stats->f->data.F32[i];
            df->data.F32[j] = stats->df->data.F32[i];
            if (!isfinite(f->data.F32[j]) || !isfinite(df->data.F32[j])) {
                maskValue = 0xff;
            }
        }
        psMetadataAdd(row, PS_LIST_TAIL, "f", PS_DATA_VECTOR | PS_META_REPLACE, "Fringe measurements", f);
        psMetadataAdd(row, PS_LIST_TAIL, "df", PS_DATA_VECTOR | PS_META_REPLACE, "Fringe errors", df);
        // Drop references
        psFree(f);
        psFree(df);

        psMetadataAddVectorMask(row, PS_LIST_TAIL, "mask", PS_META_REPLACE, "Mask", maskValue);
        table->data[i] = row;
    }

    return table;
}

psArray *pmFringesParseTable(psArray *table, psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(table, NULL);
    PS_ASSERT_PTR_NON_NULL(header, NULL);

    bool mdok;                          // Status of MD lookup

    // Read the scalars from the header
    #define READ_FRINGES_SCALAR(SCALAR, NAME) \
    int SCALAR = psMetadataLookupS32(&mdok, header, NAME); \
    if (!mdok || SCALAR <= 0) { \
        psError(PS_ERR_IO, true, "Unable to find " NAME " in fringe header.\n"); \
        return NULL; \
    }

    // Need to retrieve the scalars: dX, dY, nX, nY
    READ_FRINGES_SCALAR(dX, "PSFRNGDX");
    READ_FRINGES_SCALAR(dY, "PSFRNGDY");
    READ_FRINGES_SCALAR(nX, "PSFRNGNX");
    READ_FRINGES_SCALAR(nY, "PSFRNGNY");

    // Now the vectors: x, y, mask, f, df
    long numRows = table->n;            // Number of rows
    pmFringeRegions *regions = pmFringeRegionsAlloc(numRows, dX, dY, nX, nY); // The fringe regions
    psVector *x = psVectorAlloc(numRows, PS_TYPE_F32); // x position
    psVector *y = psVectorAlloc(numRows, PS_TYPE_F32); // y position
    psVector *mask = psVectorAlloc(numRows, PS_TYPE_VECTOR_MASK); // mask
    regions->x = x;
    regions->y = y;
    regions->mask = mask;
    psArray *f = psArrayAlloc(numRows); // Array of fringe measurements
    psArray *df = psArrayAlloc(numRows);// Array of errors
    psArray *fringes = NULL; // Array of fringes, to return

    #define READ_FRINGES_VECTOR_ROW(VECTOR, TYPE, DATATYPE, NAME, DESCRIPTION) \
    { \
        VECTOR->data.DATATYPE[i] = psMetadataLookup##TYPE(&mdok, row, NAME); \
        if (!mdok) { \
            psError(PS_ERR_IO, true, "Unable to find " #DESCRIPTION " for row %ld.\n", i); \
            goto READ_FRINGES_DONE; \
        } \
    }

    // Some values may be either a vector or a value --- need to check
    #define READ_FRINGES_ARRAY_ROW(ARRAY, TYPE, NAME, DESCRIPTION) \
    { \
        psMetadataItem *item = psMetadataLookup(row, NAME); \
        if (!item) { \
            psError(PS_ERR_IO, true, "Unable to find " #DESCRIPTION " for row %ld.\n", i); \
            goto READ_FRINGES_DONE; \
        } \
        if (item->type == PS_DATA_VECTOR) { \
            ARRAY->data[i] = psMemIncrRefCounter(item->data.V); \
        } else if (item->type == PS_DATA_##TYPE) { \
            psVector *vector = psVectorAlloc(1, PS_TYPE_##TYPE); \
            vector->data.TYPE[0] = item->data.TYPE; \
            ARRAY->data[i] = vector; \
        } else { \
            psError(PS_ERR_IO, true, "Found " #DESCRIPTION " for row %ld, but it's of an " \
                    "unsupported type (%x).\n", i, item->type); \
            goto READ_FRINGES_DONE; \
        } \
    }

    // XXX : need to extend this to support arbitrary types for the vectors on disk
    // Translate the table into vectors
    for (long i = 0; i < numRows; i++) {
        psMetadata *row = table->data[i]; // Table row
        READ_FRINGES_VECTOR_ROW(x, F32, F32, "x", "x position");
        READ_FRINGES_VECTOR_ROW(y, F32, F32, "y", "y position");
        READ_FRINGES_VECTOR_ROW(mask, VectorMask, PS_TYPE_VECTOR_MASK_DATA, "mask", "mask");
        READ_FRINGES_ARRAY_ROW(f, F32, "f", "fringe measurement");
        READ_FRINGES_ARRAY_ROW(df, F32, "df", "fringe error");
    }

    // Get f,df into pmFringeStats
    long numFringes = ((psVector*)(f->data[0]))->n; // Number of fringe components
    fringes = psArrayAlloc(numFringes);
    for (int j = 0; j < numFringes; j++) {
        fringes->data[j] = pmFringeStatsAlloc(regions);
    }

    for (long i = 0; i < numRows; i++) {
        psVector *measurements = f->data[i]; // Vector of measurements
        psVector *errors = df->data[i]; // Vector of errors
        for (int j = 0; j < numFringes; j++) {
            pmFringeStats *fringe = fringes->data[j];
            fringe->f->data.F32[i] = measurements->data.F32[j];
            fringe->df->data.F32[i] = errors->data.F32[j];
        }
    }

READ_FRINGES_DONE:
    psFree(regions);
    psFree(f);
    psFree(df);

    return fringes;
}

bool pmFringesFormat(pmCell *cell, psMetadata *inHeader, const psArray *fringes)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_ARRAY_NON_NULL(fringes, false);

    psMetadata *header = psMemIncrRefCounter(inHeader); // Metadata to hold the scalars; will be the header
    if (!inHeader) {
	inHeader = psMetadataAlloc();
    }

    psArray *table = pmFringesFormatTable(header, fringes);
    if (!table) {
	psError (PS_ERR_UNKNOWN, false, "unable to generate table from fringes");
	psFree(header);
	return NULL;
    }

    psMetadataAdd(cell->analysis, PS_LIST_TAIL, "FRINGE.HEADER", PS_DATA_METADATA, "Header for fringe data", header);
    psFree(header);

    psMetadataAdd(cell->analysis, PS_LIST_TAIL, "FRINGE.TABLE", PS_DATA_ARRAY, "Fringe data", table);
    psFree(table);

    return true;
}

psArray *pmFringesParse(pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    bool mdok;                          // Status of MD lookup
    psMetadata *header = psMetadataLookupMetadata(&mdok, cell->analysis, "FRINGE.HEADER"); // Header
    if (!mdok || !header) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find header for fringe data.\n");
        return NULL;
    }

    psArray *table = psMetadataLookupPtr(NULL, cell->analysis, "FRINGE.TABLE"); // FITS table
    if (!table) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find table for fringe data.\n");
        return NULL;
    }

    psArray *fringes = pmFringesParseTable(table, header);
    if (!fringes) {
	psError(PS_ERR_UNKNOWN, false, "Unable to add parse the fringe table data");
	return NULL;
    }

    return fringes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeScale
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void fringeScaleFree(pmFringeScale *scale)
{
    psFree(scale->coeff);
    psFree(scale->coeffErr);
    return;
}

pmFringeScale *pmFringeScaleAlloc(int nFringeFrames)
{
    pmFringeScale *scale = psAlloc(sizeof(pmFringeScale));
    (void)psMemSetDeallocator(scale, (psFreeFunc)fringeScaleFree);

    scale->nFringeFrames = nFringeFrames;
    scale->coeff = psVectorAlloc(nFringeFrames + 1, PS_TYPE_F32);
    scale->coeffErr = psVectorAlloc(nFringeFrames + 1, PS_TYPE_F32);

    return scale;
}

// Determine the fringe scales through solving the least-squares problem
static bool scaleMeasure(pmFringeScale *scale, // Scale to return
                         pmFringeStats *science, // The fringe measurements for the science image
                         psArray *fringes // Array of fringe measurements for the templates
                        )
{
    assert(scale);
    assert(science);
    assert(fringes);
    assert(scale->nFringeFrames == fringes->n);

    psVector *mask = science->regions->mask; // The region mask

    int numCoeffs = fringes->n + 1;     // Number of coefficients: scales for the templates plus a background
    int numPoints = science->regions->nRequested; // Number of points (i.e., fringe measurements)

    psImage *A = psImageAlloc(numCoeffs, numCoeffs, PS_TYPE_F64); // The least-squares matrix
    psVector *B = psVectorAlloc(numCoeffs, PS_TYPE_F64); // The least-squares vector

    // Generate the least-squares matrix and vector
    for (int i = 0; i < numCoeffs; i++) {
        psVector *fringe1 = NULL;       // A fringe measurement
        if (i != 0) {
            pmFringeStats *fringe = fringes->data[i - 1];
            fringe1 = fringe->f;
        }

        // Fill in the upper part of the matrix
        for (int j = i; j < numCoeffs; j++) {
            psVector *fringe2 = NULL;   // Another fringe measurement
            if (j != 0) {
                pmFringeStats *fringe = fringes->data[j - 1];
                fringe2 = fringe->f;
            }

            double matrix = 0.0;        // The matrix sum
            for (int k = 0; k < numPoints; k++) {
                if (!mask->data.PS_TYPE_VECTOR_MASK_DATA[k]) {
                    psF32 f1 = (fringe1) ? fringe1->data.F32[k] : 1.0; // Contribution from i fringe
                    psF32 f2 = (fringe2) ? fringe2->data.F32[k] : 1.0; // Contribution from j fringe
                    psF32 dsInv = science->df->data.F32[k]; // 1 / sigma
                    matrix += f1 * f2 * dsInv * dsInv;
                }
            }
            A->data.F64[i][j] = matrix;
        }

        // Use symmetry to fill in the lower part of the matrix
        for (int j = 0; j < i; j++) {
            A->data.F64[i][j] = A->data.F64[j][i];
        }

        double vector = 0.0;            // The vector sum
        for (int k = 0; k < numPoints; k++) {
            if (!mask->data.PS_TYPE_VECTOR_MASK_DATA[k]) {
                psF32 f1 = (fringe1) ? fringe1->data.F32[k] : 1.0; // Contribution from fringe 1
                psF32 s = science->f->data.F32[k]; // Contribution from science measurement
                psF32 dsInv = science->df->data.F32[k]; // 1 / sigma
                vector += f1 * s * dsInv * dsInv;
            }
        }
	B->data.F64[i] = vector;
    }

    if (psTraceGetLevel("psModules.detrend") >= 5) {
        printf("From %d points:\n", numPoints);
        for (int i = 0; i < numCoeffs; i++) {
            for (int j = 0; j < numCoeffs; j++) {
                printf("%.2e ", A->data.F64[i][j]);
            }
            printf("\n");
        }
    }

    // Solve the least-squares equation
    if (!psMatrixGJSolve(A, B)) {
        psLogMsg("psModules.detrend", PS_LOG_INFO, "Could not solve linear equations.  Returning NULL.\n");
	psFree(A);
	psFree(B);
        return false;
    }

    // Copy the results over
    for (int i = 0; i < numCoeffs; i++) {
        scale->coeff->data.F32[i] = B->data.F64[i];
        scale->coeffErr->data.F32[i] = sqrt(A->data.F64[i][i]);
    }

    psFree(A);
    psFree(B);

    return true;
}

// Measure the fringe differences for each region
static bool fringeScaleDiffs(psVector *diff, // Vector of differences
                             pmFringeStats *science, // Science fringe measurements
                             psArray *fringes, // Template fringe measurements
                             pmFringeScale *scale // Fringe scales
                            )
{
    assert(diff);
    assert(diff->type.type == PS_TYPE_F32);
    assert(science);
    assert(fringes);
    assert(scale);
    assert(diff->n == science->regions->nRequested);
    assert(fringes->n == scale->nFringeFrames);

    psVector *mask = science->regions->mask; // The region mask

    for (int i = 0; i < diff->n; i++) {
        if (!mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            float difference = science->f->data.F32[i] - scale->coeff->data.F32[0];
            for (int j = 0; j < fringes->n; j++) {
                pmFringeStats *fringe = fringes->data[j]; // The fringe of interest
                difference -= scale->coeff->data.F32[j + 1] * fringe->f->data.F32[i];
            }
            diff->data.F32[i] = difference * difference * science->df->data.F32[i] * science->df->data.F32[i];
        }
    }

    return true;
}

// Clip regions based on the differences; return the number masked
static int clipRegions(psVector *diffs, // Differences
                       psVector *mask,  // Region mask
                       float rej        // Rejection limit in standard deviations
                      )
{
    assert(diffs);
    assert(diffs->type.type == PS_TYPE_F32);
    assert(mask);
    assert(mask->type.type == PS_TYPE_VECTOR_MASK);
    assert(diffs->n == mask->n);

    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_QUARTILE); // Statistics
    if (!psVectorStats(stats, diffs, NULL, mask, 1)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return 0;
    }
    float middle = stats->sampleMedian; // The middle of the distribution
    float thresh = rej * 0.74 * (stats->sampleUQ - stats->sampleLQ); // The rejection threshold
    psFree(stats);

    int numClipped = 0;                 // Number clipped
    for (int i = 0; i < diffs->n; i++) {
        psTrace("psModules.detrend", 10, "Region %d (%d): %f\n", i, mask->data.PS_TYPE_VECTOR_MASK_DATA[i], diffs->data.F32[i]);
        if (!mask->data.PS_TYPE_VECTOR_MASK_DATA[i] && fabs(diffs->data.F32[i]) > middle + thresh) {
            psTrace("psModules.detrend", 5, "Masking %d: %f\n", i, diffs->data.F32[i]);
            mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
            numClipped++;
        }
    }

    return numClipped;
}


// XXX include the fringe error (fringe->df) in the fit?
pmFringeScale *pmFringeScaleMeasure(pmFringeStats *science, psArray *fringes, float rej,
                                    unsigned int nIter, float keepFrac)
{
    PS_ASSERT_PTR_NON_NULL(science, NULL);
    PS_ASSERT_PTR_NON_NULL(fringes, NULL);
    PS_ASSERT_INT_POSITIVE(fringes->n, NULL);
    PS_ASSERT_INT_POSITIVE(nIter, NULL);

    pmFringeRegions *regions = science->regions; // The fringe regions
    int numRegions = regions->nRequested; // Number of regions

    // Ensure we are dealing with the SAME fringe points for all the inputs.
    // Otherwise, we're going to get crazy results.
    for (long i = 0; i < numRegions; i++) {
        float xScience = regions->x->data.F32[i]; // The x position for the science image
        float yScience = regions->y->data.F32[i]; // The y position for the science image
        for (long j = 0; j < fringes->n; j++) {
            pmFringeStats *fringe = fringes->data[j]; // The fringe statistics from a fringe image
            pmFringeRegions *fringeRegions = fringe->regions; // The fringe regions for that fringe image
            if (fringeRegions->x->data.F32[i] != xScience ||
                    fringeRegions->y->data.F32[i] != yScience) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Science and fringe measurement regions "
                        "don't match.\n");
                return NULL;
            }
        }
	
    }

    // Set up the mask
    if (!regions->mask) {
        regions->mask = psVectorAlloc(numRegions, PS_TYPE_VECTOR_MASK);
        psVectorInit(regions->mask, 0);
    }
    psVector *mask = regions->mask;     // The region mask
    psStats *median = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN); // Median statistics
    unsigned int numClipped = 0;        // Total number clipped
    psVector *diff = psVectorAlloc(numRegions, PS_TYPE_F32); // The differences between obs. and pred.

    pmFringeScale *scale = pmFringeScaleAlloc(fringes->n); // The fringe scales

    // Get rid of bad data points
    for (int i = 0; i < fringes->n; i++) {
        pmFringeStats *fringe = fringes->data[i]; // The fringe of interest
        for (int j = 0; j < numRegions; j++) {
            if (!isfinite(fringe->f->data.F32[j])) {
                mask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 1;
                psTrace("psModules.detrend", 9, "Masking region %d because not finite in fringe %d.\n", j, i);
            }
	    else if (fabs(fringe->f->data.F32[j]) > 0.1) {
	      mask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 1;
	      psTrace("psModules.detrend", 9, "Masking region %d because too large fringe %d.\n", j, i);
	    }
	    // Mask bad points in the science data as well.
	    if ((i == 0) && (!isfinite(science->f->data.F32[j]))) {
                mask->data.PS_TYPE_VECTOR_MASK_DATA[j] = 1;
                psTrace("psModules.detrend", 9, "Masking region %d because not finite in science fringe %d.\n", j, i);
	    }	      
	    psTrace("psModules.detrend", 7, "F %f %f %f %d\n",
		    fringe->f->data.F32[j], science->f->data.F32[j],
		    1 / science->df->data.F32[j],(int) mask->data.PS_TYPE_VECTOR_MASK_DATA[j]);
        }
    }
    // Allocate array of vectors to hold data.
    psArray *bins = psArrayAlloc(4000);
    for (int j = 0; j < bins->n; j++) {
      bins->data[j] = psVectorAllocEmpty(1,PS_TYPE_F32);
      //      psVector *v = psVectorAllocEmpty(1,PS_TYPE_F32);
      //      bins = psArrayAdd(bins,1,v);
    }

    // Fill vectors
    pmFringeStats *fringe = fringes->data[0];
    for (int j = 0; j < numRegions; j++) {
      if (mask->data.PS_TYPE_VECTOR_MASK_DATA[j] == 0) {
	int array_bin = (int) ((fringe->f->data.F32[j] - -0.1) / 5e-5);
	psVector *bin = bins->data[array_bin];
	psVectorAppend(bin,science->f->data.F32[j]);
      }
    }
    
    psVector *fringe_positions = psVectorAllocEmpty(4000,PS_TYPE_F32);
    psVector *science_values   = psVectorAllocEmpty(4000,PS_TYPE_F32);
    psVector *science_errors   = psVectorAllocEmpty(4000,PS_TYPE_F32);
    psVector *science_counts   = psVectorAllocEmpty(4000,PS_TYPE_S32);

    psStats *binStats = psStatsAlloc(PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
    for (int i = 0; i < 4000; i++) {
      psVector *bin = bins->data[i];
      if (bin->n > 10) {
	psStatsInit(binStats);

	psVectorStats(binStats,bin,NULL,NULL,1);
	
	if (isfinite(binStats->clippedStdev) &&
	    isfinite(binStats->clippedMean) &&
	    (binStats->clippedStdev > 0) &&
	    (binStats->clippedNvalues > 10) &&
	    (binStats->clippedNvalues > 0.5 * bin->n)
	    ) {
	  psVectorAppend(fringe_positions,-0.1 + i * 5e-5);
	  psVectorAppend(science_values, binStats->clippedMean);
	  psVectorAppend(science_errors, binStats->clippedStdev);
	  psVectorAppend(science_counts, bin->n);
	}
      }
      psFree(bins->data[i]);
    }
    psFree(bins);
    psFree(binStats);

    for (int i = 0; i < fringe_positions->n; i++) {
      psTrace("psModules.detrend",7,"FITDATA: %f %f %f %d\n",
	      fringe_positions->data.F32[i],
	      science_values->data.F32[i],
	      science_errors->data.F32[i],
	      science_counts->data.S32[i]);
    }
/*     // Begin switch from old outlier removal and fitting code. */

    psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);

    //    pmFringeStats *fringe = fringes->data[0];
/*     psVector *errors = psVectorAlloc(science->df->n,PS_TYPE_F32); */
/*     for (int j = 0; j < errors->n; j++) { */
/*       errors->data.F32[j] = 1 / science->df->data.F32[j]; */
/*     } */
/*     psVectorFitPolynomial1D(poly,mask,0xff,science->f,errors,fringe->f); */
    psVectorFitPolynomial1D(poly,NULL,0xff,science_values,science_errors,fringe_positions);
    psFree(fringe_positions);
    psFree(science_values);
    psFree(science_errors);
    psFree(science_counts);
    
    for (int i = 0; i <= poly->nX; i++) {
      scale->coeff->data.F32[i] = poly->coeff[i];
      psTrace("psModules.detrend",7,"COEFFS: %d %g %g %g\n",i,scale->coeff->data.F32[i],poly->coeff[i],poly->coeffErr[i]);
    }

    psFree(poly);
    //    psFree(fringe);
    //    psFree(errors);

    psFree(median);
    psFree(diff);
    return scale;
    // End switch from old code.
    

    
# if (0)
    // Write fringe data to file for a test
    FILE *f = fopen ("fringe.dat", "w");
    for (int j = 0; j < numRegions; j++) {
	if (mask->data.PS_TYPE_VECTOR_MASK_DATA[j]) continue;
	fprintf (f, "%d %f %f ", j, science->f->data.F32[j], science->df->data.F32[j]);
	for (int i = 0; i < fringes->n; i++) {
	    pmFringeStats *fringe = fringes->data[i]; // The fringe of interest
            fprintf (f, "%f  ", fringe->f->data.F32[j]);
        }
	fprintf (f, "\n");
    }
    fclose (f);
# endif

    // Get rid of the extreme outliers by assuming most of the points are somewhat clustered
    if (!psVectorStats(median, science->f, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return NULL;
    }
    scale->coeff->data.F32[0] = median->sampleMedian;
    for (int i = 0; i < fringes->n; i++) {
        pmFringeStats *fringe = fringes->data[i]; // The fringe of interest
        if (!psVectorStats(median, fringe->f, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	    return NULL;
	}
        scale->coeff->data.F32[0] -= median->sampleMedian;
	if (i != 0) {
	  scale->coeff->data.F32[i] = 0.0;
	}
    }
    psFree(median);
    fringeScaleDiffs(diff, science, fringes, scale);
    numClipped = clipRegions(diff, mask, 3.0*rej);
    psTrace("psModules.detrend", 4, "%d regions clipped in initial pass.\n", numClipped);

    unsigned int iter = 0;              // Iteration number
    unsigned int iterClip = 0;          // Number clipped in this iteration
    do {
        iter++;
        scaleMeasure(scale, science, fringes); // The scales
        psTrace("psModules.detrend", 1, "Fringe scales after iteration %d:\n", iter);
        psTrace("psModules.detrend", 1, "Background: %f %f\n", scale->coeff->data.F32[0],
                scale->coeffErr->data.F32[0]);
        for (int i = 0; i < scale->nFringeFrames; i++) {
            psTrace("psModules.detrend", 1, "%d: %f %f\n", i, scale->coeff->data.F32[i + 1],
                    scale->coeffErr->data.F32[i + 1]);
        }

        fringeScaleDiffs(diff, science, fringes, scale);
        iterClip = clipRegions(diff, mask, rej); // Number clipped
        numClipped += iterClip;
        psTrace("psModules.detrend", 9, "Clipped: %d\tFrac: %f\n", iterClip,
                (float)numClipped/(float)numRegions);
    } while (iterClip > 0 && iter < nIter && (float)numClipped/(float)numRegions <= 1.0 - keepFrac);
    psFree(diff);

    // A final iteration with the last clipping
    scaleMeasure(scale, science, fringes);

    return scale;
    //# endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fringe correction
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// XXX note that this modifies the input fringe images
psImage *pmFringeCorrect(pmReadout *readout, pmFringeRegions *fringes, psArray *fringeImages,
                         psArray *fringeStats, psImageMaskType maskVal, float rej,
                         unsigned int nIter, float keepFrac)
{
    PS_ASSERT_PTR_NON_NULL(readout, NULL);
    PS_ASSERT_PTR_NON_NULL(readout->image, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(readout->image, NULL);
    PS_ASSERT_PTR_NON_NULL(fringes, NULL);
    PS_ASSERT_PTR_NON_NULL(fringeImages, NULL);
    PS_ASSERT_PTR_NON_NULL(fringeStats, NULL);
    PS_ASSERT_INT_EQUAL(fringeImages->n, fringeStats->n, NULL);
    PS_ASSERT_INT_POSITIVE(nIter, NULL);

    // measure the fringe stats for the science frame and solve for the scales
    pmFringeStats *scienceStats = pmFringeStatsMeasure(fringes, readout, maskVal);

    if (psTraceGetLevel("psModules.detrend") > 9) {
        for (int i = 0; i < fringes->nRequested; i++) {
            printf("%f", scienceStats->f->data.F32[i]);
            for (int j = 0; j < fringeStats->n; j++) {
                pmFringeStats *fringe = fringeStats->data[j];
                printf("\t%f", fringe->f->data.F32[i]);
            }
            printf("\n");
        }
    }

    pmFringeScale *scale = pmFringeScaleMeasure(scienceStats, fringeStats, rej, nIter, keepFrac);
    psFree(scienceStats);

    psTrace("psModules.detrend", 7, "Fringe solution:\n");
    for (int i = 0; i < fringeImages->n + 1; i++) {
        psTrace("psModules.detrend", 7, "%d: %f %f\n", i, scale->coeff->data.F32[i],
                scale->coeffErr->data.F32[i]);
    }

    // build the fringe correction image
    // XXX we could save data space by making the first image the output image
    psImage *sumFringe = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
    //psBinaryOp(sumFringe, sumFringe, "+", psScalarAlloc(scale->coeff->data.F32[0], PS_TYPE_F32));
    for (int i = 0; i < fringeImages->n; i++) {

        // rescale the fringe image
        psBinaryOp(fringeImages->data[i], fringeImages->data[i], "*",
                   psScalarAlloc(scale->coeff->data.F32[i+1], PS_TYPE_F32));

        // sum together
        sumFringe = (psImage*)psBinaryOp(sumFringe, sumFringe, "+", fringeImages->data[i]);
    }
    psFree(scale);

    // subtract the resulting fringe frame
    readout->image = (psImage*)psBinaryOp(readout->image, readout->image, "-", sumFringe);

    return sumFringe;
}

