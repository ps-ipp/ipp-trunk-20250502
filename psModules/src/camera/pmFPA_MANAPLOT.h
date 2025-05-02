/* @file  pmFPAview.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-24 02:54:14 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_MANAPLOT_H
#define PM_FPA_MANAPLOT_H

/// @addtogroup Camera Camera Layout
/// @{

bool pmFPAviewWriteMANAPLOT (const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmFPAWriteMANAPLOT (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmChipWriteMANAPLOT (pmChip *chip, const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmCellWriteMANAPLOT (pmCell *cell, const pmFPAview *view, pmFPAfile *file, pmConfig *config);
bool pmReadoutWriteMANAPLOT (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, pmConfig *config);

/// @}
# endif
