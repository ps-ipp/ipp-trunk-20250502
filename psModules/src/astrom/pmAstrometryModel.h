/* @file  pmAstrometryModel.h
 * @brief Astrometry model I/O functions
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-07-17 22:38:15 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROMETRY_MODEL_H
#define PM_ASTROMETRY_MODEL_H

/// @addtogroup Astrometry
/// @{

bool pmAstromModelCheckDataStatusForView (const pmFPAview *view, pmFPAfile *file);
bool pmAstromModelCheckDataStatusForFPA (const pmFPA *fpa);
bool pmAstromModelCheckDataStatusForChip (const pmChip *chip);

bool pmAstromModelWriteForView (const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmAstromModelWriteFPA (pmFPAfile *file, const pmFPA *fpa);
bool pmAstromModelWritePHU (pmFPAfile *file, const pmFPA *fpa);
bool pmAstromModelWriteSky (pmFPAfile *file);
bool pmAstromModelWriteTP (pmFPAfile *file);
bool pmAstromModelWriteFP (pmFPAfile *file);
bool pmAstromModelWriteChips (pmFPAfile *file);

bool pmAstromModelReadForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmAstromModelReadFPA (pmFPAfile *file);
bool pmAstromModelReadPHU (pmFPAfile *file);
bool pmAstromModelReadChips (pmFPAfile *file);
bool pmAstromModelReadFP (pmFPAfile *file);
bool pmAstromModelReadTP (pmFPAfile *file);
bool pmAstromModelReadSky (pmFPAfile *file);

bool pmAstromModelSetTP (pmFPAfile *file, psMetadata *concepts);

/// @}
#endif
