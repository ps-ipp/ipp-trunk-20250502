/* @file  pmFPAview.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-04-14 03:22:47 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_JPEG_H
#define PM_FPA_JPEG_H

/// @addtogroup Camera Camera Layout
/// @{

bool pmFPAviewWriteJPEG (const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmFPAWriteJPEG (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmChipWriteJPEG (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmCellWriteJPEG (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);
bool pmReadoutWriteJPEG (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config);

/// @}
# endif
