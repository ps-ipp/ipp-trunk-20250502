#include <stdio.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

# define NUM_POINTS 32
# define IMAGE_STDEV 10.0
# define IMAGE_MEAN  20.0

// Generate a dummy variance image
psImage *generateVariance(int numCols, int numRows, psElemType type)
{
    psImage *image = psImageAlloc(numCols, numRows, type);
    for (int j = 0; j < numRows; j++) {
        for (int i = 0; i < numCols; i++) {
            psImageSet(image, i, j, PS_SQR(IMAGE_STDEV));
        }
    }
    return image;
}

// Generate a dummy image
psImage *generateImage(psImage *image, psRandom *rnd, int numCols, int numRows, psElemType type)
{
    if (!image) {
	image = psImageAlloc(numCols, numRows, type);
    }
    for (int j = 0; j < numRows; j++) {
        for (int i = 0; i < numCols; i++) {
            psImageSet(image, i, j, IMAGE_MEAN + IMAGE_STDEV*psRandomGaussian(rnd));
        }
    }
    return image;
}

// Check a particular combination of image and interpolation parameters
// Each has 2 + 2*num tests
void check(int xSize, int ySize,        // Size of image
           psElemType type,             // Type for image
           int num,                     // Number of samples over image
           char *modeName, // Interpolation mode
           int xKernel, int yKernel,    // Size of interpolation kernel
           float tol,                    // Tolerance
	   psRandom *rnd		 // random seed
    )
{
    double imageVal, varianceVal;
    psImageInterpolateStatus interpOK;
    psImage *image = NULL;

    psImageInterpolateMode mode = psImageInterpolateModeFromString(modeName);
    // psImageMaskType maskVal;

    psMemId id = psMemGetLastId();

    int xCenter = xSize / 2;
    int yCenter = ySize / 2;

    psVector *pt0 = psVectorAlloc(num, PS_TYPE_F32);
    psVector *pt1 = psVectorAlloc(num, PS_TYPE_F32);
    psVector *pt2 = psVectorAlloc(num, PS_TYPE_F32);
    psVector *pt3 = psVectorAlloc(num, PS_TYPE_F32);

    // generate NUM images with Gaussian random noise, measure the interpolated value at
    // the specified points, save in vectors
    for (int i = 0; i < num; i++) {
	image = generateImage(image, rnd, xSize, ySize, type);
	
	psImageInterpolation *interp = psImageInterpolationAlloc(mode, image, NULL, NULL, 0, NAN, NAN, 0, 0, 0.0, 0);

	// point (0) : uninterpolated center of center pixel
	pt0->data.F32[i] = psImageGet(image, xCenter + 0.5, yCenter + 0.5);

	// point (1) : center of center pixel
        interpOK = psImageInterpolate(&imageVal, NULL, NULL, xCenter + 0.5, yCenter + 0.5, interp);
	pt1->data.F32[i] = imageVal;

	// point (2) : edge of center pixel
        interpOK = psImageInterpolate(&imageVal, NULL, NULL, xCenter + 0.5, yCenter, interp);
	pt2->data.F32[i] = imageVal;

	// point (3) : corner of center pixel
        interpOK = psImageInterpolate(&imageVal, NULL, NULL, xCenter, yCenter, interp);
	pt3->data.F32[i] = imageVal;

	psFree (interp);
    }

    fprintf (stderr, "results for interpolation mode %s\n", modeName);

    // measure the interpolate variance value at the specified locations
    {
	psImage *variance = generateVariance(xSize, ySize, type);
	psImageInterpolation *interp = psImageInterpolationAlloc(mode, image, variance, NULL, 0, NAN, NAN, 0, 0, 0.0, 0);
	psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);

        // Values from interpolation

	psStatsInit(stats);
	psVectorStats(stats, pt0, NULL, NULL, 0);
	fprintf (stderr, "pt 0 (pixel %5.2f,%5.2f) : pred: %6.3f, meas: %6.3f (mean: %6.3f)\n", xCenter + 0.5, yCenter + 0.5, IMAGE_STDEV, stats->sampleStdev, stats->sampleMean);

        interpOK = psImageInterpolate(&imageVal, &varianceVal, NULL, xCenter + 0.5, yCenter + 0.5, interp);
	psStatsInit(stats);
	psVectorStats(stats, pt1, NULL, NULL, 0);
	fprintf (stderr, "pt 1 (pixel %5.2f,%5.2f) : pred: %6.3f, meas: %6.3f (mean: %6.3f)\n", xCenter + 0.5, yCenter + 0.5, sqrt(varianceVal), stats->sampleStdev, stats->sampleMean);
	
        interpOK = psImageInterpolate(&imageVal, &varianceVal, NULL, xCenter + 0.5, yCenter, interp);
	psStatsInit(stats);
	psVectorStats(stats, pt2, NULL, NULL, 0);
	fprintf (stderr, "pt 2 (pixel %5.2f,%5.2f) : pred: %6.3f, meas: %6.3f (mean: %6.3f)\n", xCenter + 0.5, yCenter + 0.0, sqrt(varianceVal), stats->sampleStdev, stats->sampleMean);
	
        interpOK = psImageInterpolate(&imageVal, &varianceVal, NULL, xCenter, yCenter, interp);
	psStatsInit(stats);
	psVectorStats(stats, pt3, NULL, NULL, 0);
	fprintf (stderr, "pt 3 (pixel %5.2f,%5.2f) : pred: %6.3f, meas: %6.3f (mean: %6.3f)\n", xCenter + 0.0, yCenter + 0.0, sqrt(varianceVal), stats->sampleStdev, stats->sampleMean);

	psFree (stats);
	psFree (interp);
	psFree (variance);
    }

    psFree (image);
    psFree(pt0);
    psFree(pt1);
    psFree(pt2);
    psFree(pt3);

    ok(!psMemCheckLeaks(id, NULL, NULL, false), "No memory leaks");

    return;
}

# define NTESTS 1000

int main(int argc, char *argv[])
{
    psRandom *rnd = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);

    plan_tests(6);

    check(16, 16, PS_TYPE_F32, NTESTS, "BILINEAR", 2, 2, 1.0e-6, rnd); // 66 tests
    // check(16, 16, PS_TYPE_F64, NTESTS, "BILINEAR", 2, 2, 1.0e-6); // 66 tests

    check(16, 16, PS_TYPE_F32, NTESTS, "BIQUADRATIC", 3, 3, 1.0e-6, rnd); // 66 tests
    // check(16, 16, PS_TYPE_F64, NTESTS, "BIQUADRATIC", 3, 3, 1.0e-6); // 66 tests

    check(16, 16, PS_TYPE_F32, NTESTS, "GAUSS", 3, 3, 1.0e-3, rnd); // 66 tests
    // check(16, 16, PS_TYPE_F64, NTESTS, "GAUSS", 3, 3, 1.0e-3); // 66 tests

    check(16, 16, PS_TYPE_F32, NTESTS, "LANCZOS2", 4, 4, 1.0e-3, rnd); // 66 tests
    // check(16, 16, PS_TYPE_F64, NTESTS, "LANCZOS2", 4, 4, 1.0e-3); // 66 tests

    check(32, 32, PS_TYPE_F32, NTESTS, "LANCZOS3", 6, 6, 1.0e-3, rnd); // 66 tests
    // check(32, 32, PS_TYPE_F64, NTESTS, "LANCZOS3", 6, 6, 1.0e-3); // 66 tests

    check(32, 32, PS_TYPE_F32, NTESTS, "LANCZOS4", 8, 8, 1.0e-3, rnd); // 66 tests
    // check(32, 32, PS_TYPE_F64, NTESTS, "LANCZOS4", 8, 8, 1.0e-3); // 66 tests

    psFree (rnd);
    return exit_status();
}

