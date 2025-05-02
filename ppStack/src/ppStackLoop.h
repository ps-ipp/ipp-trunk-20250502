#ifndef PPSTACK_LOOP_H
#define PPSTACK_LOOP_H

// Loop over the inputs, doing the combination
bool ppStackLoop(
    pmConfig *config,                    // Configuration
    ppStackOptions *options             // Options for stacking
    );

// Median only loop.
bool ppStackMedianLoop(
    pmConfig *config,                    // Configuration
    ppStackOptions *options             // Options for stacking
    );

// stack by percentile range
bool ppStackLoopByPercent(
  pmConfig *config,
  ppStackOptions *options
  );

// Setup
bool ppStackSetup(
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Preparation for stacking
//
// Merge input source lists, determine target PSF
bool ppStackPrepare(
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Convolve inputs to match target PSF
bool ppStackConvolve(
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Prepare for combination
bool ppStackCombinePrepare(
    const char *outName,                // Name of output file
    const char *expName,                // Name of exposure file
    const char *bkgName,                // Name of background file
    ppStackFileList files,              // Files of interest
    ppStackThreadData *stack,           // Stack
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Initial combination
bool ppStackCombineInitial(
    ppStackThreadData *stack,           // Stack
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Reject pixels
bool ppStackReject(
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Final combination
bool ppStackCombineFinal(
    ppStackThreadData *stack,           // Stack
    psArray *covariances,               // Covariances
    ppStackOptions *options,            // Options
    pmConfig *config,                   // Configuration
    bool safe,                          // Allow safe combination?
    bool norm,                          // Normalise images?
    bool grow,                           // Grow rejection masks?
    bool bscaleoffset                   // Apply bscale offset?
    );

// Cleanup following combination
bool ppStackCleanupFiles(
    ppStackThreadData *stack,           // Stack
    ppStackOptions *options,            // Options
    pmConfig *config,			// Configuration
    ppStackFileList stackFiles,         // cleanup these stack files
    ppStackFileList photFiles,          // cleanup these phot files (PHOT or NOP)
    bool closeJPEGs    			// close the jpeg files?
);

// Photometry
bool ppStackPhotometry(
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );

// Finish up
bool ppStackFinish(
    ppStackOptions *options,            // Options
    pmConfig *config                    // Configuration
    );


bool ppStackCombinePercent(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config);

bool ppStackUpdateHeader(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config);
bool ppStackJPEGs(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config);
bool ppStackStats(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config);

#endif
