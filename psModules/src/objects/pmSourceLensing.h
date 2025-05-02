/* @file  pmSourceLensing.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2014-03-20 02:31:25 $
 * Copyright 2014 IfA, University of Hawaii
 */

# ifndef PM_SOURCE_LENSING_H
# define PM_SOURCE_LENSING_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
  float X11;
  float X12;
  float X22;
  float e1;
  float e2;
} pmLensingPars;

typedef struct {
  pmLensingPars *smear;
  pmLensingPars *shear;
  float e1;
  float e2;
} pmSourceLensing; 

pmLensingPars *pmLensingParsAlloc ();
pmSourceLensing *pmSourceLensingAlloc ();

bool pmSourceLensingShearFromMoments (pmSourceLensing *lensing, pmMoments *moments, float sigma);
bool pmSourceLensingSmearFromMoments (pmSourceLensing *lensing, pmMoments *moments, float sigma);

/// @}
# endif /* PM_SOURCE_LENSING_H */
