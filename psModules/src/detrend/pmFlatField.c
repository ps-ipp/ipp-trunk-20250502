#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPAMaskWeight.h"
#include "pmFlatField.h"
#include "pmDetrendThreads.h"

bool pmFlatFieldScan_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psImage *inImage = job->args->data[0]; // Input image
    psImage *inMask  = job->args->data[1]; // Input mask
    psImage *inVar   = job->args->data[2]; // Input variance
    const psImage *flatImage = job->args->data[3]; // Flat-field image
    const psImage *flatMask  = job->args->data[4]; // Flat-field mask

    psImageMaskType badFlat = PS_SCALAR_VALUE(job->args->data[5], PS_TYPE_IMAGE_MASK_DATA);
    int xOffset        = PS_SCALAR_VALUE(job->args->data[6], S32);
    int yOffset        = PS_SCALAR_VALUE(job->args->data[7], S32);
    int rowStart       = PS_SCALAR_VALUE(job->args->data[8], S32);
    int rowStop        = PS_SCALAR_VALUE(job->args->data[9], S32);

    return pmFlatFieldScan(inImage, inMask, inVar, flatImage, flatMask, badFlat,
                           xOffset, yOffset, rowStart, rowStop);
}


bool pmFlatFieldScan(psImage *inImage, psImage *inMask, psImage *inVar, const psImage *flatImage,
                     const psImage *flatMask, psImageMaskType badFlat,
                     int xOffset, int yOffset, int rowStart, int rowStop)
{
    // Neglecting asserts, because inputs should have been checked already

    int numCols = inImage->numCols;     // Number of columns

    // Using i,j for image; x,y for flat.
    for (int j = rowStart, y = rowStart + yOffset; j < rowStop; j++, y++) {
        for (int i = 0, x = xOffset; i < numCols; i++, x++) {
            float flatValue = 1.0 / flatImage->data.F32[y][x];
            if (!isfinite(flatValue) || flatValue <= 0.0 ||
                (flatMask && flatMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x])) {
                if (inMask) {
                    inMask->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= badFlat;
                }
                inImage->data.F32[j][i] = NAN;
                if (inVar) {
                    inVar->data.F32[j][i] = NAN;
                }
            } else {
                inImage->data.F32[j][i] *= flatValue;
                if (inVar) {
                    inVar->data.F32[j][i] *= PS_SQR(flatValue);
                }
            }
        }
    }

    return true;
}

bool pmFlatField(pmReadout *in, const pmReadout *flat, psImageMaskType badFlat)
{
    PM_ASSERT_READOUT_NON_NULL(in, false);
    PM_ASSERT_READOUT_IMAGE(in, false);
    PM_ASSERT_READOUT_NON_NULL(flat, false);
    PM_ASSERT_READOUT_IMAGE(flat, false);
    if (in->mask) {
        PM_ASSERT_READOUT_MASK(in, false);
    }
    if (in->variance) {
        PM_ASSERT_READOUT_VARIANCE(in, false);
    }
    if (flat->mask) {
        PM_ASSERT_READOUT_MASK(flat, false);
    }

    psImage *inImage   = in->image;     // Input image
    psImage *inMask    = in->mask;      // Mask for input image
    psImage *inVar     = in->variance;  // Variance for input image
    psImage *flatImage = flat->image;   // Flat-field image
    psImage *flatMask  = flat->mask;    // Mask for flat-field image

    // Add flat-field MD5 to header
    pmHDU *hdu = pmHDUFromReadout(in);  // HDU of interest
    psVector *md5 = psImageMD5(flat->image); // md5 hash
    psString md5string = psMD5toString(md5); // String
    psFree(md5);
    psStringPrepend(&md5string, "FLAT image MD5: ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     md5string, "");
    psFree(md5string);

    // Check input image is not larger than flat image; mask is the same size as the input
    if (inImage->numRows > flatImage->numRows || inImage->numCols > flatImage->numCols) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Input image (%dx%d) is larger than flat-field image "
                "(%dx%d).\n", inImage->numCols, inImage->numRows, flatImage->numCols, flatImage->numRows);
        return false;
    }

    // Offsets on the chip
    int x0in = psMetadataLookupS32(NULL, in->parent->concepts, "CELL.X0");
    int y0in = psMetadataLookupS32(NULL, in->parent->concepts, "CELL.Y0");
    int x0flat = psMetadataLookupS32(NULL, flat->parent->concepts, "CELL.X0");
    int y0flat = psMetadataLookupS32(NULL, flat->parent->concepts, "CELL.Y0");

    // Determine offset based on image offset with chip offset: input frame to flat frame
    int yOffset = in->row0 + y0in - flat->row0 - y0flat;
    int xOffset = in->col0 + x0in - flat->col0 - x0flat;

    // Check that offsets are within image limits
    if (inImage->numRows + yOffset > flatImage->numRows ||
            inImage->numCols + xOffset > flatImage->numCols) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Input image (%dx%d) with offsets (%d,%d) is larger than "
                "flat-field image (%dx%d).\n", inImage->numCols, inImage->numRows, xOffset, yOffset,
                flatImage->numCols, flatImage->numRows);
        return false;
    }

    bool threaded = true;
    int scanRows = pmDetrendGetScanRows();
    if (scanRows == 0) {
        threaded = false;
        scanRows = inImage->numRows;
    }

    for (int rowStart = 0; rowStart < inImage->numRows; rowStart += scanRows) {
      int rowStop = PS_MIN(rowStart + scanRows, inImage->numRows);

      if (threaded) {
          // allocate a job, construct the arguments for this job
          psThreadJob *job = psThreadJobAlloc("PSMODULES_DETREND_FLAT");
          psArrayAdd(job->args, 1, inImage);
          psArrayAdd(job->args, 1, inMask);
          psArrayAdd(job->args, 1, inVar);
          psArrayAdd(job->args, 1, flatImage);
          psArrayAdd(job->args, 1, flatMask);
          PS_ARRAY_ADD_SCALAR(job->args, badFlat, PS_TYPE_IMAGE_MASK);
          PS_ARRAY_ADD_SCALAR(job->args, xOffset, PS_TYPE_S32);
          PS_ARRAY_ADD_SCALAR(job->args, yOffset, PS_TYPE_S32);
          PS_ARRAY_ADD_SCALAR(job->args, rowStart, PS_TYPE_S32);
          PS_ARRAY_ADD_SCALAR(job->args, rowStop, PS_TYPE_S32);

          if (!psThreadJobAddPending(job)) {
              return false;
          }
      } else if (!pmFlatFieldScan(inImage, inMask, inVar, flatImage, flatMask, badFlat,
                                  xOffset, yOffset, rowStart, rowStop)) {
          psError(PS_ERR_UNKNOWN, false, "Unable to flat-field image.");
          return false;
      }
    }

    if (threaded) {
        // wait here for the threaded jobs to finish
        if (!psThreadPoolWait(true, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to flat-field image.");
            return false;
        }
    }

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now, used for reporting
    psString timeString = psTimeToISO(time); // String with time
    psFree(time);
    psStringPrepend(&timeString, "Flat-field processing completed at ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     timeString, "");
    psFree(timeString);

    return true;
}
