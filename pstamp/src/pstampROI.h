#ifndef PSTAMP_ROI_H
#define PSTAMP_ROI_H

// Struct defining the Postage Stamp Region of Interest

typedef struct {
    double centerRA;
    double centerDEC;
    int    centerX;
    int    centerY;
    double dRA;
    double dDEC;
    int    dX;
    int    dY;
    bool   celestialCenter;       // true if center is in RA/dec
    bool   celestialRange;        // true if range is in RA/dec
    char   *center[2];            // copy of command line strings for center (from argv don't free)
    char   *range[2];             // copy of command line strings for range (from argv don't free)
} pstampROI;

bool pstampGetROI(pstampROI *roip, int *pArgc, char **argv, bool *gotCenter, bool *gotRange);

#endif
