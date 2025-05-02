#include <stdio.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"


#define NUM_POINTS 32

// Function to define mask values
psMaskType maskFunc(float x, float y)
{
    return 0;
}

// Function to define variance values
double varianceFunc(float x, float y)
{
    return sqrtf(10.0 + 0.1 * x + 0.2 * y);
}

// Function to define image values
double imageFunc(float x, float y)
{
    return 0.01 * x + 0.02 * y;
}

// Generate a dummy mask image
psImage *generateMask(int numCols, int numRows)
{
    psImage *image = psImageAlloc(numCols, numRows, PS_TYPE_MASK);
    for (int j = 0; j < numRows; j++) {
        for (int i = 0; i < numCols; i++) {
            psImageSet(image, i, j, maskFunc(i + 0.5, j + 0.5));
        }
    }
    return image;
}

// Generate a dummy variance image
psImage *generateVariance(int numCols, int numRows, psElemType type)
{
    psImage *image = psImageAlloc(numCols, numRows, type);
    for (int j = 0; j < numRows; j++) {
        for (int i = 0; i < numCols; i++) {
            psImageSet(image, i, j, varianceFunc(i + 0.5, j + 0.5));
        }
    }
    return image;
}

// Generate a dummy image
psImage *generateImage(int numCols, int numRows, psElemType type)
{
    psImage *image = psImageAlloc(numCols, numRows, type);
    for (int j = 0; j < numRows; j++) {
        for (int i = 0; i < numCols; i++) {
            psImageSet(image, i, j, imageFunc(i + 0.5, j + 0.5));
        }
    }
    return image;
}

// Check a particular combination of image and interpolation parameters
// Each has 2 + 2*num tests
void check(int xSize, int ySize,        // Size of image
           psElemType type,             // Type for image
           int num,                     // Number of samples over image
           psImageInterpolateMode mode, // Interpolation mode
           int xKernel, int yKernel,    // Size of interpolation kernel
           float tol                    // Tolerance
    )
{

    psMemId id = psMemGetLastId();

    psImage *image = generateImage(xSize, ySize, type);
    psImage *variance = NULL; // generateVariance(xSize, ySize, type);
    psImage *mask = NULL; // generateMask(xSize, ySize);
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 12345);

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, image, variance, mask, 0, NAN, NAN,
                                                             0, 0, 0.0, 0);
    ok(interp, "Interpolation options set");
    skip_start(!interp, 2 * num, "Interpolation options failed");

    for (int i = 0; i < num; i++) {
        float x = psRandomUniform(rng) * (xSize - 1);
        float y = psRandomUniform(rng) * (ySize - 1);

        // Values from interpolation
        double imageVal, varianceVal;
        psImageMaskType maskVal;

        psImageInterpolateStatus interpOK = psImageInterpolate(&imageVal, &varianceVal, &maskVal,
                                                               x, y, interp);
        ok(interpOK != PS_INTERPOLATE_STATUS_ERROR, "Interpolation at %.2f,%.2f (%x)", x, y, interpOK);
        skip_start(interpOK == PS_INTERPOLATE_STATUS_ERROR, 1, "Interpolation failed");

        int xCentral = x - 0.5, yCentral = y - 0.5; // Central pixel of interpolation

        if (xCentral - (xKernel - 1) / 2 < 0 || xCentral + xKernel / 2 > xSize - 1 ||
            yCentral - (yKernel - 1) / 2 < 0 || yCentral + yKernel / 2 > ySize - 1) {
            ok(interpOK == PS_INTERPOLATE_STATUS_OFF || interpOK == PS_INTERPOLATE_STATUS_BAD ||
               interpOK == PS_INTERPOLATE_STATUS_POOR,
               "Interpolation at %.2f,%.2f (border): %x", x, y, interpOK);
        } else {
            is_double_tol(imageVal, imageFunc(x, y), tol, "Interpolation = %f vs %f (%d)",
                          imageVal, imageFunc(x, y), interpOK);
        }

        // Variance is very very very difficult to test without having the answer already
        // Mask might be a bit easier.
        // Defer these for now.

        skip_end();
    }

    psFree(interp);
    skip_end();

    psFree(rng);
    psFree(image);
    psFree(variance);
    psFree(mask);

    ok(!psMemCheckLeaks(id, NULL, NULL, false), "No memory leaks");

    return;
}

int main(int argc, char *argv[])
{
    plan_tests(792);

    check(16, 16, PS_TYPE_F32, 32, PS_INTERPOLATE_BILINEAR, 2, 2, 1.0e-6); // 66 tests
    check(16, 16, PS_TYPE_F64, 32, PS_INTERPOLATE_BILINEAR, 2, 2, 1.0e-6); // 66 tests

    check(16, 16, PS_TYPE_F32, 32, PS_INTERPOLATE_BIQUADRATIC, 3, 3, 1.0e-6); // 66 tests
    check(16, 16, PS_TYPE_F64, 32, PS_INTERPOLATE_BIQUADRATIC, 3, 3, 1.0e-6); // 66 tests

    check(16, 16, PS_TYPE_F32, 32, PS_INTERPOLATE_GAUSS, 3, 3, 1.0e-3); // 66 tests
    check(16, 16, PS_TYPE_F64, 32, PS_INTERPOLATE_GAUSS, 3, 3, 1.0e-3); // 66 tests

    check(16, 16, PS_TYPE_F32, 32, PS_INTERPOLATE_LANCZOS2, 4, 4, 1.0e-3); // 66 tests
    check(16, 16, PS_TYPE_F64, 32, PS_INTERPOLATE_LANCZOS2, 4, 4, 1.0e-3); // 66 tests

    check(32, 32, PS_TYPE_F32, 32, PS_INTERPOLATE_LANCZOS3, 6, 6, 1.0e-3); // 66 tests
    check(32, 32, PS_TYPE_F64, 32, PS_INTERPOLATE_LANCZOS3, 6, 6, 1.0e-3); // 66 tests

    check(32, 32, PS_TYPE_F32, 32, PS_INTERPOLATE_LANCZOS4, 8, 8, 1.0e-3); // 66 tests
    check(32, 32, PS_TYPE_F64, 32, PS_INTERPOLATE_LANCZOS4, 8, 8, 1.0e-3); // 66 tests

    return exit_status();
}

