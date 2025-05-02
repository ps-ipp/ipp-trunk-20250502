#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <assert.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmConfigMask.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAFlags.h"
#include "pmHDUUtils.h"
#include "pmConceptsRead.h"
#include "pmFPAHeader.h"

#include "pmFPARead.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Specify what to read
typedef enum {
    FPA_READ_TYPE_IMAGE,                // Read image
    FPA_READ_TYPE_MASK,                 // Read mask
    FPA_READ_TYPE_VARIANCE,             // Read variance map
    FPA_READ_TYPE_HEADER                // Read header
} fpaReadType;

// Desired type for pixels; the index corresponds to the fpaReadType, above.
static psElemType pixelTypes[] = {
    PS_TYPE_F32,
    PS_TYPE_IMAGE_MASK,
    PS_TYPE_F32,
    0
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Get the "thisXXXScan" value in the readout for the appropriate image type
static int readoutGetThisScan(pmReadout *readout, // Readout of interest
                              fpaReadType type // Type of image
    )
{
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        return readout->thisImageScan;
      case FPA_READ_TYPE_MASK:
        return readout->thisMaskScan;
      case FPA_READ_TYPE_VARIANCE:
        return readout->thisVarianceScan;
      default:
        psAbort("Unknown read type: %x\n", type);
    }
}

// Set the "thisXXXScan" value in the readout for the appropriate image type
static void readoutSetThisScan(pmReadout *readout, // Readout of interest
                              fpaReadType type, // Type of image
                              int thisScan // Starting scan number
    )
{
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        readout->thisImageScan = thisScan;
        return;
      case FPA_READ_TYPE_MASK:
        readout->thisMaskScan = thisScan;
        return;
      case FPA_READ_TYPE_VARIANCE:
        readout->thisVarianceScan = thisScan;
        return;
      default:
        psAbort("Unknown read type: %x\n", type);
    }
}

// Get the "lastXXXScan" value in the readout for the appropriate image type
static int readoutGetLastScan(pmReadout *readout, // Readout of interest
                              fpaReadType type // Type of image
    )
{
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        return readout->lastImageScan;
      case FPA_READ_TYPE_MASK:
        return readout->lastMaskScan;
      case FPA_READ_TYPE_VARIANCE:
        return readout->lastVarianceScan;
      default:
        psAbort("Unknown read type: %x\n", type);
    }
}

// Set the "lastXXXScan" value in the readout for the appropriate image type
static void readoutSetLastScan(pmReadout *readout, // Readout of interest
                              fpaReadType type, // Type of image
                              int lastScan // Last scan number
    )
{
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        readout->lastImageScan = lastScan;
        return;
      case FPA_READ_TYPE_MASK:
        readout->lastMaskScan = lastScan;
        return;
      case FPA_READ_TYPE_VARIANCE:
        readout->lastVarianceScan = lastScan;
        return;
      default:
        psAbort("Unknown read type: %x\n", type);
    }
}

// Return pointer to appropriate image
static psImage **readoutImageByType(pmReadout *readout, // Readout of interest
                                    fpaReadType type // Type of image
    )
{
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        return &readout->image;
      case FPA_READ_TYPE_MASK:
        return &readout->mask;
      case FPA_READ_TYPE_VARIANCE:
        return &readout->variance;
      default:
        psAbort("Unknown read type: %x\n", type);
    }
}

// Determine number of readouts in the FITS file
// In the process, reads the header and concepts
static int cellNumReadouts(pmCell *cell,    // Cell of interest
                            psFits *fits,    // FITS file
                            pmConfig *config // Configuration
    )
{
    assert(cell);
    assert(fits);

    // Get the HDU and read the header
    pmHDU *hdu = pmHDUFromCell(cell);   // The HDU
    if (!hdu || hdu->blankPHU) {
        psError(PS_ERR_IO, true, "Unable to find HDU");
        return 0;
    }
    if (!pmCellReadHeader(cell, fits, config)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell!\n");
        return 0;
    }
    if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_CELLS, true, NULL)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for cell.\n");
        return 0;
    }

    // Get the size of the third dimension
    bool mdok;                          // Status of MD lookup
    int naxis = psMetadataLookupS32(&mdok, hdu->header, "NAXIS"); // The number of axes
    if (!mdok) {
        psError(PS_ERR_IO, true, "Unable to find NAXIS in header for extension %s\n", hdu->extname);
        return 0;
    }

    if (naxis == 0) {
        // No pixels to read
        psError(PS_ERR_IO, true, "No pixels in extension %s.", hdu->extname);
        return 0;
    }
    if (naxis < 2 || naxis > 3) {
        psError(PS_ERR_IO, true, "NAXIS in header of extension %s (= %d) is not valid.\n",
                hdu->extname, naxis);
        return 0;
    }
    int naxis3;                     // Number of image planes
    if (naxis == 3) {
        naxis3 = psMetadataLookupS32(&mdok, hdu->header, "NAXIS3");
        if (!mdok) {
            psError(PS_ERR_IO, true, "Unable to find NAXIS3 in header for extension %s\n", hdu->extname);
            return 0;
        }
    } else {
        naxis3 = 1;
    }

    return naxis3;
}

// Determine whether a FITS file contains covariance matrices
static bool hduCovariance(pmHDU *hdu,   // Header data unit
                          psFits *fits  // FITS file
    )
{
    if (hdu->extname && !psFitsMoveExtName(fits, hdu->extname)) {
        psError(PS_ERR_IO, false, "Unable to move to extension %s", hdu->extname);
        return false;
    }
    // Need to explicitly read the header, since the HDU may not contain the variance header
    psMetadata *header = psFitsReadHeader(NULL, fits); // Header
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read variance header.");
        return false;
    }
    bool mdok;                          // Status of MD lookup
    bool covar = psMetadataLookupBool(&mdok, header, PM_HDU_COVARIANCE_KEYWORD); // Got covariance?
    psFree(header);
    return covar;
}

// Does the current readout, with scans set for a new read, represent any real data, or is it beyond the end?
// Requires that cellNumReadouts() has been called before (for header and concepts to have been read)
// In the process, adjusts the TRIMSEC
static bool readoutHaveMoreScans(int *start, // Start of scan
                                 int *last, // Last possible scan (defined by TRIMSEC)
                                 pmReadout *readout, // Readout of interest
                                 int numScans, // Number of scans to read at a time
                                 fpaReadType type, // Type of image
                                 pmConfig *config // Configuration
                                 )
{
    assert(start);
    assert(last);
    assert(readout);

    if (!pmConceptsReadCell(readout->parent, PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_DATABASE,
                            true, config)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for cell.");
        return false;
    }
    // Header and concepts have been read by a call to cellNumReadouts(), so we can just assume they're there.

    // Get the trim and bias sections
    pmCell *cell = readout->parent;     // Parent cell
    PS_ASSERT_PTR_NON_NULL(cell, false);
    pmHDU *hdu = pmHDUFromCell(cell);   // HDU for data

    bool mdok = true;                   // Status of MD lookup
    psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim sections
    if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
        psError(PS_ERR_IO, true, "CELL.TRIMSEC is not set.\n");
        return false;
    }
    int readdir = psMetadataLookupS32(&mdok, cell->concepts, "CELL.READDIR"); // Read direction
    if (!mdok || readdir == 0 || (readdir != 1 && readdir != 2)) {
        psError(PS_ERR_IO, true, "CELL.READDIR is not set to 1 or 2.\n");
        return false;
    }

    // Rationalize trimsec against naxis1, naxis2:  valid range for trimsec is 1-Nx,1-Ny
    if (trimsec->x1 < 1) {
        int naxis1 = psMetadataLookupS32(&mdok, hdu->header, "NAXIS1"); // The number of columns
        if (!mdok) {
            psError(PS_ERR_IO, true, "Unable to find NAXIS1 in header for extension %s\n", hdu->extname);
            return false;
        }
        trimsec->x1 = naxis1 + trimsec->x1;
    }
    if (trimsec->y1 < 1) {
        int naxis2 = psMetadataLookupS32(&mdok, hdu->header, "NAXIS2"); // The number of columns
        if (!mdok) {
            psError(PS_ERR_IO, true, "Unable to find NAXIS2 in header for extension %s\n", hdu->extname);
            return false;
        }
        trimsec->y1 = naxis2 + trimsec->y1;
    }

    *last = (readdir == 1) ? trimsec->y1 : trimsec->x1; // Maximum possible scan number

    // Calculate the segment offset and upper limit
    if (numScans == 0) {
        // Read entire image.  In that case, we never call this funtion unless the data has not yet been read.
        // thus, only if the delta is should we return false (ie, trimsec defines an empty region)
        *start = (readdir == 1) ? trimsec->y0 : trimsec->x0;
    } else if (readout->forceScan) {
        // We're forced to read what we're told
        *start = readoutGetThisScan(readout, type);
    } else {
        // Progressive scans
        psImage *image = *readoutImageByType(readout, type); // Appropriate image from readout
        *start = image ? readoutGetLastScan(readout, type) : 0;
    }

    return true;
}

static bool readoutMore(pmReadout *readout, // Readout of interest
                        psFits *fits,    // FITS file
                        int z,          // Plane number to read
                        int *zMax,      // Max plane number in this cell
                        int numScans,   // Number of scans to read at a time
                        fpaReadType type, // Type of image
                        pmConfig *config// Configuration
    )
{
    assert(readout);
    assert(fits);

    psImage *image = *readoutImageByType(readout, type);

    // XXX this may not be the valid test in a multithread environment. consider a fileGroup of
    // N readouts, but numScans set to 0.  only the first should report that it requires data,
    // even if all readouts lack the image pointer.
    if (numScans == 0) {
      if (!image) {
        return true;
      } else {
        return false;
      }
    }

    pmCell *cell = readout->parent;     // Parent cell
    if (!cell) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find parent cell.");
        return false;
    }
    *zMax = cellNumReadouts(cell, fits, config); // Number of planes
    if (z >= *zMax) {
        // No more to read
        return false;
    }

    int start, last;                    // Start and last scans
    if (!readoutHaveMoreScans(&start, &last, readout, numScans, type, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine readout properties.");
        return false;
    }

    return start < last;
}

// Carve a readout from the image pixels
static bool readoutCarve(pmReadout *readout, // Readout to be carved up
                         psImage *image, // Image that will be carved
                         const psRegion *trimsec, // Trim section
                         const psList *biassecs, // Bias sections
                         fpaReadType type // Type of image
                        )
{
    assert(readout);
    assert(image);
    assert(trimsec);
    assert(biassecs);

    // The image corresponding to the trim region
    if (psRegionIsNaN(*trimsec)) {
        psString regionString = psRegionToString(*trimsec);
        psError(PS_ERR_UNKNOWN, true, "Invalid trim section: %s\n", regionString);
        psFree(regionString);
        psFree(readout);
        return false;
    }
    psRegion region = psRegionSet(PS_MAX(trimsec->x0 - readout->col0, 0), // x0
                                  PS_MIN(trimsec->x1 - readout->col0, image->numCols), // x1
                                  PS_MAX(trimsec->y0 - readout->row0, 0), // y0
                                  PS_MIN(trimsec->y1 - readout->row0, image->numRows) // y1
                                 );

    // Place the image subset in the appropriate target location, freeing if needed
    psImage **target = readoutImageByType(readout, type); // Target image
    if (*target) {
        psFree(*target);
    }
    *target = psImageSubset(image, region);

    // Get the list of overscans: only for IMAGE types (no overscan for MASK and VARIANCE)
    if (type == FPA_READ_TYPE_IMAGE) {
        if (readout->bias->n != 0) {
            // Make way!
            psFree(readout->bias);
            readout->bias = psListAlloc(NULL);
        }
        psListIterator *iter = psListIteratorAlloc((psList*)biassecs, PS_LIST_HEAD, false); // Iterator
        psRegion *biassec = NULL;       // A BIASSEC region from the list
        while ((biassec = psListGetAndIncrement(iter))) {
            if (psRegionIsNaN(*biassec)) {
                psString regionString = psRegionToString(*biassec);
                psError(PS_ERR_IO, true, "Invalid bias section: %s\n", regionString);
                psFree(regionString);
                psFree(readout);
                psFree(iter);
                return false;
            }
            psRegion region = psRegionSet(PS_MAX(biassec->x0 - readout->col0, 0), // x0
                                          PS_MIN(biassec->x1 - readout->col0, image->numCols), // x1
                                          PS_MAX(biassec->y0 - readout->row0, 0), // y0
                                          PS_MIN(biassec->y1 - readout->row0, image->numRows) // y1
                );
            psImage *overscan = psImageSubset(image, region);
            psListAdd(readout->bias, PS_LIST_TAIL, overscan);
            psFree(overscan);
        }
        psFree(iter);
    }

    return true;
}

// Read a component of a readout.  We read in only the rows from min to max for plane z, for
// the full region requested.  if we request a range outside the region, we will pad to fill
// out the edges of the region with 'bad' pixels.  The output image always has max-min rows.
// The region represents the maximum bounds of the full image
static psImage *readoutReadComponent(psImage *image, // Image into which to read
                                     psFits *fits, // FITS file from which to read
                                     const psRegion *fullImage, // full image region, read a subset
                                     int readdir, // Read direction (1=rows, 2=cols)
                                     int min,  // Minimum row/col number to read
                                     int max,   // Maximum row/col number to read
                                     int z,     // Image plane to read
                                     float bad, // Bad value
                                     psElemType type // Expected type for image
    )
{
    assert(fits);
    assert(fullImage);
    assert((readdir == 1) || (readdir == 2));

    int nRead = 0;                      // Number of scans read
    int nScans = max - min;             // Number of scans desired
    assert(nScans > 0);

    psRegion toRead = *fullImage;  // full image region

    int dX = 0, dY = 0;                 // Offset from image in FITS file to lower left corner of what's read
    int nX = 0, nY = 0;                 // Size of region to read

    if (readdir == 1) {
        toRead.y0 = PS_MAX(toRead.y0, min);
        toRead.y1 = PS_MIN(toRead.y1, max);
        nRead = toRead.y1 - toRead.y0;
        if (min < fullImage->y0) {
            dY = toRead.y0;
        }
        nX = toRead.x1 - toRead.x0;
        nY = nScans;
    } else {
        toRead.x0 = PS_MAX(toRead.x0, min);
        toRead.x1 = PS_MIN(toRead.x1, max);
        nRead = toRead.x1 - toRead.x0;
        if (min < fullImage->x0) {
            dX = toRead.x0;
        }
        nX = nScans;
        nY = toRead.y1 - toRead.y0;
    }

    psTrace("psModules.camera", 5, "Reading section [%.0f:%.0f,%.0f:%.0f]\n",
            toRead.x0, toRead.x1, toRead.y0, toRead.y1);
    image = psFitsReadImageBuffer(image, fits, toRead, z); // Desired pixels
    psTrace("psModules.camera", 7, "Image is %dx%d\n", image->numCols, image->numRows);

    // Ensure the pixel type corresponds to what we desire
    if (image->type.type != type) {
        psImage *temp = psImageCopy(NULL, image, type);
        psFree(image);
        image = temp;
    }

    // Resize the image so that it matches the number of scans requested
    // XXX this modification is not carried back up stream: it affects readout->row0,col0
    //
    // XXXXX Do we really want to do this???  Why???
    if (nRead < nScans) {
        // The region of interest is smaller than the number of pixels we want.
        psTrace("psModules.camera", 5, "Resizing image to %d,%d\n", nX, nY);
        psImage *temp = psImageAlloc(nX, nY, image->type.type);
        psImageInit(temp, bad);
        psImageOverlaySection(temp, image, dX, dY, "=");
        psFree(image);
        image = temp;
    }

    return image;
}

// Read a chunk of a readout (or the whole lot)
static bool readoutReadChunk(pmReadout *readout, // Readout into which to read
                             psFits *fits, // FITS file
                             int z,     // Desired image plane
                             int *zMax, // Max plane number in this cell
                             int numScans, // Number of scans (row or col depends on CELL.READDIR); 0 for all
                             int overlap, // Number of scans (row/col) to overlap between scans
                             fpaReadType type, // Type of image
                             pmConfig *config   // Configuration
    )
{
    assert(readout);
    assert(fits);
    assert(z >= 0);
    assert(numScans >= 0);
    assert(overlap >= 0);

    psImage **image = readoutImageByType(readout, type); // Pointer to the image of interest
    if (*image && numScans == 0) {
        psError(PS_ERR_UNKNOWN, true, "Already read entire image --- won't clobber.");
        return false;
    }

    pmCell *cell = readout->parent;     // The parent cell
    if (!cell) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find parent cell.");
        return false;
    }

    int naxis3 = cellNumReadouts(cell, fits, config); // Number of image planes
    if (zMax) *zMax = naxis3;

    if (z >= naxis3) {
        psError(PS_ERR_IO, false, "Desired image plane (%d) exceeds available number (%d).",
                z, naxis3);
        return false;
    }

    int thisScan;                       // Starting scan for this read
    int maxScan;                        // Maximum scan number
    if (!readoutHaveMoreScans(&thisScan, &maxScan, readout, numScans, type, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine readout properties.");
        return false;
    }
    if (thisScan >= maxScan) {
        psError(PS_ERR_IO, true, "No more of the readout to read.");
        return false;
    }

    pmHDU *hdu = pmHDUFromCell(cell);   // The HDU
    assert(hdu && !hdu->blankPHU);      // Checked by cellNumReadouts()

    bool mdok;                          // Status of MD lookup
    int readdir = psMetadataLookupS32(&mdok, cell->concepts, "CELL.READDIR"); // Read direction
    if (!mdok || readdir == 0 || (readdir != 1 && readdir != 2)) {
        psError(PS_ERR_IO, true, "CELL.READDIR is not set to -1 or +1.\n");
        return false;
    }

    // Need to set the invalid (unread) pixels appropriately, and to read the mask bits
    float bad = 0;                      // Bad level
    switch (type) {
      case FPA_READ_TYPE_MASK: {
          // Need to explicitly read the header, since what's in the pmHDU may not correspond to the mask
          psMetadata *header = psFitsReadHeader(NULL, fits);
          if (!header) {
              psError(PS_ERR_IO, false, "Unable to read mask header.");
              return false;
          }
          if (!pmConfigMaskReadHeader(config, header)) {
              psError(PS_ERR_IO, false, "Unable to determine mask bits");
              psFree(header);
              return false;
          }
          psFree(header);
          bad = pmConfigMaskGet("LOW", config);
          if (!bad) {
              // XXX look up old name for compatability
              bad = pmConfigMaskGet("BAD", config);
          }
          break;
      }
      case FPA_READ_TYPE_IMAGE:
      case FPA_READ_TYPE_VARIANCE:
        bad = psMetadataLookupF32(&mdok, cell->concepts, "CELL.BAD");
        break;
      default:
        psAbort("Unrecognised type: %x", type);
    }

    psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim sections
    if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
        psError(PS_ERR_IO, true, "CELL.TRIMSEC is not set.\n");
        return false;
    }

    // Check the third dimension
    int naxis = psMetadataLookupS32(&mdok, hdu->header, "NAXIS"); // The number of axes
    if (!mdok) {
        psError(PS_ERR_IO, true, "Unable to find NAXIS in header for extension %s\n", hdu->extname);
        return false;
    }
    if (naxis == 0) {
        // No pixels to read
        psError(PS_ERR_IO, true, "No pixels in extension %s.", hdu->extname);
        return false;
    }
    if (naxis < 2 || naxis > 3) {
        psError(PS_ERR_IO, true, "NAXIS in header of extension %s (= %d) is not valid.\n",
                hdu->extname, naxis);
        return false;
    }

    int origThisScan = thisScan;        // Original value of thisScan (starting point for read)
    if (thisScan == 0) {
        overlap = 0;
    }
    thisScan -= overlap;
    if (thisScan < 0) {
        thisScan = 0;
    }

    // Calculate limits, adjust readout->row0,col0
    // XXX Should row0,col0 be adjusted, since they are used for astrometry???
    if (readdir == 1) {
        // Reading rows
        readout->row0 = thisScan;
        readout->col0 = trimsec->x0;
        if (numScans == 0) {
            numScans = trimsec->y1 - trimsec->y0;
        }
    } else {
        // Reading cols
        readout->col0 = thisScan;
        readout->row0 = trimsec->y0;
        if (numScans == 0) {
            numScans = trimsec->x1 - trimsec->x0;
        }
    }
    int lastScan = origThisScan + numScans; // Last scan to read

    readoutSetThisScan(readout, type, thisScan);
    readoutSetLastScan(readout, type, lastScan);

    // Blow away existing data.
    // Do this before returning, so that we're not returning data from a previous read
//    psFree(*image);
//    *image = NULL;
    *image = readoutReadComponent(*image, fits, trimsec, readdir, thisScan, lastScan, z, bad, pixelTypes[type]);

    // Read overscans only for "image" type --- variances and masks shouldn't record overscans
    if (type == FPA_READ_TYPE_IMAGE) {
        // Blow away existing data
        while (readout->bias->n > 0) {
            psListRemove(readout->bias, PS_LIST_HEAD);
        }

        // Get the new bias sections
        psList *biassecs = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.BIASSEC"); // Bias sections
        if (!mdok || !biassecs) {
            psError(PS_ERR_IO, true, "CELL.BIASSEC is not set.\n");
            return false;
        }
        psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, false); // Iterator
        psRegion *biassec = NULL;           // Bias section from iteration
        while ((biassec = psListGetAndIncrement(biassecsIter))) {
            psImage *bias = readoutReadComponent(NULL, fits, biassec, readdir, thisScan, lastScan, z, bad, pixelTypes[type]); // The bias
            psListAdd(readout->bias, PS_LIST_TAIL, bias);
            psFree(bias);                   // Drop reference
        }
        psFree(biassecsIter);
    }

    return true;
}

// Read into an cell; this is the engine for pmCellRead, pmCellReadMask, pmCellReadVariance
// Does most of the work for the reading --- reads the HDU, and portions the HDU into readouts.
static bool cellRead(pmCell *cell,      // Cell into which to read
                     psFits *fits,      // FITS file from which to read
                     pmConfig *config,  // Configuration
                     fpaReadType type   // Type to read
                    )
{
    assert(cell);
    assert(fits);

    pmHDU *hdu = pmHDUFromCell(cell);   // The HDU
    if (!hdu) {
        return true;                    // We read everything we could
    }

    // check if we have read the desired data, read it if needed
    bool (*hduReadFunc)(pmHDU*, psFits*) = NULL; // Function to use to read the HDU
    void *dataPointer = NULL;           // pointer to location of desired data
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        hduReadFunc = pmHDURead;
        dataPointer = hdu->images;
        break;
      case FPA_READ_TYPE_HEADER:
        hduReadFunc = pmHDUReadHeader;
        dataPointer = hdu->header;
        break;
      case FPA_READ_TYPE_MASK:
        hduReadFunc = pmHDUReadMask;
        dataPointer = hdu->masks;
        break;
      case FPA_READ_TYPE_VARIANCE:
        hduReadFunc = pmHDUReadVariance;
        dataPointer = hdu->variances;
        break;
      default:
        psAbort("Unknown read type: %x\n", type);
    }

    // do we have the data we want (image, header, or etc).
    if (!dataPointer) {
        // attempt to read in the desired data
        if (!hduReadFunc(hdu, fits)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to read HDU for cell.\n");
            return false;
        }
    }

    // load in the concept information for this cell
    if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_DATABASE, true, config)) {
        //psError(PS_ERR_UNKNOWN, false, "Failed to read concepts for cell");
        //return false;
        psWarning("Difficulty reading concepts for cell; attempting to proceed.");
    }

    // skip the image arrays completely for the header-only files
    if (type == FPA_READ_TYPE_HEADER) {
        pmCellSetDataStatus(cell, true);
        return true;
    }

    // set up pointers for the different possible image arrays
    psArray *imageArray = NULL; // Array of images in the HDU
    psElemType imageType = pixelTypes[type]; // Expected type for image
    switch (type) {
      case FPA_READ_TYPE_IMAGE:
        imageArray = hdu->images;
        break;
      case FPA_READ_TYPE_MASK:
        imageArray = hdu->masks;
        break;
      case FPA_READ_TYPE_VARIANCE:
        imageArray = hdu->variances;
        break;
      default:
        psAbort("Unknown read type: %x\n", type);
    }

    // Having read the cell, we now have to cut it up
    psRegion *trimsec = psMetadataLookupPtr(NULL, cell->concepts, "CELL.TRIMSEC");
    psList *biassecs = psMetadataLookupPtr(NULL, cell->concepts, "CELL.BIASSEC");
    if (psRegionIsNaN(*trimsec)) {
        psError(PS_ERR_IO, false, "CELL.TRIMSEC is not set --- can't read cell.\n");
        return false;
    }

    // Iterate over each of the image planes, converting type if necessary, and extracting the bits that
    // matter (CELL.TRIMSEC, CELL.BIASSEC) into readouts with readoutCarve.
    for (int i = 0; i < imageArray->n; i++) {
        psImage *source = imageArray->data[i]; // Source image, from the i-th plane
        PS_ASSERT_IMAGE_NON_NULL(source, false);

        // Type conversion here to support the modules, which don't have multiple type support yet
        if (source->type.type != imageType) {
            psImage *temp = psImageCopy(NULL, source, imageType); // Temporary image
            psFree(imageArray->data[i]);
            imageArray->data[i] = temp;
            source = temp;
        }

        pmReadout *readout;             // Readout into which to read
        if (cell->readouts->n > i && cell->readouts->data[i]) {
            readout = psMemIncrRefCounter(cell->readouts->data[i]);
        } else {
            readout = pmReadoutAlloc(cell);
        }

        if (!readoutCarve(readout, source, trimsec, biassecs, type)) {
            psError(PS_ERR_UNEXPECTED_NULL, false,
                    "Unable to carve readout into image and bias sections for %d the plane.", i);
            return NULL;
        }
        psFree(readout);                // Drop reference
    }

    pmCellSetDataStatus(cell, true);
    return true;
}


// Read into an chip; this is the engine for pmChipRead, pmChipReadMask, pmChipReadVariance
// Iterates over component cells, reading each
static bool chipRead(pmChip *chip,      // Chip into which to read
                     psFits *fits,      // FITS file from which to read
                     pmConfig *config,  // Configuration
                     fpaReadType type   // Type to read
                    )
{
    assert(chip);
    assert(fits);

    bool success = false;               // Were we able to read at least one HDU?
    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // The cell of interest
        success |= cellRead(cell, fits, config, type);
    }
    if (success) {
        if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_DATABASE,
                                true, true, NULL)) {
            psError(PS_ERR_IO, false, "Failed to read concepts for chip.\n");
            return false;
        }
        // XXX probably could just use chip->data_exists
        pmChipSetDataStatus(chip, true);
    }

    return success;
}


// Read into an FPA; this is the engine for pmFPARead, pmFPAReadMask, pmFPAReadVariance
// Iterates over component chips, reading each
static bool fpaRead(pmFPA *fpa,         // FPA into which to read
                    psFits *fits,       // FITS file from which to read
                    pmConfig *config,   // Configuration
                    fpaReadType type    // Type to read
                   )
{
    assert(fpa);
    assert(fits);

    bool success = false;               // Were we able to read at least one HDU?
    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // The cell of interest
        success |= chipRead(chip, fits, config, type);
    }
    if (success) {
        if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_DATABASE, true, NULL)) {
            psError(PS_ERR_IO, false, "Failed to read concepts for FPA.\n");
            return false;
        }
    } else {
        psError(PS_ERR_UNKNOWN, false, "Unable to read any chips in FPA");
    }

    return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading images
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// pmReadoutReadNext is maintained here (for now) to maintain backwards compatibility.
// pmReadoutReadNext has been replaced by pmReadoutRead, pmReadoutReadChunk, pmReadoutMore
bool pmReadoutReadNext(bool *status, pmReadout *readout, psFits *fits, int z, int numScans, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_INT_NONNEGATIVE(z, false);
    PS_ASSERT_INT_NONNEGATIVE(numScans, false);

    assert (numScans || !readout->image); // cannot have readout->image and !numScans

    *status = false;

    // Get the HDU and read the header
    pmCell *cell = readout->parent;     // The parent cell

    pmHDU *hdu = pmHDUFromCell(cell);   // The HDU
    if (!hdu || hdu->blankPHU) {
        // XXX is this an error condition?
        *status = true;
        return false;
    }

    if (!pmCellReadHeader(cell, fits, config)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell!\n");
        return false;
    }

    // Make sure we have the information we need
    if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER | PM_CONCEPT_SOURCE_CELLS |
                            PM_CONCEPT_SOURCE_DEFAULTS | PM_CONCEPT_SOURCE_DATABASE, true, NULL)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for cell.\n");
        return false;
    }

    // Get the trim and bias sections
    bool mdok = true;                   // Status of MD lookup
    psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim sections
    if (!mdok || !trimsec || psRegionIsNaN(*trimsec)) {
        psError(PS_ERR_IO, true, "CELL.TRIMSEC is not set.\n");
        return false;
    }
    psList *biassecs = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.BIASSEC"); // Bias sections
    if (!mdok || !biassecs) {
        psError(PS_ERR_IO, true, "CELL.BIASSEC is not set.\n");
        return false;
    }
    int readdir = psMetadataLookupS32(&mdok, cell->concepts, "CELL.READDIR"); // Read direction
    if (!mdok || readdir == 0 || (readdir != 1 && readdir != 2)) {
        psError(PS_ERR_IO, true, "CELL.READDIR is not set to -1 or +1.\n");
        return false;
    }
    float bad = psMetadataLookupF32(&mdok, cell->concepts, "CELL.BAD"); // Bad level
    if (!mdok) {
        psWarning("CELL.BAD is not set --- assuming zero.\n");
        bad = 0.0;
    }

    // Check the third dimension
    int naxis = psMetadataLookupS32(&mdok, hdu->header, "NAXIS"); // The number of axes
    if (!mdok) {
        psError(PS_ERR_IO, true, "Unable to find NAXIS in header for extension %s\n", hdu->extname);
        return false;
    }
    if (naxis == 0) {
        // No pixels to read, as for a PHU.
        *status = true;
        return false;
    }
    if (naxis < 2 || naxis > 3) {
        psError(PS_ERR_IO, true, "NAXIS in header of extension %s (= %d) is not valid.\n",
                hdu->extname, naxis);
        return false;
    }
    int naxis3 = 1;                     // The number of image planes
    if (naxis == 3) {
        naxis3 = psMetadataLookupS32(&mdok, hdu->header, "NAXIS3");
        if (!mdok) {
            psError(PS_ERR_IO, true, "Unable to find NAXIS3 in header for extension %s\n", hdu->extname);
            return false;
        }
    }
    if (z >= naxis3) {
        // Nothing to see here.  Move along.
        *status = true;
        return false;
    }

    // Get the size of the image plane
    int naxis1 = psMetadataLookupS32(&mdok, hdu->header, "NAXIS1"); // The number of columns
    if (!mdok) {
        psError(PS_ERR_IO, true, "Unable to find NAXIS1 in header for extension %s\n", hdu->extname);
        return false;
    }
    int naxis2 = psMetadataLookupS32(&mdok, hdu->header, "NAXIS2"); // The number of columns
    if (!mdok) {
        psError(PS_ERR_IO, true, "Unable to find NAXIS2 in header for extension %s\n", hdu->extname);
        return false;
    }

    // rationalize trimsec against naxis1, naxis2
    // valid range for trimsec is 1-Nx,1-Ny
    // if (trimsec->x0 == 0) trimsec->x0 = 1;
    if (trimsec->x1 <  1)
        trimsec->x1 = naxis1 + trimsec->x1;
    // if (trimsec->y0 == 0) trimsec->y0 = 1;
    if (trimsec->y1 <  1)
        trimsec->y1 = naxis2 + trimsec->y1;

    // XX not used int maxSize;                        // Number of cols,rows in image
    // XX not used if (readdir == 1) {
    // XX not used     maxSize = PS_MIN(naxis2, trimsec->y1 - trimsec->y0);
    // XX not used } else {
    // XX not used     maxSize = PS_MIN(naxis1, trimsec->x1 - trimsec->x0);
    // XX not used }

    int offset;                         // start of the segment
    int upper;                          // end of the segment
    int lastScan;                       // last possible scan

    // Calculate the segment offset and upper limit, adjust readout->row0,col0
    if (readdir == 1) {
        // Reading rows
        offset = (readout->image) ? readout->row0 + numScans : 0; // extend to next section or start at beginning?
        offset = (numScans == 0) ? trimsec->x0 : offset; // full array ? read full trimsec : read section
        readout->row0 = offset;
        readout->col0 = trimsec->x0;
        lastScan = trimsec->y1;
    } else {
        // Reading cols
        offset = (readout->image) ? readout->col0 + numScans : 0;
        offset = (numScans == 0) ? trimsec->y0 : offset; // full array ? read full trimsec : read section
        readout->col0 = offset;
        readout->row0 = trimsec->y0;
        lastScan = trimsec->x1;
    }
    upper = offset + numScans;

    // Blow away existing data.
    // Do this before returning, so that we're not returning data from a previous read
    psFree(readout->image);
    readout->image = NULL;

    while (readout->bias->n > 0) {
        psListRemove(readout->bias, PS_LIST_HEAD);
    }

    if (offset >= lastScan) {
        // We've read everything there is
        psTrace("psModules.camera", 7, "Read everything.\n");
        *status = true;
        return false;
    }

    psTrace("psModules.camera", 7, "offset=%d, upper = %d, image is %dx%d, trimsec [%.0f:%.0f,%.0f:%.0f]\n",
            offset, upper, naxis1, naxis2, trimsec->x0, trimsec->x1, trimsec->y0, trimsec->y1);

    // Get the new the trim section
    readout->image = readoutReadComponent(readout->image, fits, trimsec, readdir, offset, upper, z, bad,
                                          PS_TYPE_F32); // The image

    // Get the new bias sections
    psListIterator *biassecsIter = psListIteratorAlloc(biassecs, PS_LIST_HEAD, false); // Iterator for BIASSEC
    psRegion *biassec = NULL;           // Bias section from iteration
    while ((biassec = psListGetAndIncrement(biassecsIter))) {
        psImage *bias = readoutReadComponent(NULL, fits, biassec, readdir, offset, upper, z, bad,
                                             PS_TYPE_F32); // The bias
        psListAdd(readout->bias, PS_LIST_TAIL, bias);
        psFree(bias);                   // Drop reference
    }
    psFree(biassecsIter);

    *status = true;
    return true;
}



bool pmReadoutMore(pmReadout *readout, psFits *fits, int z, int *zMax, int numScans, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return readoutMore(readout, fits, z, zMax, numScans, FPA_READ_TYPE_IMAGE, config);
}

bool pmReadoutReadChunk(pmReadout *readout, psFits *fits, int z, int *zMax, int numScans, int overlap, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_INT_NONNEGATIVE(z, false);
    PS_ASSERT_INT_NONNEGATIVE(numScans, false);

    return readoutReadChunk(readout, fits, z, zMax, numScans, overlap, FPA_READ_TYPE_IMAGE, config);
}

bool pmReadoutRead(pmReadout *readout, psFits *fits, int z, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return readoutReadChunk(readout, fits, z, NULL, 0, 0, FPA_READ_TYPE_IMAGE, config);
}

int pmCellNumReadouts(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return cellNumReadouts(cell, fits, config);
}

bool pmCellRead(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return cellRead(cell, fits, config, FPA_READ_TYPE_IMAGE);
}

bool pmChipRead(pmChip *chip, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return chipRead(chip, fits, config, FPA_READ_TYPE_IMAGE);
}

bool pmFPARead(pmFPA *fpa, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return fpaRead(fpa, fits, config, FPA_READ_TYPE_IMAGE);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading the mask
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmReadoutMoreMask(pmReadout *readout, psFits *fits, int z, int *zMax, int numScans, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return readoutMore(readout, fits, z, zMax, numScans, FPA_READ_TYPE_MASK, config);
}

bool pmReadoutReadChunkMask(pmReadout *readout, psFits *fits, int z, int *zMax, int numScans, int overlap,
                            pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_INT_NONNEGATIVE(z, false);
    PS_ASSERT_INT_NONNEGATIVE(numScans, false);

    return readoutReadChunk(readout, fits, z, zMax, numScans, overlap, FPA_READ_TYPE_MASK, config);
}

bool pmReadoutReadMask(pmReadout *readout, psFits *fits, int z, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return readoutReadChunk(readout, fits, z, NULL, 0, 0, FPA_READ_TYPE_MASK, config);
}

bool pmCellReadMask(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return cellRead(cell, fits, config, FPA_READ_TYPE_MASK);
}

bool pmChipReadMask(pmChip *chip, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return chipRead(chip, fits, config, FPA_READ_TYPE_MASK);
}

bool pmFPAReadMask(pmFPA *fpa, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return fpaRead(fpa, fits, config, FPA_READ_TYPE_MASK);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading the variance map
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmReadoutMoreVariance(pmReadout *readout, psFits *fits, int z, int *zMax, int numScans, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return readoutMore(readout, fits, z, zMax, numScans, FPA_READ_TYPE_VARIANCE, config);
}

bool pmReadoutReadChunkVariance(pmReadout *readout, psFits *fits, int z, int *zMax, int numScans, int overlap,
                              pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_INT_NONNEGATIVE(z, false);
    PS_ASSERT_INT_NONNEGATIVE(numScans, false);

    return readoutReadChunk(readout, fits, z, zMax, numScans, overlap, FPA_READ_TYPE_VARIANCE, config);
}

bool pmReadoutReadVariance(pmReadout *readout, psFits *fits, int z, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return readoutReadChunk(readout, fits, z, NULL, 0, 0, FPA_READ_TYPE_VARIANCE, config);
}

bool pmCellReadVariance(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!cellRead(cell, fits, config, FPA_READ_TYPE_VARIANCE)) {
        return false;
    }
    pmHDU *hdu = pmHDUFromCell(cell);   // Header data unit
    if (hduCovariance(hdu, fits)) {
        return pmCellReadCovariance(cell, fits);
    }
    return true;
}

bool pmChipReadVariance(pmChip *chip, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!chipRead(chip, fits, config, FPA_READ_TYPE_VARIANCE)) {
        return false;
    }
    pmHDU *hdu = pmHDUFromChip(chip);   // Header data unit
    if (hduCovariance(hdu, fits)) {
        return pmChipReadCovariance(chip, fits);
    }
    return true;
}

bool pmFPAReadVariance(pmFPA *fpa, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!fpaRead(fpa, fits, config, FPA_READ_TYPE_VARIANCE)) {
        return false;
    }
    pmHDU *hdu = pmHDUFromFPA(fpa);     // Header data unit
    if (hduCovariance(hdu, fits)) {
        return pmFPAReadCovariance(fpa, fits);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading the image header
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmCellReadHeaderSet(pmCell *cell, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return cellRead(cell, fits, config, FPA_READ_TYPE_HEADER);
}

bool pmChipReadHeaderSet(pmChip *chip, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return chipRead(chip, fits, config, FPA_READ_TYPE_HEADER);
}

bool pmFPAReadHeaderSet(pmFPA *fpa, psFits *fits, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    return fpaRead(fpa, fits, config, FPA_READ_TYPE_HEADER);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading FITS tables
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int pmCellReadTable(pmCell *cell, psFits *fits, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(cell, 0);
    PS_ASSERT_FITS_NON_NULL(fits, 0);
    PS_ASSERT_STRING_NON_EMPTY(name, 0);

    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
    psString extname = NULL;            // Extension name
    psStringAppend(&extname, "%s_%s_%s", name, chipName, cellName);

    // XXX Could do a table lookup from the camera format, in case the input file isn't laid out with
    // NAME_CHIP_CELL extension names --- use these as keys, and the value as the proper extension name.
    // Allow interpolation of concepts, e.g., "{CHIP.NAME}" --> "ccd13".

    if (!psFitsMoveExtName(fits, extname)) {
        psError(PS_ERR_IO, false, "Unable to move to extension %s\n", extname);
        psFree(extname);
        return 0;
    }

    psMetadata *header = psFitsReadHeader(NULL, fits); // The FITS header
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read header for extension %s\n", extname);
        psFree(extname);
        psFree(header);
        return 0;
    }

    psString headerName = NULL;         // Name for header
    psStringAppend(&headerName, "%s.HEADER", name);
    if (!psMetadataAdd(cell->analysis, PS_LIST_TAIL, headerName, PS_DATA_METADATA,
                       "FITS table header", header)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add header from extension %s to analysis metadata "
                "for chip %s, cell %s\n", extname, chipName, cellName);
        psFree(headerName);
        psFree(header);
        psFree(extname);
        return 0;
    }
    psFree(headerName);
    psFree(header);

    psArray *table = psFitsReadTable(fits); // The table
    if (!psMetadataAdd(cell->analysis, PS_LIST_TAIL, name, PS_DATA_ARRAY, "FITS table", table)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add table from extension %s to analysis metadata "
                "for chip %s, cell %s\n", extname, chipName, cellName);
        psFree(table);
        psFree(extname);
        return 0;
    }

    psFree(extname);
    psFree(table);                      // Dropping reference

    return 1;
}


int pmChipReadTable(pmChip *chip, psFits *fits, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(chip, 0);
    PS_ASSERT_FITS_NON_NULL(fits, 0);
    PS_ASSERT_STRING_NON_EMPTY(name, 0);

    int numRead = 0;                    // Number of reads
    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        numRead += pmCellReadTable(cell, fits, name);
    }

    return numRead;
}


int pmFPAReadTable(pmFPA *fpa, psFits *fits, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(fpa, 0);
    PS_ASSERT_FITS_NON_NULL(fits, 0);
    PS_ASSERT_STRING_NON_EMPTY(name, 0);

    int numRead = 0;                    // Number of reads
    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        numRead += pmChipReadTable(chip, fits, name);
    }

    return numRead;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading covariance matrices
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmCellReadCovariance(pmCell *cell, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
    psString extname = NULL;            // Extension name
    psStringAppend(&extname, "COVAR_%s_%s", chipName, cellName);

    if (!psFitsMoveExtName(fits, extname)) {
        psError(PS_ERR_IO, false, "Unable to move to extension %s\n", extname);
        psFree(extname);
        return false;
    }
    psFree(extname);

    psMetadata *header = psFitsReadHeader(NULL, fits); // The FITS header
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read header for extension %s\n", extname);
        psFree(header);
        return false;
    }

    bool mdok;                          // Status of MD lookup
    int x0 = psMetadataLookupS32(&mdok, header, "COVARIANCE.CENTRE.X"); // Centre of matrix in x
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to read covariance centre");
        psFree(header);
        return false;
    }
    int y0 = psMetadataLookupS32(&mdok, header, "COVARIANCE.CENTRE.Y"); // Centre of matrix in y
    if (!mdok) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to read covariance centre");
        psFree(header);
        return false;
    }
    psFree(header);

    psArray *images = psFitsReadImageCube(fits, psRegionSet(0,0,0,0)); // Covariance matrices
    if (!images) {
        psError(PS_ERR_IO, false, "Unable to read covariance matrices for chip %s, cell %s",
                chipName, cellName);
        return false;
    }

    psArray *readouts = cell->readouts; // Readouts of cell
    if (images->n != readouts->n) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Number of covariance matrices (%ld) doesn't match number of readouts (%ld)",
                images->n, readouts->n);
        psFree(images);
        return false;
    }

    for (int i = 0; i < readouts->n; i++) {
        pmReadout *ro = readouts->data[i]; // Readout of interest
        psImage *image = images->data[i]; // Image of interest
        if (ro->covariance) {
            psWarning("Clobbering extant covariance matrix in chip %s, cell %s, readout %d",
                      chipName, cellName, i);
            psFree(ro->covariance);
        }
        ro->covariance = psKernelAllocFromImage(image, x0, y0);
    }
    psFree(images);

    return true;
}


bool pmChipReadCovariance(pmChip *chip, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        if (!pmCellReadCovariance(cell, fits)) {
            return false;
        }
    }

    return true;
}


bool pmFPAReadCovariance(pmFPA *fpa, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        if (!pmChipReadCovariance(chip, fits)) {
            return false;
        }
    }

    return true;
}
