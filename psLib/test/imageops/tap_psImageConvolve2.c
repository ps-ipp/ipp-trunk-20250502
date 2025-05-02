#include <stdio.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

#define IMAGE_SIZE 21
#define KERNEL_SIZE 3
#define TOL 1.0e-6
#define MASK_INITIAL 0x08
#define MASK_FINAL   0x80


// Generate image with single high pixel
static psImage *generateImage(void)
{
    psImage *image = psImageAlloc(IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_F32);
    psImageInit(image, 0.0);
    image->data.F32[IMAGE_SIZE/2][IMAGE_SIZE/2] = 1.0;
    return image;
}

// Generate mask with single pixel set
static psImage *generateMask(void)
{
    psImage *mask = psImageAlloc(IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_MASK);
    psImageInit(mask, 0);
    mask->data.PS_TYPE_MASK_DATA[IMAGE_SIZE/2][IMAGE_SIZE/2] = MASK_INITIAL;
    return mask;
}

// Generate Gaussian kernel
static psKernel *generateKernel(void)
{
    psKernel *kernel = psKernelAlloc(- KERNEL_SIZE, KERNEL_SIZE, - KERNEL_SIZE, KERNEL_SIZE);
    for (int y = - KERNEL_SIZE; y <= KERNEL_SIZE; y++) {
        for (int x = - KERNEL_SIZE; x <= KERNEL_SIZE; x++) {
            kernel->kernel[y][x] = exp(- x*x - y*y);
        }
    }
    kernel->kernel[KERNEL_SIZE/3][KERNEL_SIZE/2] = 1.0;
    return kernel;
}

// Check the convolved image matches the kernel
static bool checkConvolved(const psImage *image, const psKernel *kernel)
{
    bool correct = true;
    for (int y = 0; y < IMAGE_SIZE; y++) {
        for (int x = 0; x < IMAGE_SIZE; x++) {
            if (x < IMAGE_SIZE/2 - KERNEL_SIZE || x > IMAGE_SIZE/2 + KERNEL_SIZE ||
                y < IMAGE_SIZE/2 - KERNEL_SIZE || y > IMAGE_SIZE/2 + KERNEL_SIZE) {
                if (fabs(image->data.F32[y][x]) > TOL) {
                    diag("%d,%d --> %f", x, y, fabs(image->data.F32[y][x]));
                    correct = false;
                }
            } else {
                // Position relative to the centre
                int kx = x - IMAGE_SIZE/2;
                int ky = y - IMAGE_SIZE/2;
                if (fabs(image->data.F32[y][x] - kernel->kernel[ky][kx]) > TOL) {
                    diag("%d,%d --> %f", x, y, fabs(image->data.F32[y][x] - kernel->kernel[ky][kx]));
                    correct = false;
                }
            }
        }
    }
    return correct;
}

// Check the convolved mask matches the kernel
static bool checkConvolvedMask(const psImage *mask)
{
    bool correct = true;
    for (int y = 0; y < IMAGE_SIZE; y++) {
        for (int x = 0; x < IMAGE_SIZE; x++) {
            if (x < IMAGE_SIZE/2 - KERNEL_SIZE || x > IMAGE_SIZE/2 + KERNEL_SIZE ||
                y < IMAGE_SIZE/2 - KERNEL_SIZE || y > IMAGE_SIZE/2 + KERNEL_SIZE) {
                if (mask->data.PS_TYPE_MASK_DATA[y][x] != 0) {
                    diag("%d,%d --> %d", x, y, mask->data.PS_TYPE_MASK_DATA[y][x]);
                    correct = false;
                }
            } else if (x == IMAGE_SIZE/2 && y == IMAGE_SIZE/2) {
                if (mask->data.PS_TYPE_MASK_DATA[y][x] != (MASK_INITIAL | MASK_FINAL)) {
                    diag("%d,%d --> %d", x, y, mask->data.PS_TYPE_MASK_DATA[y][x]);
                    correct = false;
                }
            } else if (mask->data.PS_TYPE_MASK_DATA[y][x] != MASK_FINAL) {
                diag("%d,%d --> %d", x, y, mask->data.PS_TYPE_MASK_DATA[y][x]);
                correct = false;
            }
        }
    }
    return correct;
}


int main(int argc, char *argv[])
{
    plan_tests(30);

    diag("psImageConvolve tests");

    {
        diag("Mask convolution");
        // Better (separable) mask convolution: 5 tests
        psMemId id = psMemGetId();

        psImage *mask = generateMask();

        psImage *convolved = psImageConvolveMask(NULL, mask, MASK_INITIAL, MASK_FINAL,
                                                 -KERNEL_SIZE, KERNEL_SIZE, -KERNEL_SIZE, KERNEL_SIZE);
        ok(convolved, "convolution result");
        skip_start(!convolved, 3, "convolution failed");
        ok(convolved->type.type == PS_TYPE_MASK, "output type");
        ok(convolved->numCols == IMAGE_SIZE && convolved->numRows == IMAGE_SIZE, "output size %dx%d",
           convolved->numCols, convolved->numRows);
        ok(checkConvolvedMask(convolved), "convolved mask correct");
        psFree(convolved);
        skip_end();

        psFree(mask);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    {
        diag("Mask convolution in-place");
        // Better (separable) in-place FFT mask convolution: 5 tests
        psMemId id = psMemGetId();

        psImage *mask = generateMask();

        bool result = psImageConvolveMask(mask, mask, MASK_INITIAL, MASK_FINAL, -KERNEL_SIZE, KERNEL_SIZE,
                                          -KERNEL_SIZE, KERNEL_SIZE);
        ok(result, "convolution result");
        skip_start(!result, 3, "convolution failed");
        ok(mask->type.type == PS_TYPE_MASK, "output type");
        ok(mask->numCols == IMAGE_SIZE && mask->numRows == IMAGE_SIZE, "output size %dx%d",
           mask->numCols, mask->numRows);
        ok(checkConvolvedMask(mask), "convolved mask correct");
        skip_end();

        psFree(mask);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    {
        diag("Direct convolution");
        // Direct convolution: 5 tests
        psMemId id = psMemGetId();

        psImage *image = generateImage();
        psKernel *kernel = generateKernel();

        psImage *convolved = psImageConvolveDirect(NULL, image, kernel);
        ok(convolved, "convolution result");
        skip_start(!convolved, 3, "convolution failed");
        ok(convolved->type.type == PS_TYPE_F32, "output type");
        ok(convolved->numCols == IMAGE_SIZE && convolved->numRows == IMAGE_SIZE, "output size %dx%d",
           convolved->numCols, convolved->numRows);
        ok(checkConvolved(convolved, kernel), "convolved image correct");
        psFree(convolved);
        skip_end();

        psFree(kernel);
        psFree(image);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    {
        diag("FFT convolution");
        // FFT convolution: 5 tests
        psMemId id = psMemGetId();

        psImage *image = generateImage();
        psKernel *kernel = generateKernel();

        psImage *convolved = psImageConvolveFFT(NULL, image, NULL, 0, kernel);
        ok(convolved, "convolution result");
        skip_start(!convolved, 3, "convolution failed");
        ok(convolved->type.type == PS_TYPE_F32, "output type");
        ok(convolved->numCols == IMAGE_SIZE && convolved->numRows == IMAGE_SIZE, "output size %dx%d",
           convolved->numCols, convolved->numRows);
        ok(checkConvolved(convolved, kernel), "convolved image correct");
        psFree(convolved);
        skip_end();

        psFree(kernel);
        psFree(image);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    {
        diag("FFT mask convolution");
        // FFT mask convolution: 5 tests
        psMemId id = psMemGetId();

        psImage *mask = generateMask();

        psImage *convolved = psImageConvolveMaskFFT(NULL, mask, MASK_INITIAL, MASK_FINAL,
                                                    -KERNEL_SIZE, KERNEL_SIZE, -KERNEL_SIZE, KERNEL_SIZE,
                                                    0.5);
        ok(convolved, "convolution result");
        skip_start(!convolved, 3, "convolution failed");
        ok(convolved->type.type == PS_TYPE_MASK, "output type");
        ok(convolved->numCols == IMAGE_SIZE && convolved->numRows == IMAGE_SIZE, "output size %dx%d",
           convolved->numCols, convolved->numRows);
        ok(checkConvolvedMask(convolved), "convolved mask correct");
        psFree(convolved);
        skip_end();

        psFree(mask);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    {
        diag("FFT mask convolution in-place");
        // In-place FFT mask convolution: 5 tests
        psMemId id = psMemGetId();

        psImage *mask = generateMask();

        bool result = psImageConvolveMaskFFT(mask, mask, MASK_INITIAL, MASK_FINAL, -KERNEL_SIZE, KERNEL_SIZE,
                                             -KERNEL_SIZE, KERNEL_SIZE, 0.5);
        ok(result, "convolution result");
        skip_start(!result, 3, "convolution failed");
        ok(mask->type.type == PS_TYPE_MASK, "output type");
        ok(mask->numCols == IMAGE_SIZE && mask->numRows == IMAGE_SIZE, "output size %dx%d",
           mask->numCols, mask->numRows);
        ok(checkConvolvedMask(mask), "convolved mask correct");
        skip_end();

        psFree(mask);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    return exit_status();
}
