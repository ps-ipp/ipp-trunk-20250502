#include <stdio.h>
#include <pslib.h>

// Generate image with single high pixel
static psImage *generateImage(int numCols, int numRows, psRandom *rng)
{
    psImage *image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] = psRandomGaussian(rng);
        }
    }
    return image;
}


static psKernel *generateKernel(int numCols, int numRows)
{
    psKernel *kernel = psKernelAlloc(-numCols, numCols, -numRows, numRows);
    for (int y = -numRows; y <= numRows; y++) {
        for (int x = -numCols; x <= numCols; x++) {
            kernel->kernel[y][x] = psGaussian(sqrtf((float)(x*x + y*y)), 0.0,
                                              (float)PS_MIN(numCols, numRows) / 3.0, true);
        }
    }
    return kernel;
}

static void runBench(int imageCols, int imageRows, int kernelCols, int kernelRows, int iter, psRandom *rng)
{
    double direct = 0.0, fft = 0.0;     // Sum of elapsed times for the two methods
    for (int i = 0; i < iter; i++) {
        {
            psImage *image = generateImage(imageCols, imageRows, rng);
            psKernel *kernel = generateKernel(kernelCols, kernelRows);
            psTimerStart("direct");
            psImage *convolved = psImageConvolveDirect(NULL, image, kernel);
            direct += psTimerMark("direct");
            psFree(convolved);
            psFree(kernel);
            psFree(image);
            psTimerClear("direct");
        }

        {
            psImage *image = generateImage(imageCols, imageRows, rng);
            psKernel *kernel = generateKernel(kernelCols, kernelRows);
            psTimerStart("fft");
            psImage *convolved = psImageConvolveFFT(NULL, image, NULL, 0, kernel);
            fft += psTimerMark("fft");
            psFree(convolved);
            psFree(kernel);
            psFree(image);
            psTimerClear("fft");
        }
    }

    char size[16];
    sprintf(size, "%dx%d", imageCols, imageRows);
    printf("%15s", size);
    sprintf(size, "%dx%d", 2*kernelCols+1, 2*kernelRows+1);
    printf(" %15s", size);
    printf("        %8f        %8f\n", direct / iter, fft / iter);
}



int main(int argc, char *argv[])
{
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0); // Random number generator

    printf("#%14s%16s        %8s        %8s\n", "Image", "Kernel", "Direct", "FFT");
    runBench( 100,  100, 3, 3, 10, rng);
    runBench( 200,  200, 3, 3, 10, rng);
    runBench( 400,  400, 3, 3, 10, rng);
    runBench( 600,  600, 3, 3, 10, rng);
    runBench( 800,  800, 3, 3,  8, rng);
    runBench(1000, 1000, 3, 3,  6, rng);
    runBench(2000, 2000, 3, 3,  4, rng);
    runBench(4000, 4000, 3, 3,  2, rng);

    runBench(600, 600,  1,  1, 10, rng);
    runBench(600, 600,  2,  2, 10, rng);
    runBench(600, 600,  3,  3, 10, rng);
    runBench(600, 600,  4,  4, 10, rng);
    runBench(600, 600,  6,  6,  8, rng);
    runBench(600, 600,  8,  8,  4, rng);
    runBench(600, 600, 10, 10,  2, rng);
    runBench(600, 600, 15, 15,  2, rng);

    psFree(rng);
    psTimerStop();

    return EXIT_SUCCESS;
}
