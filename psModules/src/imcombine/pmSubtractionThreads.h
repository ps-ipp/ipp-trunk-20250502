#ifndef PM_SUBTRACTION_THREADS_H
#define PM_SUBTRACTION_THREADS_H

/// Return whether theads have been activated
bool pmSubtractionThreaded(void);

/// Set up threading for image matching
///
/// Sets up thread tasks
void pmSubtractionThreadsInit(void);


/// Take down threading for image matching
///
/// Destroys thread tasks
void pmSubtractionThreadsFinalize(void);

#endif
