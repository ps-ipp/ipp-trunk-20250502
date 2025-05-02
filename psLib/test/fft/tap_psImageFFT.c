#include <stdio.h>
#include <math.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

#define TOL 2.0e-5                      // Tolerance for comparison


// Generate image with single high pixel
static psImage *generateImage(int numCols, int numRows)
{
    psImage *image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] = 1.2 * cos(2.0 * M_PI * x / numCols + M_PI / 4.0) +
                3.4 * sin(2.0 * M_PI * y / numRows + M_PI);
        }
    }
    return image;
}

// FFT forward, then back --- do I get what I started with?
// A total of 6 tests here.
static void testFFT(int numCols, int numRows)
{
    psMemId id = psMemGetId();

    psImage *old = generateImage(numCols, numRows);
    psImage *fftReal = NULL, *fftImag = NULL;
    bool result = psImageForwardFFT(&fftReal, &fftImag, old);
    ok(result, "forward fft result");
    skip_start(!result || !fftReal || !fftImag, 3, "forward fft failed");
    ok(fftReal->type.type == PS_TYPE_F32 && fftImag->type.type == PS_TYPE_F32, "forward fft types");
    psImage *new = NULL;
    result = psImageBackwardFFT(&new, fftReal, fftImag, old->numCols);
    ok(result, "backward fft result");
    skip_start(!result || !new, 2, "backward fft failed");
    ok(new->type.type == PS_TYPE_F32, "backward fft type");
    float maxDev = 0.0;                 // Maximum deviation from expected
    for (int y = 0; y < old->numRows; y++) {
        for (int x = 0; x < old->numCols; x++) {
            float dev = fabs(new->data.F32[y][x] / numCols / numRows - old->data.F32[y][x]);
            if (dev > maxDev) {
                maxDev = dev;
            }
        }
    }
    ok(maxDev < TOL, "maximum deviation: %f", maxDev);
    psFree(new);
    skip_end();
    skip_end();

    psFree(fftReal);
    psFree(fftImag);
    psFree(old);
    ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");

    return;
}


int main(int argc, char *argv[])
{
    plan_tests(8 + 6 * 5);

    // Test with NULL real arg
    {
        psMemId id = psMemGetId();
        psImage *real = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *imag = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *in = psImageAlloc(512, 512, PS_TYPE_F32);
        bool rc = psImageForwardFFT(NULL, &imag, in);
        ok(rc == false, "psImageForwardFFT() returned FALSE with a NULL real image input");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Test with NULL imag arg
    {
        psMemId id = psMemGetId();
        psImage *real = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *imag = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *in = psImageAlloc(512, 512, PS_TYPE_F32);
        bool rc = psImageForwardFFT(&real, NULL, in);
        ok(rc == false, "psImageForwardFFT() returned FALSE with a NULL imaginary image input");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Test with NULL input arg
    {
        psMemId id = psMemGetId();
        psImage *real = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *imag = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *in = psImageAlloc(512, 512, PS_TYPE_F32);
        bool rc = psImageForwardFFT(&real, &imag, NULL);
        ok(rc == false, "psImageForwardFFT() returned FALSE with a NULL real image input");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Test with incorrect input image type
    {
        psMemId id = psMemGetId();
        psImage *real = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *imag = psImageAlloc(512, 512, PS_TYPE_F32);
        psImage *in = psImageAlloc(512, 512, PS_TYPE_F64);
        bool rc = psImageForwardFFT(NULL, &imag, in);
        ok(rc == false, "psImageForwardFFT() returned FALSE with incorrect input image type");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    testFFT(10, 10);                    // Real quick test
    testFFT(10, 20);                    // Testing differing numCols, numRows
    testFFT(20, 10);                    // Testing differing numCols, numRows
    testFFT(611, 610);                  // Test something like an OTA cell
    testFFT(2048, 4096);                // Test something like a megacam chip
}
