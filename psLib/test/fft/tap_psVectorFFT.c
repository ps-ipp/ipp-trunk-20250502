#include <stdio.h>
#include <math.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

#define TOL 2.5e-5                      // Tolerance for comparison


// Generate image with single high pixel
static psVector *generateVector(long num)
{
    psVector *vector = psVectorAlloc(num, PS_TYPE_F32);
    for (long i = 0; i < num; i++) {
        vector->data.F32[i] = 1.2 * cos(2.0 * M_PI * i / num + M_PI / 4.0) +
            3.4 * sin(0.5 * 2.0 * M_PI * i / num + M_PI);
    }
    return vector;
}

// FFT forward, then back --- do I get what I started with?
// A total of 6 tests here.
static void testFFT(long num)
{
    psMemId id = psMemGetId();

    psVector *old = generateVector(num);
    psVector *fftReal = NULL, *fftImag = NULL;
    bool result = psVectorForwardFFT(&fftReal, &fftImag, old);
    ok(result, "forward fft result");
    skip_start(!result || !fftReal || !fftImag, 3, "forward fft failed");
    ok(fftReal->type.type == PS_TYPE_F32 && fftImag->type.type == PS_TYPE_F32, "forward fft types");
    psVector *new = NULL;
    result = psVectorBackwardFFT(&new, fftReal, fftImag, old->n);
    ok(result, "backward fft result");
    skip_start(!result || !new, 2, "backward fft failed");
    ok(new->type.type == PS_TYPE_F32, "backward fft type");
    float maxDev = 0.0;                 // Maximum deviation from expected
    for (long i = 0; i < old->n; i++) {
        float dev = fabs(new->data.F32[i] / num - old->data.F32[i]);
        if (dev > maxDev) {
            maxDev = dev;
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
    plan_tests(8 + 6 * 3);

    // Test with NULL real arg
    {
        psMemId id = psMemGetId();
        psVector *real = psVectorAlloc(512, PS_TYPE_F32);;
        psVector *imag = psVectorAlloc(512, PS_TYPE_F32);
        psVector *in = psVectorAlloc(512, PS_TYPE_F32);
        bool rc = psVectorForwardFFT(NULL, &imag, in);
        ok(rc == false, "psVectorForwardFFT() returned FALSE with a null real vector input");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Test with NULL imag arg
    {
        psMemId id = psMemGetId();
        psVector *real = psVectorAlloc(512, PS_TYPE_F32);;
        psVector *imag = psVectorAlloc(512, PS_TYPE_F32);
        psVector *in = psVectorAlloc(512, PS_TYPE_F32);
        bool rc = psVectorForwardFFT(&real, NULL, in);
        ok(rc == false, "psVectorForwardFFT() returned FALSE with a null imag vector input");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Test with NULL input arg
    {
        psMemId id = psMemGetId();
        psVector *real = psVectorAlloc(512, PS_TYPE_F32);;
        psVector *imag = psVectorAlloc(512, PS_TYPE_F32);
        psVector *in = psVectorAlloc(512, PS_TYPE_F32);
        bool rc = psVectorForwardFFT(&real, &imag, NULL);
        ok(rc == false, "psVectorForwardFFT() returned FALSE with a null input vector input");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // Test with incorrect type for input arg
    {
        psMemId id = psMemGetId();
        psVector *real = psVectorAlloc(512, PS_TYPE_F32);;
        psVector *imag = psVectorAlloc(512, PS_TYPE_F32);
        psVector *in = psVectorAlloc(512, PS_TYPE_F64);
        bool rc = psVectorForwardFFT(&real, &imag, in);
        ok(rc == false, "psVectorForwardFFT() returned FALSE with a incorrect input vector type");
        psFree(real);
        psFree(imag);
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    testFFT(128);                       // Real quick test
    testFFT(2048);                      // Test something big
    testFFT(123456);                    // Test something really big
}
