#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "ppMonet.h"

#define LEAF_SIZE 4                     // Size of leaf
#define MATCH_RADIUS SEC_TO_RAD(1.0)    // Matching radius
#define MJD_TOL 1.0/3600.0/24.0         // Tolerance for MJD matching
#define BORESIGHT_TOL SEC_TO_RAD(1.0)   // Tolerance for boresight matching
#define EXPTIME_TOL 1.0e-3              // Tolerance for exposure time matching
#define POSANGLE_TOL SEC_TO_RAD(1.0)    // Tolerance for position angle matching
#define AIRMASS_TOL 1.0e-3              // Tolerance for airmass matching

// Get distance from detection to centre of image
static float mergeDistance(const ppMonetDetections *detections, // Detections of interest
                           long index                          // Index for source of interest
    )
{
    float dx = detections->x->data.F32[index] - detections->naxis1->data.S32[index] / 2.0;
    float dy = detections->y->data.F32[index] - detections->naxis2->data.S32[index] / 2.0;
    return PS_SQR(dx) + PS_SQR(dy);
}


ppMonetDetections *ppMonetMerge(const psArray *detections)
{
    PS_ASSERT_ARRAY_NON_NULL(detections, NULL);

    psTrace("ppMonet.merge", 1, "Merging detections from %ld inputs\n", detections->n);

    ppMonetDetections *merged = NULL;    // Merged list
    int num = 1;                                                         // Number of merged files
    for (int i = 0; i < detections->n; i++) {
        ppMonetDetections *det = detections->data[i]; // Detections of interest
        if (!det) {
            psTrace("ppMonet.merge", 3, "Ignoring NULL input %d\n", i);
            continue;
        } else if (det->num == 0) {
            psTrace("ppMonet.merge", 3, "Ignoring empty input %d\n", i);
            continue;
        }
        num++;
        if (!merged) {
            psTrace("ppMonet.merge", 3, "Accepting %ld detections from input %d\n", det->num, i);
            merged = psMemIncrRefCounter(det);
            continue;
        }
        psTrace("ppMonet.merge", 3, "Merging %ld detections from input %d\n", det->num, i);

        // XXX compare exposure properties
        if (strcmp(merged->raBoresight, det->raBoresight) != 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure RA values differ: %s vs %s",
                    merged->raBoresight, det->raBoresight);
            return NULL;
        }
        if (strcmp(merged->decBoresight, det->decBoresight) != 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure Dec values differ: %s vs %s",
                    merged->decBoresight, det->decBoresight);
            return NULL;
        }
        if (strcmp(merged->filter, det->filter) != 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure filter values differ: %s vs %s",
                    merged->filter, det->filter);
            return NULL;
        }

        if (fabsf(merged->airmass - det->airmass) > AIRMASS_TOL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure airmass values differ: %f vs %f",
                    merged->airmass, det->airmass);
            return NULL;
        }
        if (fabsf(merged->exptime - det->exptime) > EXPTIME_TOL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure exposure time values differ: %f vs %f",
                    merged->exptime, det->exptime);
            return NULL;
        }
        if (fabs(merged->posangle - det->posangle) > POSANGLE_TOL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure position angle values differ: %f vs %f",
                    merged->posangle, det->posangle);
            return NULL;
        }
        if (fabs(merged->alt - det->alt) > BORESIGHT_TOL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure altitude values differ: %lf vs %lf",
                    merged->alt, det->alt);
            return NULL;
        }
        if (fabs(merged->az - det->az) > BORESIGHT_TOL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure azimuth values differ: %lf vs %lf",
                    merged->az, det->az);
            return NULL;
        }
        if (fabs(merged->mjd - det->mjd) > MJD_TOL) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure MJD values differ: %lf vs %lf",
                    merged->mjd, det->mjd);
            return NULL;
        }

        merged->seeing += det->seeing;  // Taking average

        psTree *tree = psTreePlant(2, LEAF_SIZE, PS_TREE_SPHERICAL, merged->ra, merged->dec); // kd tree
        if (!tree) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate kd tree");
            psFree(merged);
            return NULL;
        }

        psVector *coords = psVectorAlloc(2, PS_TYPE_F64); // Coordinates of interest
        for (int j = 0; j < det->num; j++) {
            coords->data.F64[0] = det->ra->data.F64[j];
            coords->data.F64[1] = det->dec->data.F64[j];
            psVector *indices = psTreeAllWithin(tree, coords, MATCH_RADIUS); // Indices for matching sources
            if (!indices) {
                psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to search for matches");
                psFree(coords);
                psFree(tree);
                psFree(merged);
                return NULL;
            }
            if (indices->n == 0) {
                psTrace("ppMonet.merge", 9, "No matches for source %d in input %d\n", j, i);
                psFree(indices);
                ppMonetDetectionsCopySingle(merged, det, j);
                continue;
            }
            psTrace("ppMonet.merge", 5, "%ld matches for source %d from input %d\n", indices->n, j, i);

            // Which one do we keep?
            float bestDistance = INFINITY; // Best distance to centre
            long bestIndex = -1;           // Index with best distance
            for (int k = 0; k < indices->n; k++) {
                long index = indices->data.S64[k]; // Index of point
                float distance = mergeDistance(merged, index); // Distance to centre of image
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestIndex = index;
                }
            }

            float distance = mergeDistance(det, j); // Distance to centre of image
            if (distance < bestDistance) {
                psTrace("ppMonet.merge", 6, "New source clobbers old sources\n");
                // Blow away existing sources
                for (int k = 0; k < indices->n; k++) {
                    long index = indices->data.S64[k]; // Index of point
                    merged->mask->data.U8[index] = 0xFF;
                }
                ppMonetDetectionsCopySingle(merged, det, j);
            } else {
                psTrace("ppMonet.merge", 6, "Old sources clobber new source\n");
            }
            psFree(indices);
        }

        psTrace("ppMonet.merge", 3, "Done merging input %d, %ld merged sources\n", i, merged->num);

        psFree(tree);
        ppMonetDetectionsPurge(merged);
    }

    psTrace("ppMonet.merge", 2, "%ld sources in merged detections list\n", merged->num);

    merged->seeing /= num;

    return merged;
}

