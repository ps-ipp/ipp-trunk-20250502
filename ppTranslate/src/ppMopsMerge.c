#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "ppMops.h"

#define LEAF_SIZE 4                     // Size of leaf
#define MATCH_RADIUS SEC_TO_RAD(1.0)    // Matching radius
#define MJD_TOL 1.0/3600.0/24.0         // Tolerance for MJD matching
#define BORESIGHT_TOL SEC_TO_RAD(1.0)   // Tolerance for boresight matching
#define EXPTIME_TOL 1.0e-3              // Tolerance for exposure time matching
#define POSANGLE_TOL SEC_TO_RAD(1.0)    // Tolerance for position angle matching
#define AIRMASS_TOL 1.0e-3              // Tolerance for airmass matching

#if 0
#undef psTrace
#define psTrace(facil, level, ...)              \
    if (level <= 5) {                           \
        fprintf(stderr, __VA_ARGS__);           \
    }
#endif

// Get distance from detection to centre of image
static float mergeDistance(const ppMopsDetections *detections, // Detections of interest
                           long index                          // Index for source of interest
    )
{
    float dx = detections->x->data.F32[index] - detections->naxis1 / 2.0;
    float dy = detections->y->data.F32[index] - detections->naxis2 / 2.0;
    return PS_SQR(dx) + PS_SQR(dy);
}


bool ppMopsPurgeDuplicates(const psArray *detections)
{
    PS_ASSERT_ARRAY_NON_NULL(detections, NULL);

    int numInputs = detections->n;                // Number of inputs
    psTrace("ppMops.merge", 1, "Checking detections from %d inputs\n", numInputs);

    long total = 0;                                // Total number of sources
    psArray *duplicates = psArrayAlloc(numInputs); // Vector of duplicate bits for each input
    psVector *dupNum = psVectorAlloc(numInputs, PS_TYPE_U32); // Number of duplicates for each input
    psVectorInit(dupNum, 0);
    for (int i = 0; i < numInputs; i++) {
        ppMopsDetections *det = detections->data[i]; // Detections from
        if (!det || det->num == 0) {
            continue;
        }
        psVector *dupes = duplicates->data[i] = psVectorAlloc(det->num, PS_TYPE_U8);
        psVectorInit(dupes, 0);
        total += det->num;
    }
    psTrace("ppMops.merge", 2, "Total detections: %ld\n", total);

    psVector *raMerged = psVectorAlloc(total, PS_TYPE_F64); // Merged RAs
    psVector *decMerged = psVectorAlloc(total, PS_TYPE_F64); // Merged Decs
    psVector *sourceMerged = psVectorAlloc(total, PS_TYPE_U16); // Source image of merged sources
    psVector *indexMerged = psVectorAlloc(total, PS_TYPE_U32); // Index of merged sources
    long num = 0;                                                   // Number of merged sources

    const char *raBoresight = NULL, *decBoresight = NULL; // Boresight coordinates
    const char *filter = NULL;                    // Filter name
    float airmass = NAN, exptime = NAN;           // Exposure details
    double posangle = NAN, alt = NAN, az = NAN; // Telescope pointing
    double mjd = NAN;                           // Time of exposure

    for (int i = 0; i < numInputs; i++) {
      psTrace("ppMops.merge", 1, "Processing batch %d\n", i);
        ppMopsDetections *det = detections->data[i]; // Detections of interest
        if (!det) {
            psTrace("ppMops.merge", 3, "Ignoring NULL input %d\n", i);
            continue;
        } else if (det->num == 0) {
            psTrace("ppMops.merge", 3, "Ignoring empty input %d\n", i);
            continue;
        }

        // Check exposure characteristics
        if (num == 0) {
            raBoresight = det->raBoresight;
            decBoresight = det->decBoresight;
            filter = det->filter;
            airmass = det->airmass;
            exptime = det->exptime;
            posangle = det->posangle;
            alt = det->alt;
            az = det->az;
        } else {
            if (strcmp(raBoresight, det->raBoresight) != 0) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure RA values differ: %s vs %s",
                        raBoresight, det->raBoresight);
                return false;
            }
            if (strcmp(decBoresight, det->decBoresight) != 0) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure Dec values differ: %s vs %s",
                        decBoresight, det->decBoresight);
                return false;
            }
            if (strcmp(filter, det->filter) != 0) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure filter values differ: %s vs %s",
                        filter, det->filter);
                return false;
            }
            if (fabsf(airmass - det->airmass) > AIRMASS_TOL) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure airmass values differ: %f vs %f",
                        airmass, det->airmass);
                return false;
            }
            if (fabsf(exptime - det->exptime) > EXPTIME_TOL) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure exposure time values differ: %f vs %f",
                        exptime, det->exptime);
                return false;
            }
            if (fabs(posangle - det->posangle) > POSANGLE_TOL) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure position angle values differ: %f vs %f",
                        posangle, det->posangle);
                return false;
            }
            if (fabs(alt - det->alt) > BORESIGHT_TOL) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure altitude values differ: %lf vs %lf",
                        alt, det->alt);
                return false;
            }
            if (fabs(az - det->az) > BORESIGHT_TOL) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure azimuth values differ: %lf vs %lf",
                        az, det->az);
                return false;
            }
            if (fabs(mjd - det->mjd) > MJD_TOL) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure MJD values differ: %lf vs %lf",
                        mjd, det->mjd);
                return false;
            }
        }

        psTrace("ppMops.merge", 3, "Accepting %ld detections from input %d\n", det->num, i);
        memcpy(&raMerged->data.F64[num], det->ra->data.F64, det->num * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        memcpy(&decMerged->data.F64[num], det->dec->data.F64, det->num * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        for (long j = 0, k = num; j < det->num; j++, k++) {
            sourceMerged->data.U16[k] = i;
            indexMerged->data.U32[k] = j;
        }
        num += det->num;
    }

    psTrace("ppMops.merge", 3, "Generating kd-tree from %ld sources\n", num);
    psTree *tree = psTreePlant(2, LEAF_SIZE, PS_TREE_SPHERICAL, raMerged, decMerged); // kd tree
    if (!tree) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate kd tree");
        return false;
    }

    for (int i = 0; i < detections->n; i++) {
        ppMopsDetections *det = detections->data[i]; // Detections of interest
        if (!det) {
            continue;
        } else if (det->num == 0) {
            continue;
        }
        psTrace("ppMops.merge", 3, "Checking %ld detections from input %d\n", det->num, i);

        psVector *dupes = duplicates->data[i];            // Duplicates list
        psVector *coords = psVectorAlloc(2, PS_TYPE_F64); // Coordinates of interest
        for (int j = 0; j < det->num; j++) {
            // XXX: added by Bill
            if (det->mask->data.U8[j]) {
                // we marked this source marked as bad when we read it, mark it as a duplicate
                dupes->data.U8[j] = 0xFF;
                continue;
            }

            coords->data.F64[0] = det->ra->data.F64[j];
            coords->data.F64[1] = det->dec->data.F64[j];
            psVector *indices = psTreeAllWithin(tree, coords, MATCH_RADIUS); // Indices for matching sources
            if (!indices) {
                psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to search for matches");
                psFree(coords);
                psFree(tree);
                return false;
            }
            psTrace("ppMops.merge", 5, "%ld matches for source %d from input %d\n", indices->n, j, i);
            psAssert(indices->n > 0, "Expect at least one match for source %d in input %d", j, i);

            if (indices->n == 1 && sourceMerged->data.U16[indices->data.S64[0]] == i) {
                // It's myself
                psFree(indices);
                continue;
            }

            // Which one do we keep?
            float bestDistance = INFINITY; // Best distance to centre
            long bestIndex = -1;           // Index with best distance
            for (int k = 0; k < indices->n; k++) {
                long mergeIndex = indices->data.S64[k]; // Index of point in merged list
                int source = sourceMerged->data.U16[mergeIndex]; // Source image
                if (source == i) {
                    continue;
                }
                long index = indexMerged->data.U32[mergeIndex];  // Index in source
                psVector *dupes = duplicates->data[source];     // Duplicates list
                if (dupes->data.U8[index]) {
                    continue;
                }

                float distance = mergeDistance(detections->data[source], index); // Distance to centre of image
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestIndex = index;
                }
            }

            if (bestIndex >= 0) {
                float distance = mergeDistance(det, j); // Distance to centre of image
                if (bestIndex >= 0 && distance < bestDistance) {
                    psTrace("ppMops.merge", 6, "New source clobbers old sources\n");
                    for (int k = 0; k < indices->n; k++) {
                        long mergeIndex = indices->data.S64[k]; // Index of point
                        int source = sourceMerged->data.U16[mergeIndex]; // Source image
                        if (source == i) {
                            continue;
                        }
                        long index = indexMerged->data.U32[mergeIndex];  // Index in source
                        psVector *dupes = duplicates->data[source];      // Duplicates list
                        if (!dupes->data.U8[index]) {
                            dupes->data.U8[index] = 0xFF;
                            dupNum->data.U32[source]++;
                        }
                    }
                } else {
                    psTrace("ppMops.merge", 6, "Old sources clobber new source\n");
                    dupes->data.U8[j] = 0xFF;
                    dupNum->data.U32[i]++;
                }
            }
            psFree(indices);
        }
        psFree(coords);
    }
    psFree(tree);
    psFree(raMerged);
    psFree(decMerged);
    psFree(sourceMerged);
    psFree(indexMerged);

    // Remove duplicates
    for (int i = 0; i < detections->n; i++) {
        ppMopsDetections *det = detections->data[i]; // Detections of interest
        if (!det) {
            continue;
        } else if (det->num == 0) {
            continue;
        }
        psTrace("ppMops.merge", 3, "Purging %d duplicates from input %d\n", dupNum->data.U32[i], i);

#define VECTOR_PURGE_CASE(TYPE)                                     \
        case PS_TYPE_##TYPE:    {                                   \
          long j = 0;                                               \
          for (long i = 0; i < vector->n; i++) {                    \
              if (!dupes->data.U8[i]) {                             \
                  if (i == j) {                                     \
                      j++;                                          \
                      continue;                                     \
                  }                                                 \
                  vector->data.TYPE[j++] = vector->data.TYPE[i];    \
              }                                                     \
          }                                                         \
          vector->n = j;                                            \
        }                                                           \
        break

        psVector *dupes = duplicates->data[i]; // Duplicates
        psArray *table = psListToArray(det->table->list); // Table of data
        long newLength = -1;
        for (int t = 0; t < table->n; t++) {
            psMetadataItem *item = table->data[t]; // Table item
            psAssert(item->type == PS_DATA_VECTOR, "Table column is not a vector: %x", item->type);
            psVector *vector = item->data.V; // Vector to purge
            switch (vector->type.type) {
                VECTOR_PURGE_CASE(U8);
                VECTOR_PURGE_CASE(U16);
                VECTOR_PURGE_CASE(U32);
                VECTOR_PURGE_CASE(U64);
                VECTOR_PURGE_CASE(S8);
                VECTOR_PURGE_CASE(S16);
                VECTOR_PURGE_CASE(S32);
                VECTOR_PURGE_CASE(S64);
                VECTOR_PURGE_CASE(F32);
                VECTOR_PURGE_CASE(F64);
              default:
                psAbort("Unrecognised vector type: %x", vector->type.type);
            }
            if (newLength == -1) {
                newLength = vector->n;
            } else if (vector->n != newLength) {
                psAbort("Unexpected new length found : %ld expected: %ld",
                                                vector->n, newLength);
            }
	}
        // XXX IS this safe? Perhaps save in numGood?
        det->num = newLength;
        psFree(table);
    }
    psFree(dupNum);
    psFree(duplicates);

    return true;
}

