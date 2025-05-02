#ifndef PS_FFT_H
#define PS_FFT_H

// Only fftw_execute is thread-safe: all other calls have to be wrapped in a mutex

/// Lock FFT mutex
void psFFTLock(void);

/// Unlock FFT mutex
void psFFTUnlock(void);

/// Set number of threads for FFTW to use
///
/// Pass zero to turn off threading.  Note that the FFTW threads are *independent* of any other threads set up
/// throuhg, e.g., psThread.
bool psFFTThreads(int threads           // Number of threads
    );

#endif
