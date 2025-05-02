#ifndef PSTAMP_INT_H
#define PSTAMP_INT_H

#include <stdio.h>
#include <string.h>
#include <strings.h>  // for strcasecmp
#include <unistd.h>   // for unlink
#include "pslib.h"
#include "psmodules.h"
#include "pstampErrorCodes.h"

#include "pstamp.h"
typedef enum {
    PSTAMP_UNKNOWN = -1,
    PSTAMP_RAW,
    PSTAMP_CHIP,
    PSTAMP_WARP,
    PSTAMP_DIFF,
    PSTAMP_STACK
} pstampImageType;


// command modes for pstampparse 
typedef enum {
    PSP_MODE_UNKNOWN = 0,
    PSP_MODE_QUEUE_JOB,
    PSP_MODE_LIST_URI,
    PSP_MODE_LIST_JOB
} pspMode;

#endif
