/* @file  pmSourceSatstar.h
 *
 * @author EAM, IfA
 * @date $Date: 2012-08-30 $
 * Copyright 2012 University of Hawaii
 */

# ifndef PM_SOURCE_SATSTAR_H
# define PM_SOURCE_SATSTAR_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    float Xo;
    float Yo;
    float Rmax;
    psVector *logFmodel;
    psVector *logRmodel;
} pmSourceSatstar;


pmSourceSatstar *pmSourceSatstarAlloc();
bool psMemCheckSourceSatstar(psPtr ptr);

/// @}
# endif /* PM_SOURCE_SATSTAR_H */
