#ifndef PPSTACK_THREAD_H
#define PPSTACK_THREAD_H

// Thread for stacking chunks
//
// Each input file contributes a readout, into which is read a chunk from that file
typedef struct {
    psArray *readouts;                  // Input readouts to read and stack
    bool read;                          // Has the scan been read?
    bool busy;                          // Is the scan being processed?
    int firstScan;                      // First row of the chunk to be read for this group
    int lastScan;                       // Last row of the chunk to be read for this group
    int entry;				// thread number for tracking progress
    int status;				// thread number for tracking progress
} ppStackThread;

// Allocator
ppStackThread *ppStackThreadAlloc(
    psArray *readouts                   // Inputs readouts to read and stack
    );

// Data for threads
typedef struct {
    psArray *threads;                   // Threads doing stacking
    int lastScan;                       // Last row that's been read
    psArray *imageFits;                 // FITS file pointers for images
    psArray *maskFits;                  // FITS file pointers for masks
    psArray *varianceFits;              // FITS file pointers for variances
    psArray *bkgFits;                   // FITS file pointers for background models
} ppStackThreadData;

// Set up thread data
ppStackThreadData *ppStackThreadDataSetup(
    const ppStackOptions *options,      // Options
    const pmConfig *config,             // Configuration
    bool conv                           // Use convolved products?
    );

// Read chunk into the first available file thread
ppStackThread *ppStackThreadRead(bool *status, // Status of read
                                 ppStackThreadData *stack, // Stacks available for reading
                                 pmConfig *config, // Configuration
                                 int numChunk, // Chunk number (only for interest)
                                 int overlap // Overlap between subsequent scans
    );

// Initialise the threads
void ppStackSetThreads(void);


enum {PPSTACK_THREAD_NEW, PPSTACK_THREAD_READ, PPSTACK_THREAD_RUN, PPSTACK_THREAD_SUCCESS, PPSTACK_THREAD_FAILURE};


#endif
