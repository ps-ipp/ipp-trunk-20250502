/* @file  pmAstrometryRefstars.h
 * @brief Functions to write (and read?) astrometric reference stars
 *
 * @ingroup AstroImage
 *
 * @author EAM, IfA
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-07-17 22:38:15 $
 * Copyright 2008 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROMETRY_REFSTARS_H
#define PM_ASTROMETRY_REFSTARS_H

/// @addtogroup Astrometry
/// @{

bool pmAstromRefstarsCheckDataStatusForView (const pmFPAview *view, pmFPAfile *file);
bool pmAstromRefstarsCheckDataStatusForFPA (const pmFPA *fpa);
bool pmAstromRefstarsCheckDataStatusForChip (const pmChip *chip);
bool pmAstromRefstarsCheckDataStatusForCell (const pmCell *cell);
bool pmAstromRefstarsCheckDataStatusForReadout (const pmReadout *readout);

bool pmAstromRefstarsWriteForView (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmAstromRefstarsWritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config);

bool pmAstromRefstarsWriteFPA (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmAstromRefstarsWriteChip (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmAstromRefstarsWriteCell (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmAstromRefstarsWriteReadout (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);


/// @}
#endif
