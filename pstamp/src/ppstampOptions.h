#ifndef PPSTAMP_OPTIONS_H
#define PPSTAMP_OPTIONS_H

#include "pstampROI.h"

// Options for ppstamp processing
typedef struct {
    // input arguments
    pstampROI   roip;
    bool    	centeroffchip;
    bool        wholeFile;
    psString    chipName;
    psString    cellName;
    psString    stage;
    psMetadata  *headerAdditions;
    bool        censorMasked;
    bool        writeJPEG;
    bool        writeCMF;
    bool        nocompress;
    //
    // Calculated Values
    //
    psString    outputFileRule;
    psRegion    roi;            // roi in chip coordinates
} ppstampOptions;

ppstampOptions *ppstampOptionsAlloc(void);

#endif


