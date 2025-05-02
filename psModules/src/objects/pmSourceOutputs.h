/* @file  pmSourceOutputs.h
 * @brief functions to perform common I/O conversions
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.20 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2011 Institute for Astronomy, University of Hawaii
 *
 */

# ifndef PM_SOURCE_OUTPUTS_H
# define PM_SOURCE_OUTPUTS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    float xPos;
    float yPos;
    float xErr;
    float yErr;
    double ra;
    double dec;
    float psfMajor;
    float psfMinor;
    float psfTheta;
    float psfCore;
    float psfMajorFWHM;
    float psfMinorFWHM;
    float chisq;
    int nPix;
    int nDOF;
    float calMag;
    float peakMag;
    float posAngle;
    float pltScale;
} pmSourceOutputs;

typedef struct {
    float Mxx;
    float Mxy;
    float Myy;
    float M_c3;
    float M_s3;
    float M_c4;
    float M_s4;
    float Mrf;
    float Mrh;
    float Krf;
    float dKrf;
    float Kinner;
    float Kouter;
    float KronCore;
    float KronCoreErr;
    float KronPSF;
    float KronPSFErr;
} pmSourceOutputsMoments;

bool pmSourceOutputsCommonValues (float *magOffset, float *zeroptErr, float *fwhmMajor, float *fwhmMinor, pmReadout *readout, psMetadata *header);

bool pmSourceOutputsSetValues (pmSourceOutputs *outputs, pmSource *source, pmChip *chip, float fwhmMajor, float fwhmMinor, float magOffset);

bool pmSourceOutputsSetMoments (pmSourceOutputsMoments *moments, pmSource *source);

# endif
