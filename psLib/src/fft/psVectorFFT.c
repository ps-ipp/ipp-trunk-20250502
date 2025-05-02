/** @file  psVectorFFT.c
 *
 *  @brief Contains FFT transform related functions for psVector
 *
 *  @author Paul Price, IfA
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.40 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-14 03:23:13 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>

#include "psAssert.h"
#include "psError.h"
#include "psMemory.h"
#include "psLogMsg.h"
#include "psConstants.h"
#include "psThread.h"
#include "psFFT.h"
#include "psVectorFFT.h"

// Lock FFTW access
#define FFTW_LOCK \
if (threaded) { \
    psFFTLock(); \
}
// Unlock FFTW access
#define FFTW_UNLOCK \
if (threaded) { \
    psFFTUnlock(); \
}

#define FFTW_PLAN_RIGOR FFTW_ESTIMATE   // How rigorous the FFTW planning is

static psBool fftwWisdomImported = false; // Has the system wisdom been imported?

bool psVectorForwardFFT(psVector **real, psVector **imag, const psVector *in)
{
    PS_ASSERT_VECTOR_NON_NULL(in, false);
    PS_ASSERT_VECTOR_TYPE(in, PS_TYPE_F32, false);
    PS_ASSERT_PTR_NON_NULL(real, false);
    PS_ASSERT_PTR_NON_NULL(imag, false);

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    // Make sure the system-level wisdom information is imported.
    FFTW_LOCK;
    if (!fftwWisdomImported) {
        fftwf_import_system_wisdom();
        fftwWisdomImported = true;
    }
    FFTW_UNLOCK;

    long num = in->n;                   // Number of elements

    // Do the FFT
    FFTW_LOCK;
    fftwf_complex *out = fftwf_malloc((num/2 + 1) * sizeof(fftwf_complex)); // Output data
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(num, in->data.F32, out, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(plan);

    FFTW_LOCK;
    fftwf_destroy_plan(plan);
    FFTW_UNLOCK;

    // Pull the real and imaginary parts out
    num = num/2 + 1;
    *real = psVectorRecycle(*real, num, PS_TYPE_F32);
    *imag = psVectorRecycle(*imag, num, PS_TYPE_F32);
    for (int i = 0; i < num; i++) {
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
        // C99 complex support
        (*real)->data.F32[i] = creal(out[i]);
        (*imag)->data.F32[i] = cimag(out[i]);
#else
        // FFTW's backup complex support
        (*real)->data.F32[i] = out[i][0];
        (*imag)->data.F32[i] = out[i][1];
#endif
    }

    FFTW_LOCK;
    fftwf_free(out);
    FFTW_UNLOCK;

    return true;
}

bool psVectorBackwardFFT(psVector **out, const psVector *real, const psVector *imag, long origNum)
{
    PS_ASSERT_VECTOR_NON_NULL(real, false);
    PS_ASSERT_VECTOR_TYPE(real, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_NON_NULL(imag, false);
    PS_ASSERT_VECTOR_TYPE(imag, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(real, imag, false);
    PS_ASSERT_PTR_NON_NULL(out, false);

    bool threaded = psThreadPoolSize(); // Are we running threaded?

    // Make sure the system-level wisdom information is imported.
    FFTW_LOCK;
    if (!fftwWisdomImported) {
        fftwf_import_system_wisdom();
        fftwWisdomImported = true;
    }
    FFTW_UNLOCK;

    long num = real->n;                 // Number of elements

    // Stuff the real and imaginary parts in
    FFTW_LOCK;
    fftwf_complex *in = fftwf_malloc(num * sizeof(fftwf_complex)); // Input data
    FFTW_UNLOCK;
    for (int i = 0; i < num; i++) {
#if !defined(FFTW_NO_Complex) && defined(_Complex_I) && defined(complex) && defined(I)
        // C99 complex support
        in[i] = real->data.F32[i] + imag->data.F32[i] * I;
#else
        // FFTW's backup complex support
        in[i][0] = real->data.F32[i];
        in[i][1] = imag->data.F32[i];
#endif
    }


    // Do the FFT
    *out = psVectorRecycle(*out, origNum, PS_TYPE_F32);
    FFTW_LOCK;
    fftwf_plan plan = fftwf_plan_dft_c2r_1d(origNum, in, (*out)->data.F32, FFTW_PLAN_RIGOR);
    FFTW_UNLOCK;

    fftwf_execute(plan);

    FFTW_LOCK;
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    FFTW_UNLOCK;

    return true;
}

psVector *psVectorPowerSpectrum(psVector* out, const psVector* in)
{
    PS_ASSERT_VECTOR_NON_NULL(in, NULL);
    PS_ASSERT_VECTOR_TYPE(in, PS_TYPE_F32, NULL);

    psVector *real = NULL;              // Real component of FFT
    psVector *imag = NULL;              // Imaginary component of FFT

    if (!psVectorForwardFFT(&real, &imag, in)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to perform forward FFT.");
        return NULL;
    }

    int num = real->n;                  // Number of elements

    float norm = 1.0 / PS_SQR(in->n);
    for (int i = 0; i < num; i++) {
        // Power spectrum is the square of the complex modulus
        real->data.F32[i] = norm * (PS_SQR(real->data.F32[i]) + PS_SQR(imag->data.F32[i]));
    }
    psFree(imag);

    return real;
}

bool psVectorComplexMultiply(psVector **outReal, psVector **outImag,
                             const psVector *in1Real, const psVector *in1Imag,
                             const psVector *in2Real, const psVector *in2Imag)
{
    PS_ASSERT_VECTOR_NON_NULL(in1Real, false);
    PS_ASSERT_VECTOR_NON_NULL(in1Imag, false);
    PS_ASSERT_VECTOR_NON_NULL(in2Real, false);
    PS_ASSERT_VECTOR_NON_NULL(in2Imag, false);
    PS_ASSERT_VECTOR_TYPE(in1Real, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_TYPE(in1Imag, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_TYPE(in2Real, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_TYPE(in2Imag, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(in1Imag, in1Real, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(in2Real, in1Real, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(in2Imag, in1Real, false);
    PS_ASSERT_PTR_NON_NULL(outReal, false);
    PS_ASSERT_PTR_NON_NULL(outImag, false);
    if (*outReal) {
        PS_ASSERT_VECTOR_NON_NULL(*outReal, false);
        PS_ASSERT_VECTOR_NON_NULL(*outImag, false);
    } else if (*outImag) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "If the output real part is provided, the output imaginary part must also be provided.");
        return false;
    }

    int num = in1Real->n;               // Number of elements

    *outReal = psVectorRecycle(*outReal, num, PS_TYPE_F32);
    *outImag = psVectorRecycle(*outImag, num, PS_TYPE_F32);

    for (int i = 0; i < num; i++) {
        // (a + bi) * (c + di) = (ac - bd) + (bc + ad)i
        float real = in1Real->data.F32[i] * in2Real->data.F32[i] -
            in1Imag->data.F32[i] * in2Imag->data.F32[i];
        float imag = in1Imag->data.F32[i] * in2Real->data.F32[i] +
            in1Real->data.F32[i] * in2Imag->data.F32[i];

        (*outReal)->data.F32[i] = real;
        (*outImag)->data.F32[i] = imag;
    }

    return true;
}
