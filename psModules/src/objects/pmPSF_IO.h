/* @file  pmPSF.h
 *
 * This file contains typedefs for the Point-Spread Function and prototypes
 * for functions that calculate the PSF.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-07-17 22:38:15 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_PSF_IO_H
# define PM_PSF_IO_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

bool pmPSFmodelWriteForView (const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmPSFmodelWriteFPA (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmPSFmodelWriteChip (pmChip *chip, const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmPSFmodelWrite (const psMetadata *chipAnalysis, const psMetadata *roAnalysis, const pmFPAview *view, pmFPAfile *file, pmConfig *config);

bool pmPSFmodelWritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config);

bool pmPSFmodelReadForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmPSFmodelReadFPA (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmPSFmodelReadChip (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmPSFmodelRead (psMetadata *chipAnalysis, psMetadata *roAnalysis, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);

bool pmPSFmodelCheckDataStatusForView (const pmFPAview *view, const pmFPAfile *file);
bool pmPSFmodelCheckDataStatusForFPA (const pmFPA *fpa);
bool pmPSFmodelCheckDataStatusForChip (const pmChip *chip);

/// @}
# endif
