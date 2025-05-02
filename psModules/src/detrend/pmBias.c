#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPACalibration.h"
#include "pmDetrendThreads.h"

#include "pmOverscan.h"
#include "pmBias.h"

bool pmBiasSubtractScan_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);
    pmReadout *in = job->args->data[0];
    const pmReadout *sub = job->args->data[1];
    float scale  = PS_SCALAR_VALUE(job->args->data[2],F32);
    int xOffset  = PS_SCALAR_VALUE(job->args->data[3],S32);
    int yOffset  = PS_SCALAR_VALUE(job->args->data[4],S32);
    int rowStart = PS_SCALAR_VALUE(job->args->data[5],S32);
    int rowStop  = PS_SCALAR_VALUE(job->args->data[6],S32);
    return pmBiasSubtractScan(in, sub, scale, xOffset, yOffset, rowStart, rowStop);
}

bool pmBiasSubtractScan(pmReadout *in, const pmReadout *sub, float scale,
                        int xOffset, int yOffset, int rowStart, int rowStop)
{
    psImage *inImage  = in->image;      // The input image
    psImage *inMask   = in->mask;       // The input mask
    const psImage *subImage = sub->image; // The image to be subtracted
    const psImage *subMask  = sub->mask; // The mask for the subtraction image

    if (scale == 1.0) {
        for (int i = rowStart; i < rowStop; i++) {
            for (int j = 0; j < inImage->numCols; j++) {
                inImage->data.F32[i][j] -= subImage->data.F32[i+yOffset][j+xOffset];
                if (inMask && subMask) {
                    inMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] |= subMask->data.PS_TYPE_IMAGE_MASK_DATA[i+yOffset][j+xOffset];
                }
            }
        }
    } else {
        for (int i = rowStart; i < rowStop; i++) {
            for (int j = 0; j < inImage->numCols; j++) {
                inImage->data.F32[i][j] -= subImage->data.F32[i+yOffset][j+xOffset] * scale;
                if (inMask && subMask) {
                    inMask->data.PS_TYPE_IMAGE_MASK_DATA[i][j] |= subMask->data.PS_TYPE_IMAGE_MASK_DATA[i+yOffset][j+xOffset];
                }
            }
        }
    }
    return true;
}

// pmBiasSubtractFrame():
// this routine will take as input a readout for the input image and a readout for the bias
// image.  The bias image is subtracted in place from the input image.
bool pmBiasSubtractFrame(pmReadout *in, // Input readout
                         pmReadout *sub, // Readout to be subtracted from input
                         float scale   // Scale to apply before subtracting
    )
{
    PS_ASSERT_PTR_NON_NULL(in, false);
    PS_ASSERT_PTR_NON_NULL(in->image, false);
    PS_ASSERT_IMAGE_TYPE(in->image, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_NON_EMPTY(in->image, false);
    PS_ASSERT_PTR_NON_NULL(sub, false);
    PS_ASSERT_PTR_NON_NULL(sub->image, false);
    PS_ASSERT_IMAGE_TYPE(sub->image, PS_TYPE_F32, false);
    PS_ASSERT_IMAGE_NON_EMPTY(sub->image, false);
    PS_ASSERT_IMAGES_SIZE_EQUAL(in->image, sub->image, false);

    psImage *inImage  = in->image;      // The input image
    psImage *subImage = sub->image;     // The image to be subtracted

    // Check parities
    int xIpar = psMetadataLookupS32(NULL, in->parent->concepts, "CELL.XPARITY");
    int xSpar = psMetadataLookupS32(NULL, sub->parent->concepts, "CELL.XPARITY");
    if (xIpar != xSpar) {
        psError(PS_ERR_UNKNOWN, true, "images for subtraction do not have the same "
                "CELL.XPARITY (%d vs %d).\n    pmSubtractBias must be upgraded to handle this situation\n",
                xIpar, xSpar);
        return false;
    }

    int yIpar = psMetadataLookupS32(NULL, in->parent->concepts, "CELL.YPARITY");
    int ySpar = psMetadataLookupS32(NULL, sub->parent->concepts, "CELL.YPARITY");
    if (yIpar != ySpar) {
        psError(PS_ERR_UNKNOWN, true, "images for subtraction do not have the same "
                "CELL.YPARITY (%d vs %d).\n    pmSubtractBias must be upgraded to handle this situation\n",
                xIpar, xSpar);
        return false;
    }

    // Offsets of the cells
    int x0in = psMetadataLookupS32(NULL, in->parent->concepts, "CELL.X0");
    int y0in = psMetadataLookupS32(NULL, in->parent->concepts, "CELL.Y0");
    int x0sub = psMetadataLookupS32(NULL, sub->parent->concepts, "CELL.X0");
    int y0sub = psMetadataLookupS32(NULL, sub->parent->concepts, "CELL.Y0");

    if ((inImage->numCols + x0in - x0sub) > subImage->numCols) {
        psError(PS_ERR_UNKNOWN, true, "Image does not have enough columns for subtraction (%d vs %d).\n",
                inImage->numCols + x0in - x0sub, subImage->numCols);
        return false;
    }
    if ((inImage->numRows + y0in - y0sub) > subImage->numRows) {
        psError(PS_ERR_UNKNOWN, true, "Image does not have enough rows for subtraction (%d vs %d).\n",
                inImage->numRows + y0in - y0sub, subImage->numRows);
        return false;
    }

    int xOffset = x0in - x0sub;
    int yOffset = y0in - y0sub;

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
            psThreadJob *job = psThreadJobAlloc("PSMODULES_DETREND_BIAS");
            psArrayAdd(job->args, 1, in);
            psArrayAdd(job->args, 1, sub);
            PS_ARRAY_ADD_SCALAR(job->args, scale, PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, xOffset, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, yOffset, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, rowStart, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, rowStop, PS_TYPE_S32);

            if (!psThreadJobAddPending(job)) {
                return false;
            }
        } else if (!pmBiasSubtractScan(in, sub, scale, xOffset, yOffset, rowStart, rowStop)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to apply bias correction.");
            return false;
        }
    }

    if (threaded) {
        // wait here for the threaded jobs to finish
        if (!psThreadPoolWait(true, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to apply bias correction.");
            return false;
        }
    }

    return true;
}

bool pmBiasSubtract(pmReadout *in, 
                    pmReadout *bias, pmReadout *dark, const pmFPAview *view)
{
    psTrace("psModules.detrend", 4,
            "---- pmBiasSubtract() begin ----\n");

    PS_ASSERT_PTR_NON_NULL(in, false);
    PS_ASSERT_IMAGE_NON_NULL(in->image, false);
    PS_ASSERT_IMAGE_TYPE(in->image, PS_TYPE_F32, false);
    if (bias) {
        PS_ASSERT_IMAGE_NON_NULL(bias->image, false);
        PS_ASSERT_IMAGE_TYPE(bias->image, PS_TYPE_F32, false);
    }
    if (dark) {
        psWarning("Dark processing is now available using pmDark --- perhaps you should use that instead?");
        PS_ASSERT_PTR_NON_NULL(view, false);
        PS_ASSERT_IMAGE_NON_NULL(dark->image, false);
        PS_ASSERT_IMAGE_TYPE(dark->image, PS_TYPE_F32, false);
    }

    pmHDU *hdu = pmHDUFromReadout(in);  // HDU of interest

    // Bias frame subtraction
    if (bias) {
        psVector *md5 = psImageMD5(bias->image); // md5 hash
        psString md5string = psMD5toString(md5); // String
        psFree(md5);
        psStringPrepend(&md5string, "BIAS image MD5: ");
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                         md5string, "");
        psFree(md5string);

        if (!pmBiasSubtractFrame(in, bias, 1.0)) {
            return false;
        }
    }

    if (dark) {
        // Get the scaling
        float inTime = psMetadataLookupF32(NULL, in->parent->concepts, "CELL.DARKTIME");
        float darkTime = psMetadataLookupF32(NULL, dark->parent->concepts, "CELL.DARKTIME");
        if (isnan(inTime) || isnan(darkTime)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine dark scaling.");
            return false;
        }

        float darkNorm = 1.0;
        float inNorm = pmFPADarkNorm(in->parent->parent->parent, view, inTime);

        // if we have a normalized dark exposure, we simply multiply the master by inNorm.  if
        // we do not have a normalized exposure, we have to scale the master as well.  XXX do
        // we need to explicitly identify the master as normalized?

        if (darkTime != 1.0) {
            darkNorm = pmFPADarkNorm(dark->parent->parent->parent, view, darkTime);
        }

        if (isnan(inNorm) || isnan(darkNorm)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to determine dark normalisations.");
            return false;
        }

        float scale = inNorm / darkNorm;// Scaling to apply to dark exposure

        psVector *md5 = psImageMD5(dark->image); // md5 hash
        psString md5string = psMD5toString(md5); // String
        psFree(md5);
        psStringPrepend(&md5string, "DARK image (scale %.3f) MD5: ", scale);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                         md5string, "");
        psFree(md5string);

        if (!pmBiasSubtractFrame(in, dark, scale)) {
            return false;
        }
    }

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now, used for reporting
    psString timeString = psTimeToISO(time); // String with time
    psFree(time);
    psStringPrepend(&timeString, "Bias/dark processing completed at ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     timeString, "");
    psFree(timeString);


    return true;
}


