/** @file  pmKHcorrect.h
 *  @brief Functions to read (and write?) Koppenhoefer correction file
 *
 *  The Koppenhoefer correction is needed for some chips of gpc1 before the camera voltages were adjusted 2011/05/11.
 *  The correction is a modification of the X (and possibly Y) coordinate of a star which depends on the instrumental 
 *  surface brightness, defined as -2.5 log_10 (DN) + 5.0 log_10 fwhm_maj [XXX be careful about the definition of fwhm_maj]
 *
 *  @ingroup AstroImage
 *  @author EAM, IfA
 *
 *  Copyright 2014 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_KH_CORRECT_H
#define PM_KH_CORRECT_H

/// @addtogroup Astrometry
/// @{

typedef struct {
    int N;
    float *xk;
    float *yk;
    float *y2;
} KHcorrectData;

KHcorrectData *KHcorrectDataAlloc (int Nrow);
float KHcorrectApply (KHcorrectData *spline, float X);

bool pmKHcorrectReadForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmKHcorrectReadFPA (pmFPAfile *file);
bool pmKHcorrectReadChips (pmFPAfile *file);

/// @}
#endif
