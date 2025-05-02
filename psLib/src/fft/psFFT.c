#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fftw3.h>
#include <pthread.h>

#include "psAssert.h"
#include "psLogMsg.h"
#include "psFFT.h"

static pthread_mutex_t fftwLock = PTHREAD_MUTEX_INITIALIZER; // Lock for FFTW

void psFFTLock(void)
{
    pthread_mutex_lock(&fftwLock);
}

void psFFTUnlock(void)
{
    pthread_mutex_unlock(&fftwLock);
}

bool psFFTThreads(int threads)
{
    PS_ASSERT_INT_NONNEGATIVE(threads, false);
#if HAVE_FFTW_THREADS
    static int numThreads = 0;          // Number of threads to use with FFTW
    if (threads > 0) {
        if (numThreads == 0) {
            fftwf_init_threads();
        }
        fftwf_plan_with_nthreads(threads);
    } else {
        fftwf_cleanup_threads();
    }
    numThreads = threads;
    return true;
#else
    if (threads > 0) {
        psWarning("No thread support for FFTW.");
        return false;
    }
    return true;
#endif
}
